@echo off
cd /d "%~dp0"
qemu\qemu-system-x86_64.exe -L qemu\share -M pc -kernel bzImage -drive file=rootfs.ext2,if=virtio,format=raw -append "rootwait root=/dev/vda console=ttyS0 panic=1" -m 256 -nographic -nic none -no-reboot
pause