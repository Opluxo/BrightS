# 更新日志 | Changelog

## [v0.1.3.4] — 2026-06-17

### 主题系统 | Theme System
- `theme.h` / `theme.c`: 集中式 UI 颜色管理，15+ 颜色槽位
- 5 个内置主题: Dark, Ocean, Forest, Sunset, Mono
- 运行时切换: `theme dark`, `theme ocean`, `theme forest`, `theme sunset`, `theme mono`
- 所有 TUI 组件自动使用主题颜色

### 终端图形层渲染 | Terminal Graphics Layer
- `fb_console_scroll()` 通过 double buffer 执行，底部清空使用主题背景色
- `fb_console_clear()` 使用 `dbuffer_clear()` + `dbuffer_flip()` 统一刷新
- 退格键直接操作 double buffer 像素
- 终端背景色/前景色自动使用主题颜色

### ANSI 颜色支持 | ANSI Color Support
- 完整 SGR 解析: 30-37/40-47 (8色), 90-97/100-107 (亮色)
- 重置 (0), 加亮 (1), 正常亮度 (22), 默认色 (39/49)
- 16 色柔和调色板，适配暗色主题

### TUI 美化 | TUI Beautification
- 标题栏/状态栏使用主题渐变和颜色
- 增强状态栏: 用户名@brights#/$、时间、内存、进程数
- Unicode bullet 分隔符
- 光标: 8×14 方块光标，主题 accent 色，30 tick 闪烁

### Toast 通知系统 | Toast Notifications
- 4 个槽位，支持 info/success/warning/error 类型
- 滑入动画 + 淡出效果（~3 秒自动过期）
- `tui_toast()` API，shell 可调用

### 对话框/控件 | Dialog/Widgets
- `tui_draw_dialog()` — 通用圆角对话框
- `tui_draw_input_field()` — 聚焦高亮输入框

### 版本 | Version
- 所有文件版本号: v0.1.3.4

---

## [v0.1.3.3] — 2026-06-16

### Rust 算法优化 | Rust Algorithm Optimizations
- Robin Hood 哈希表: O(1) 均摊 get/insert/remove，用于 ARP 缓存查找
- LRU 缓存: O(1) 哈希查找 + 双向链表淘汰，为文件系统缓存设计
- 网络包验证: Rust-safe `rust_ip_validate` / `rust_tcp_validate` 替代不安全的 C 内联解析
- Rust IP 校验和: `rust_ip_checksum` 纯 Rust 实现
- Ring buffer: 零拷贝生产者-消费者缓冲区

### 安全漏洞修复 | Security Vulnerability Fixes
- TCP/IP 包解析: `ip_handle` / `tcp_handle` 添加长度验证，防止越界读取
- Signal handler: `brights_signal_sigaction` 改为 per-process 信号状态（不再全局共享）
- sys_mmap: 添加长度上限检查（0x10000000），预清零分配内存
- ramfs stat: 路径复制循环添加 `BRIGHTS_RAMFS_MAX_NAME` 边界检查
- pmem free: 已有位图双重释放保护（确认安全）

### Slab 分配器修复 | Slab Allocator Fixes
- Rust `slab_page_init` 路径: 添加 `slab_page_hash_insert`，修复 slab 页面不在哈希表中
- `slab_page_free_to_pmem`: 当所有槽位空闲时将 slab 页面归还 pmem
- 统一 kfree: O(1) 哈希表查找替代 O(n) 遍历
- SMEP/SMAP: CPUID 检测后条件启用，支持 QEMU TCG

### x86_64 架构修复 | x86_64 Architecture Fixes
- ISR ABI: `mov %rsp, %rdi` → `mov %rsp, %rcx`（Windows ABI 兼容）
- 32 个 CPU 异常向量全部有正确的 ISR stub
- `bsf` 内联汇编替换为 `__builtin_ctzll()`（lld-link 兼容）
- `-mno-movbe` 编译标志
- LTO 从内核构建中移除（防止关键堆栈切换被优化掉）
- 内核栈改为 256KB 静态 `.data` 数组

### 显示子系统 | Display Subsystem
- 双缓冲帧缓冲区渲染
- Bochs VBE dispi 图形模式: 1024×768×32（I/O 端口 0x1CE/0x1CF）
- `brights_paging_remap_uc()`: 帧缓冲区 PCD/PWT 缓存属性
- EFI Boot Services 结构体布局修复（ExitBootServices 成功）

### Ventoy UEFI 启动支持 | Ventoy UEFI Boot Support
- `tools/grub.cfg`: Ventoy 链式加载配置
- ISO 重建: xorriso + grub.cfg 三路径（/grub.cfg, /boot/grub/grub.cfg, /EFI/BOOT/grub.cfg）

### 版本 | Version
- 所有文件版本号: v0.1.3.3

---

## [v0.1.2.8] — 2026-06-12

### 安全修复 | Security Fixes
- sys_kill: 信号现在发送到目标进程，而非当前进程
- UDP VLA: 替换为固定大小缓冲区 + 边界检查
- sys_reboot: 正确的键盘控制器复位（端口 0x64）
- sys_shutdown: ACPI 关机（PM1a_CNT 寄存器）

### 系统调用 | Syscalls
- sys_env_set: 进程级环境变量存储（32 个变量）
- sys_getsockname/sys_getpeername: 返回真实套接字地址
- sys_system_load: 使用定点数代替 double（符合 AGENTS.md）
- sys_get_system_info: 查询真实 CPU/内存信息
- sys_process_list: 遍历实际进程表

