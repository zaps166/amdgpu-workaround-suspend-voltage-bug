pkgname=amdgpu-workaround-suspend-voltage-bug
pkgver=1
pkgrel=1
pkgdesc='Workaround for the amdgpu voltage bug after resume from suspend when doing undervolting'
arch=('x86_64')
makedepends=(cmake gcc)
source=(
    'amdgpu-workaround-suspend-voltage-bug.install'
    'CMakeLists.txt'
    'main.cpp'
    'reset-amdgpu-clk-voltage.service'
    'upload-amdgpu-pp-table.service'
)
sha1sums=(
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
)
install='amdgpu-workaround-suspend-voltage-bug.install'

build() {
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
}

package() {
    install -D -m 755 "${srcdir}/build/amdgpu-workaround-suspend-voltage-bug" "${pkgdir}/usr/bin/amdgpu-workaround-suspend-voltage-bug"
    install -D -m 644 "${srcdir}/reset-amdgpu-clk-voltage.service" "${pkgdir}/etc/systemd/system/reset-amdgpu-clk-voltage.service"
    install -D -m 644 "${srcdir}/upload-amdgpu-pp-table.service" "${pkgdir}/etc/systemd/system/upload-amdgpu-pp-table.service"
}
