# Adventure — QEMU/Buildroot edition (no WSL required)

Runs the same `adventure.elf` (built elsewhere in this repo from the
PL/I source) inside a minimal Linux system built with
[Buildroot](https://buildroot.org) (`qemu_x86_64_defconfig`), booted
by a portable copy of QEMU for Windows. No WSL, no Hyper-V, no driver
install, no admin rights — QEMU's software (TCG) emulation runs as a
plain user-mode process.

## How to play

Double-click **`Play.bat`**, or run it from a terminal. Say `NO` to
the instructions prompt to jump straight into the game.

Saves are real: `STORAGE` lives inside `rootfs.ext2` (a small ext2
disk image QEMU boots from), so `SAVE` writes land on that file on
your actual disk and are still there next time you launch `Play.bat`
— including from a completely separate run of the VM. Verified by
saving in one boot and restoring in an entirely separate `qemu`
process reading the same `rootfs.ext2`.

## What's in this folder

- `Play.bat` — the launcher. Just calls `qemu-system-x86_64.exe`
  directly with `-nographic`; no wrapper program of any kind.
- `qemu\` — a trimmed copy of the official QEMU-for-Windows build
  (qemu.weilnetz.de), just the x86_64 emulator + its DLLs + the BIOS
  files needed for a headless boot (no GPU/audio/network ROMs).
- `bzImage` — the Linux kernel (Buildroot 2025.02, Debian's default
  config plus `CONFIG_IA32_EMULATION=y`, needed because `adventure.elf`
  is a 32-bit binary — Iron Spring PL/I only targets 32-bit ELF).
- `rootfs.ext2` — the disk image: a minimal BusyBox-based root
  filesystem containing `adventure.elf`, its `OBJECT` database, a
  blank `STORAGE`, and an `/etc/inittab` entry that runs the game
  once at boot and powers the VM off when it exits.

## Rebuilding

`bzImage` and `rootfs.ext2` were produced by Buildroot 2025.02 in a WSL2
Debian distro (WSL is only needed to *build* this image, never to *run*
it). The tree itself is not kept here; recreating it means unpacking
Buildroot 2025.02, starting from `qemu_x86_64_defconfig`, and adding two
custom bits alongside it:

- an overlay directory, pointed at by `BR2_ROOTFS_OVERLAY`, containing
  `/opt/adventure/{adventure.elf,OBJECT,STORAGE}`,
  `/opt/adventure/launcher.sh`, `/etc/inittab`, and
  `/etc/network/interfaces` (network disabled — nothing's attached).
- a `kernel-fragment.config` holding the `CONFIG_IA32_EMULATION=y`
  fragment, wired in via `BR2_LINUX_KERNEL_CONFIG_FRAGMENT_FILES` in
  Buildroot's own `.config`.

To rebuild after changing `adventure.elf`: update the copy under
`overlay/opt/adventure/`, then `make` from the Buildroot tree (rootfs
repacks in seconds; the kernel and toolchain are cached and won't
rebuild unless their own config changed).

## Known rough edges

- Boot takes a few seconds (kernel + BusyBox init) before the game
  appears — inherent to any VM approach, not something to optimize
  away further without real embedded-Linux tuning.
- `rootfs.ext2` is fixed at 60MB; comfortably fits the game with
  headroom, but would need `BR2_TARGET_ROOTFS_EXT2_SIZE` bumped and a
  rebuild if much more content were ever added.