### 网络改进 | Network
- TCP 状态常量: 所有魔数替换为命名常量
- UDP 校验和: 伪头部 + 数据校验和计算
- ARP 缓存: 增加到 64 条目，LRU 淘汰

### Shell 命令 | Shell
- nice: 设置当前进程调度优先级
- renice: 按 PID 更改任何进程优先级

### 代码质量 | Code Quality
- 移除 kernel_main.c 中的调试打印
- File_Struct_Define: 完整的文件系统结构文档

---

## [v0.1.2.7] — 2026-06-12

### 性能优化 | Performance Optimizations
- pmem: 多页分配提示缓存，跳过已扫描区域
- kmalloc: O(1) slab 释放（页→类哈希表）
- proc: O(1) PID 分配（位图 + BSF）
- clock: RDTSCP 序列化，纳秒精度提升

### 网络改进 | Network Improvements
- net.h: htons/ntohs/htonl/ntohl 字节序辅助函数
- ARP 缓存: TTL 过期（30 秒），防止陈旧条目
- ICMP: 正常 ident/seq 追踪，ping 显示 min/avg/max 统计
- TCP: 重传超时，指数退避（最多 5 次重试）

### Shell 命令 | Shell Commands
- 新增 19 个命令: top, df, which, type, tree, killall, nice, renice, bg, fg, wait, disown, umask, ulimit, realpath, seq, yes, ps
- ping: 使用 sleep 代替 spin-wait，显示统计信息

### 代码质量 | Code Quality
- 消除 ~100 行重复字节交换代码
- 修复调度器老化优先级检查

---

## [v0.1.2.6] — 2026-06-11

### 网络协议栈 | Network Stack (Real Implementation)
- DHCP 客户端完整实现：DISCOVER → OFFER → REQUEST → ACK 握手
- DNS 解析器：raw-byte 查询、名称压缩、A 记录解析
- HTTP 客户端：DNS → TCP → GET/POST → recv，内核态 HTTP 请求
- `netget` 命令：直接从 Shell 获取 URL 内容
- `brights_udp_send()` 修复：正确发送数据载荷（之前仅发送 8 字节 UDP 头）
- 广播 IP 处理：`0xFFFFFFFF` 绕过 ARP，直接使用 `FF:FF:FF:FF:FF:FF`
- `brights_net_poll_all()`：轮询所有注册的 NIC 驱动帧

### Shell 增强 | Shell Enhancements
- 环境变量系统：`env_table[ENV_MAX]`，支持 `env_get`/`env_set`/`env_sync`
- `export` 命令：设置环境变量
- `$VAR` 变量展开：`cmd_echo` 和 `handle_line` 中支持变量替换
- `uname` 命令：显示内核版本、架构、机器类型（条件编译 i386/x86_64）
- `bst` 命令：系统信息显示

### 架构改进 | Architecture
- x86_64/i386 条件编译：`uname`/`bst` 根据 `#ifdef __i386__` 显示正确架构
- FAT32 镜像修复：集群链、EBPB 损坏、集群偏移、LFN 校验和
- `mkfs.fat`+`mcopy` 回退：ISO 创建工具兼容性改进

### 品牌更新 | Branding
- OpenLight Studio → Opluxo LLC（全部 23 处引用）
- BSPM 仓库 URL 更新：`github.com/Opluxo/BrightS`
- CMake 项目名：`unix_v6_modern` → `BrightS`

### 文档 | Documentation
- 独立 `docs` 分支（orphan branch）
- Apple 风格 `index.html` 着陆页
- `view.html` Markdown 渲染器（marked.js + highlight.js）
- GitHub Pages 自动部署

---

## [v0.1.2.5] — 2026-05-29

### 目录结构优化 | Directory Structure Optimization
- 统一内核源码到 `kernel/` 目录，消除重复结构
- 移动 i386 架构代码到 `arch/i386/`，清理冗余目录
- 更新 CMakeLists.txt 适配新结构

### 文档全面更新 | Documentation Overhaul
- 更新 TODO.md、PROGRESS.md、SECURITY_STATUS.md、I386_PORTING_PLAN.md
- 修复 PROJECT_STRUCTURE.md 残缺内容
- 更新所有 README 系列文档

---

## [v0.1.0.0] — 2026-04-09

### 主要功能 | Major Features
- 完整操作系统内核：UEFI 启动、内存管理、文件系统、网络通信
- 多文件系统：RAMFS、DevFS、Btrfs
- LightShell 命令行：40+ 内置命令
- 设备驱动：NVMe、AHCI、PS/2、TTY、Framebuffer
- 网络协议栈：TCP/IP、VirtIO-Net
- 进程管理：fork/exec、信号、SMP、抢占式调度
- 多语言运行时：Rust、Python、C++ 内联执行

### 性能优化 | Performance Optimizations
- LTO 优化、SIMD 加速、LRU 缓存
- O(1) 调度器、Slab 分配器

### 安全特性 | Security Features
- SMEP/SMAP 硬件保护、堆分配器安全加固
- 系统调用边界检查

### 架构改进 | Architecture Improvements
- x86_64 UEFI + i386 BIOS 双架构支持

---

*版本控制遵循语义化版本 | v0.1.3.3*
