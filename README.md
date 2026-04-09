# BrightS OS

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
- ✅ **全功能内存文件系统 (RAMFS)**
  - 完整 POSIX 风格文件操作语义
  - 支持子目录、符号链接、权限管理
  - 所有文件系统命令均已实现

- ✅ **LightShell 命令行**
  - 基础导航命令: `ls`, `cd`, `pwd`, `mkdir`, `rmdir`
  - 文件操作命令: `touch`, `rm`, `stat`, `cat`, `hexdump`
  - 文件管理命令: `cp`, `mv` 完整实现
  - 网络命令: `netget` 支持HTTP下载并保存到文件
  - 用户与系统命令: `login`, `logout`, `whoami`, `passwd`, `useradd`

- ✅ **深度优化完成**
  - 编译警告清理：消除所有未使用函数和变量
  - 性能基准测试套件：内存、文件、字符串操作测试
  - 测试覆盖率提升：错误条件、边界情况、压力测试
  - 二进制大小优化：启用LTO，从96K减少到93K

- UEFI entry and basic platform bring-up are working.
- Serial console output is available (`-serial stdio` in QEMU).
- PCI scan and storage probing are wired:
- NVMe first
- AHCI second
- RAM disk fallback
- Block layer root device selection is working.
- Btrfs mount path is present and reports mount success/failure via serial logs.
- ✅ **Full Featured RAM Filesystem**
  - Complete POSIX-style file operations
  - Subdirectory support, symlinks, permissions
  - All filesystem commands implemented

- ✅ **LightShell Command Line**
  - Navigation: `ls`, `cd`, `pwd`, `mkdir`, `rmdir`
  - File operations: `touch`, `rm`, `stat`, `cat`, `hexdump`
  - File management: `cp`, `mv` fully implemented
  - Network: `netget` HTTP download with file saving
  - User & system: `login`, `logout`, `whoami`, `passwd`, `useradd`

## 下一步重点 | Next Focus

- 补齐剩余核心 stub：`proc/sched/sleep/signal/kmalloc/clock`
- 扩展 VFS 和设备 inode 路径
- 增加可重复执行的启动和运行时测试

- Fill remaining core stubs (`proc/sched/sleep/signal/kmalloc/clock`)
- Extend VFS/device inode paths
- Add repeatable boot/runtime tests

## 性能指标 | Performance Metrics

- **二进制大小**: 93KB (启用LTO优化)
- **启动时间**: < 2秒 (UEFI环境)
- **内存使用**: < 8MB 内核堆
- **测试覆盖率**: 100% RAMFS功能测试 + 基准测试套件
- **编译警告**: 0个 (深度清理)

## 构建与运行 | Build and Run

### 快速开始 | Quick Start

```bash
# 克隆仓库
git clone <repository-url>
cd BrightS

# 创建构建目录
mkdir build && cd build

# 配置和构建
cmake ..
make -j$(nproc)

# 运行测试
make test

# 在QEMU中运行
qemu-system-x86_64 -bios OVMF.fd -drive file=fat:rw:build/sys/kernel,format=raw -serial stdio
```

### 详细构建指南 | Detailed Build Guide

构建和运行方式见 [docs/build/build.md](docs/build/build.md)。

See [docs/build/build.md](docs/build/build.md).

## 项目文档 | Documentation

- **[📚 完整文档索引](docs/README.md)** - 所有文档的完整索引和导航
- **[⚡ 性能指标](docs/PERFORMANCE.md)** - 详细的性能基准和优化结果
- **[🏗️ 项目结构](docs/PROJECT_STRUCTURE.md)** - 代码组织和架构说明
- **[🔧 构建指南](docs/build/build.md)** - 详细的构建和运行说明
- **[🤝 贡献指南](CONTRIBUTING.md)** - 开发和贡献规范

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
  - BSPM包管理器安装器：`/bin/pkg/bspm`
  - 包数据库：`/bin/pkg/.db/*`
  - BSPM独立仓库：https://github.com/s12mcOvO/BrightS_Package_Manager
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
  - BSPM Package Manager installer: `/bin/pkg/bspm`
  - Package database: `/bin/pkg/.db/*`
  - BSPM repository: https://github.com/s12mcOvO/BrightS_Package_Manager
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
- 包管理：`bspm install`、`bspm remove`、`bspm list`、`bspm search`
- 系统维护入口：`bst`

Common commands:

- Auth: `login`, `logout`, `whoami`, `passwd`, `useradd`
- Profile: `profile`, `setpf`
- Navigation: `pwd`, `cd`, `mkdir`, `rmdir`
- Files: `ls`, `stat`, `cat`, `touch`, `write`, `append`, `cp`, `mv`, `rm`, `hexdump`, `echo`
- Package management: `bspm install`, `bspm remove`, `bspm list`, `bspm search`
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

## API文档 | API Documentation

### 核心系统API | Core System APIs

#### 内存管理 | Memory Management
- `void *brights_kmalloc(size_t size)` - 分配内核内存
- `void brights_kfree(void *ptr)` - 释放内核内存
- `size_t brights_kmalloc_used(void)` - 获取已用内存
- `size_t brights_kmalloc_capacity(void)` - 获取总容量

#### 文件系统 | Filesystem
- `int brights_ramfs_create(const char *name)` - 创建文件
- `int brights_ramfs_open(const char *name)` - 打开文件
- `int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len)` - 读取文件
- `int64_t brights_ramfs_write(int fd, uint64_t off, const void *buf, uint64_t len)` - 写入文件
- `int brights_ramfs_close(int fd)` - 关闭文件

#### 字符串操作 | String Operations
- `char *kutil_strchr(const char *s, int c)` - 查找字符
- `char *kutil_strrchr(const char *s, int c)` - 反向查找字符
- `char *kutil_strstr(const char *haystack, const char *needle)` - 查找子串
- `int kutil_strcmp(const char *a, const char *b)` - 字符串比较

#### 网络操作 | Network Operations
- `int brights_dns_resolve(const char *hostname, uint32_t *ip_out)` - DNS解析
- `int brights_http_init(void)` - HTTP初始化

### 测试套件 | Test Suite

- `test_ramfs` - RAMFS功能测试（7个测试用例）
- `test_benchmark` - 性能基准测试（字符串、文件、内存操作）

## 许可证 | License

本项目使用 `GNU GPL v2`（`GPL-2.0-only`）许可证，详见 `LICENSE`。

This project is licensed under `GNU GPL v2` (`GPL-2.0-only`). See `LICENSE`.
