# BrightS (UNIX V6-inspired, x86_64 UEFI)

BrightS is an experimental x86_64 kernel inspired by UNIX V6 semantics.

## Goals
- Modern x86_64 kernel with simple, inspectable core design
- UEFI boot on physical hardware and QEMU
- Clear tree split: `/sys` (kernel), `/usr` (userspace), `/opt` (packages), `/config` (configuration)

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

Current shell behavior:
- Working directory is tracked (`pwd`, `cd`)
- Paths support `/`, relative paths, `.` and `..`
- RAMFS now models directories and regular files
- System and maintenance commands are grouped under `bst procom`

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
- Navigation: `pwd`, `cd`, `mkdir`
- Files: `ls`, `stat`, `cat`, `touch`, `write`, `append`, `rm`, `hexdump`, `echo`
- System/maintenance entry: `bst`

`bst` command layout:
- `bst help`
- `bst procom help`
- `bst procom version`
- `bst procom memory`
- `bst procom processes`
- `bst procom clock`
- `bst procom signals`
- `bst procom raise-signal <signo>`
- `bst procom clear-signals [signo]`
- `bst procom time`
- `bst procom keyboard-test`
- `bst procom mount`
- `bst procom clear`
- `bst procom enter-user`
- `bst procom reboot`
- `bst procom shutdown`

Quick session example:
```text
guest$ pwd
/
guest$ ls
config/
usr/
dev/
guest$ cd /usr/home/guest
guest$ touch note.txt
guest$ write note.txt hello
guest$ cat note.txt
hello
guest$ mkdir work
guest$ cd work
guest$ pwd
/usr/home/guest/work
```

Command migration notes:
- Top-level system/debug commands were removed to keep the normal shell small.
- Use `bst procom version` instead of `uname` or `version`.
- Use `bst procom memory` instead of `mem`.
- Use `bst procom processes` instead of `ps`.
- Use `bst procom clock` instead of `ticks`.
- Use `bst procom signals`, `bst procom raise-signal`, and `bst procom clear-signals` instead of `signal`, `raise`, and `clearsig`.
- Use `bst procom time` instead of `date`.
- Use `bst procom keyboard-test` instead of `kbdtest`.
- Use `bst procom mount` instead of `mount`.
- Use `bst procom clear` instead of `clear`.
- Use `bst procom enter-user` instead of `runuser`.
- Use `bst procom reboot` and `bst procom shutdown` instead of the removed top-level power commands.

## License
This project is licensed under `GNU GPL v2` (`GPL-2.0-only`). See `LICENSE`.
