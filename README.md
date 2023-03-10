Workaround for AMD GPU driver bug which applies too low voltage after resuming from suspend when doing the undervolting.

**This software is not tested and can cause various issues. Use with care!**

Works on my MSI Radeon RX 6900XT GPU with Linux 6.1.14. This can trigger another amdgpu bug and crash the driver. The proper patch is already under review and will be applied to kernel soon.

---

Installation on Arch Linux based distributions:
```
git clone https://github.com/zaps166/amdgpu-workaround-suspend-voltage-bug
cd amdgpu-workaround-suspend-voltage-bug
makepkg -si
```
