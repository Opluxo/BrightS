# BrightS (UNIX V6-inspired, x86_64 UEFI)

## 简介 | Overview

BrightS 是一个跨平台系统内核。

## 目标 | Goals

- 现代 x86_64 内核，核心设计尽量简单、可检查、可理解
- 支持物理机和 QEMU 下的 UEFI 启动
- 保持清晰的目录分层：`/sys`（内核）、`/usr`（用户空间）、`/opt`（包）、`/config`（配置）

- Modern x86_64 kernel with simple, inspectable core design
- UEFI boot on physical hardware and QEMU
- Clear tree split: `/sys` (kernel), `/usr` (userspace), `/opt` (packages), `/config` (configuration)

## 当前状态 | Current Status

- UEFI 入口和基础平台初始化已经可用
- 串口控制台输出已经可用，QEMU 下可通过 `-serial stdio` 使用
- 已接入 PCI 扫描和存储探测流程：
- `NVMe` 优先
- `AHCI` 次之
- 最后回退到 RAM disk
- 块设备根盘选择逻辑已工作
- 已有 Btrfs 挂载路径，并通过串口日志报告成功或失败

- UEFI entry and basic platform bring-up are working.
- Serial console output is available (`-serial stdio` in QEMU).
- PCI scan and storage probing are wired:
- NVMe first
- AHCI second
- RAM disk fallback
- Block layer root device selection is working.
- Btrfs mount path is present and reports mount success/failure via serial logs.

## 下一步重点 | Next Focus

- 补齐剩余核心 stub：`proc/sched/sleep/signal/kmalloc/clock`
- 扩展 VFS 和设备 inode 路径
- 增加可重复执行的启动和运行时测试

- Fill remaining core stubs (`proc/sched/sleep/signal/kmalloc/clock`)
- Extend VFS/device inode paths
- Add repeatable boot/runtime tests

## 构建与运行 | Build and Run

构建和运行方式见 [docs/build.md](/home/s12mc/CodeSpace/brights/docs/build.md)。

See [docs/build.md](/home/s12mc/CodeSpace/brights/docs/build.md).

## 交互式 Shell | Interactive Shell

系统启动后，BrightS 会进入串口 shell 提示符（`guest$` 或 `root#`）。

After boot, BrightS enters a serial shell prompt (`guest$` or `root#`).

当前 shell 行为：

- 维护当前工作目录（`pwd`、`cd`）
- 路径支持 `/`、相对路径、`.` 和 `..`
- RAMFS 现在同时支持目录和普通文件
- 系统和维护命令统一收敛到 `bst procom`

Current shell behavior:

- Working directory is tracked (`pwd`, `cd`)
- Paths support `/`, relative paths, `.` and `..`
- RAMFS now models directories and regular files
- System and maintenance commands are grouped under `bst procom`

用户和系统目录布局：

- 系统核心文件：`/sys/*`
- 用户文件：`/usr/home/*` 和 `/usr/home/<username>/*`
- 软件包：`/bin/pkg/*`
- 用户软件配置：`/bin/config/<username>/example.pf`
- 固件包：`/bin/firmware/*`
- 运行时环境：`/bin/runtime/*`
  - Rust：`/bin/runtime/rust/*`（rustc, cargo）
  - C：`/bin/runtime/c/*`（gcc, clang, make）
  - Python：`/bin/runtime/python/*`（python3, pip）
- 外部存储自动挂载元数据：`/mnt/drive/*`
- `/mnt/drive/.mounted`
- `/mnt/drive/fs`
- `/mnt/drive/role`
- `/mnt/drive/backend`
- 输入设备：`/mnt/input/*`
- 输出设备：`/mnt/output/*`
- 缓存文件：`/tmp/*`
- 交换分区：`/swp/*`

User/system layout:

- System core files: `/sys/*`
- User files: `/usr/home/*` and `/usr/home/<username>/*`
- Software packages: `/bin/pkg/*`
- User software configurations: `/bin/config/<username>/example.pf`
- Firmware packages: `/bin/firmware/*`
- Runtime environments: `/bin/runtime/*`
  - Rust: `/bin/runtime/rust/*` (rustc, cargo)
  - C: `/bin/runtime/c/*` (gcc, clang, make)
  - Python: `/bin/runtime/python/*` (python3, pip)
- External storage auto-mount metadata: `/mnt/drive/*`
- `/mnt/drive/.mounted`
- `/mnt/drive/fs`
- `/mnt/drive/role`
- `/mnt/drive/backend`
- Input devices: `/mnt/input/*`
- Output devices: `/mnt/output/*`
- Cache files: `/tmp/*`
- Swap partition: `/swp/*`

系统盘策略：

- 系统启动存储当前要求为 `Btrfs`
- 如果 Btrfs 挂载失败，内核会直接进入 halt，而不是继续启动 shell

System disk policy:

- System boot storage is expected to be `Btrfs`.
- If Btrfs mount fails, kernel enters halt state instead of normal shell startup.

常用命令：

- 认证：`login`、`logout`、`whoami`、`passwd`、`useradd`
- 配置：`profile`、`setpf`
- 导航：`pwd`、`cd`、`mkdir`、`rmdir`
- 文件：`ls`、`stat`、`cat`、`touch`、`write`、`append`、`cp`、`mv`、`rm`、`hexdump`、`echo`
- 系统维护入口：`bst`

Common commands:

- Auth: `login`, `logout`, `whoami`, `passwd`, `useradd`
- Profile: `profile`, `setpf`
- Navigation: `pwd`, `cd`, `mkdir`, `rmdir`
- Files: `ls`, `stat`, `cat`, `touch`, `write`, `append`, `cp`, `mv`, `rm`, `hexdump`, `echo`
- System/maintenance entry: `bst`

`bst` 命令结构 / `bst` command layout:

- `bst help`
- `bst procom help`
- `bst procom version`
- `bst procom cpu`
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

快速示例 / Quick session example:

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

命令迁移说明：

- 顶层系统/调试命令被移除，以保持常规 shell 更小
- 用 `bst procom version` 替代 `uname` 或 `version`
- 用 `bst procom memory` 替代 `mem`
- 用 `bst procom cpu` 查看 CPU 厂商/家族/型号与关键特性
- 用 `bst procom processes` 替代 `ps`
- 用 `bst procom clock` 替代 `ticks`
- 用 `bst procom signals`、`bst procom raise-signal`、`bst procom clear-signals` 替代 `signal`、`raise`、`clearsig`
- 用 `bst procom time` 替代 `date`
- 用 `bst procom keyboard-test` 替代 `kbdtest`
- 用 `bst procom mount` 替代 `mount`
- 用 `bst procom clear` 替代 `clear`
- 用 `bst procom enter-user` 替代 `runuser`
- 用 `bst procom reboot` 和 `bst procom shutdown` 替代被删除的顶层电源命令

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

## 许可证 | License

本项目使用 `GNU GPL v2`（`GPL-2.0-only`）许可证，详见 `LICENSE`。

This project is licensed under `GNU GPL v2` (`GPL-2.0-only`). See `LICENSE`.
