// SPDX-License-Identifier: Unlicense

#include <unistd.h>
#include <fcntl.h>

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

enum class Mode
{
    UNKNOWN,
    BEFORE_SUSPEND,
    AFTER_RESUME,
};

class FD
{
public:
    FD(int fd)
        : m_fd(fd)
    {}
    FD(const FD &other) = delete;
    ~FD()
    {
        if (m_fd > -1)
            close(m_fd);
    }

    operator int() const
    {
        return m_fd;
    }

private:
    const int m_fd;
};

class Array
{
public:
    Array() = default;
    Array(const Array &other) = delete;
    Array(Array &&other)
    {
        swap(m_data, other.m_data);
        swap(m_capacity, other.m_capacity);
        swap(m_size, other.m_size);
    }
    ~Array()
    {
        delete[] m_data;
    }

    bool allocate(ssize_t size)
    {
        if (m_capacity > 0)
            return false;

        m_data = new uint8_t[size];
        m_capacity = size;
        return true;
    }

    bool set_size(ssize_t size)
    {
        if (size < 0 || size > m_capacity)
            return false;

        m_size = size;
        return true;
    }

    const uint8_t *data() const
    {
        return m_data;
    }
    uint8_t *data()
    {
        return m_data;
    }

    ssize_t size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return (m_size <= 0);
    }

private:
    uint8_t *m_data = nullptr;
    ssize_t m_capacity = 0;
    ssize_t m_size = 0;
};

static bool has_pp_override_mask()
{
    constexpr uint32_t PP_OVERDRIVE_MASK = 0x4000;

    ifstream f;
    f.open("/sys/module/amdgpu/parameters/ppfeaturemask");
    if (!f.is_open())
        return false;

    uint32_t ppfeaturemask = 0;
    f >> hex >> ppfeaturemask;
    return (ppfeaturemask & PP_OVERDRIVE_MASK);
}

static bool is_vendor_amd(const string &path)
{
    constexpr uint16_t VENDOR_ID_AMD = 0x1002;

    ifstream f;
    f.open(path + "/vendor");
    if (!f.is_open())
        return false;

    uint16_t vendor_id = 0;
    f >> hex >> vendor_id;
    return (vendor_id == VENDOR_ID_AMD);
}

static bool is_enabled(const string &path)
{
    ifstream f;
    f.open(path + "/enable");
    if (!f.is_open())
        return false;

    bool enable = false;
    f >> enable;
    return enable;
}

static bool reset_clk_voltage(const string &path)
{
    const FD fd(open((path + "/pp_od_clk_voltage").c_str(), O_WRONLY));
    if (fd < 0)
        return false;

    const auto write_size = write(fd, "r", 1);
    return (write_size == 1);
}

static Array fetch_pp_table(const string &path)
{
    Array data;

    const FD fd(open((path + "/pp_table").c_str(), O_RDONLY));
    if (fd < 0)
        return data;

    const auto size = lseek(fd, 0, SEEK_END);
    if (size <= 0)
        return data;

    if (lseek(fd, 0, SEEK_SET) != 0)
        return data;

    data.allocate(size);

    const auto size_read = read(fd, data.data(), size);
    if (size_read <= 0)
        return data;

    data.set_size(size_read);
    return data;
}
static bool upload_pp_table(const string &path, const Array &data)
{
    if (data.empty())
        return false;

    const FD fd(open((path + "/pp_table").c_str(), O_WRONLY));
    if (fd < 0)
        return false;

    const auto write_size = write(fd, data.data(), data.size());
    return (write_size == data.size());
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return -1;

    const string_view arg(argv[1]);
    Mode mode = Mode::UNKNOWN;

    if (arg == "suspend")
        mode = Mode::BEFORE_SUSPEND;
    else if (arg == "resume")
        mode = Mode::AFTER_RESUME;

    if (mode == Mode::UNKNOWN)
        return -1;

    if (!has_pp_override_mask())
        return 0; // Nothing to do

    bool once = false;

    for (const auto &entry : filesystem::directory_iterator("/sys/class/drm"))
    {
        if (!entry.is_directory())
            continue;

        const auto name = entry.path().filename().string();
        const auto name_view = string_view(name);

        if (name_view.substr(0, 4) != "card")
            continue;

        { // Check if matches the pattern "cardN" where "N" is a number
            const auto number = name_view.substr(4);
            const auto it = find_if(number.begin(), number.end(), [](auto &&c) {
                return !isdigit(c);
            });
            if (it != number.end())
                continue;
        }

        const auto device_path = entry.path().string() + "/device";

        if (!is_vendor_amd(device_path))
            continue; // Not an AMD GPU

        if (!is_enabled(device_path))
            continue; // GPU is not enabled

        const auto pp_table = fetch_pp_table(device_path);
        if (pp_table.empty())
            continue; // Can't fetch PP table

        switch (mode)
        {
            case Mode::BEFORE_SUSPEND:
            {
                if (!once)
                {
                    system("killall corectrl_helper -SIGSTOP 2> /dev/null"); // Pause CoreCtrl process before suspend
                    once = true;
                }

                const bool ret = reset_clk_voltage(device_path);
                cerr << "Reset clock and voltage " << (ret ? "succeeded" : "failed") << " for " << name << endl;

                break;
            }
            case Mode::AFTER_RESUME:
            {
                const bool ret = upload_pp_table(device_path, pp_table);
                cerr << "PP table upload " << (ret ? "succeeded" : "failed") << " for " << name << endl;

                once = true;

                break;
            }
            default:
            {
                break;
            }
        }
    }

    if (mode == Mode::AFTER_RESUME && once)
        system("killall corectrl_helper -SIGCONT 2> /dev/null"); // Resume CoreCtrl process after resetting the SMU

    return 0;
}
