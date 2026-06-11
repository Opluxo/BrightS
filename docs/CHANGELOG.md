# 更新日志 | Changelog

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

*版本控制遵循语义化版本 | v0.1.2.6*