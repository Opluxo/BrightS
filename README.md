# BrightS (UNIX V6-inspired, x86_64 UEFI)

BrightS is an experimental x86_64 kernel inspired by UNIX V6 semantics.

## Goals
- Modern x86_64 kernel with simple, inspectable core design
- UEFI boot on physical hardware and QEMU
- Clear tree split: `/sys` (kernel), `/usr` (userspace), `/opt` (packages), `/config` (configuration)
- Historical UNIX V6 sources preserved under `legacy/`

## Current Status
- UEFI entry and basic platform bring-up are working.
- Serial console output is available (`-serial stdio` in QEMU).
- PCI scan and storage probing are wired:
  - NVMe first
  - AHCI second
  - RAM disk fallback
- Block layer root device selection is working.
- Btrfs mount path is present and reports mount success/failure via serial logs.

## Next Focus
- Fill remaining core stubs (`proc/sched/sleep/signal/kmalloc/clock`)
- Extend VFS/device inode paths
- Add repeatable boot/runtime tests

## Build and Run
See `docs/build.md`.

## Interactive Shell
After boot, BrightS enters a serial shell prompt (`guest$` or `root#`).

User/system layout:
- System userspace config: `/config/userspace/*`
- Per-user profile: `/config/<username>/example.pf`
- User files: `/usr/home/*` and `/usr/home/<username>/*`
- External storage auto-mount metadata: `/dev/mnt/*`
  - `/dev/mnt/.mounted`
  - `/dev/mnt/fs`
  - `/dev/mnt/role`
  - `/dev/mnt/backend`

System disk policy:
- System boot storage is expected to be `Btrfs`.
- If Btrfs mount fails, kernel enters halt state instead of normal shell startup.

Common commands:
- Auth: `login`, `logout`, `whoami`, `passwd`, `useradd`
- Profile: `profile`, `setpf`
- Files: `ls`, `stat`, `cat`, `touch`, `write`, `append`, `rm`, `hexdump`
- System: `uname`, `pwd`, `mem`, `mount`, `runuser`, `reboot`, `halt`

## License
This project is licensed under `GNU GPL v2` (`GPL-2.0-only`). See `LICENSE`.
