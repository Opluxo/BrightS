# 更新日志 | Changelog

## [v2.0.0] — 2026-05-29

### 目录结构优化 | Directory Structure Optimization
- 统一内核源码到 `kernel/` 目录，消除重复结构
- 移动 i386 架构代码到 `arch/i386/`，清理冗余目录
- 更新 CMakeLists.txt 适配新结构

### 文档全面更新 | Documentation Overhaul
- 更新 TODO.md、PROGRESS.md、SECURITY_STATUS.md、I386_PORTING_PLAN.md
- 修复 PROJECT_STRUCTURE.md 残缺内容
- 更新所有 README 系列文档

---

## [v1.0.0] — 2026-04-09

### 主要功能 | Major Features
- 完整操作系统内核：UEFI 启动、内存管理、文件系统、网络通信
- 多文件系统：RAMFS、DevFS、Btrfs
- LightShell 命令行：40+ 内置命令
- 设备驱动：NVMe、AHCI、PS/2、TTY、Framebuffer
- 网络协议栈：TCP/IP、VirtIO-Net、DNS、HTTP、DHCP
- 进程管理：fork/exec、信号、SMP、抢占式调度
- 多语言运行时：Rust、Python、C++ 内联执行

### 性能优化 | Performance Optimizations
- LTO 优化、SIMD 加速、LRU 缓存
- O(1) 调度器、Slab 分配器
- Phase 1-4 全部完成，预计性能提升 40-60%

### 安全特性 | Security Features
- SMEP/SMAP 硬件保护、堆分配器安全加固
- 系统调用边界检查

### 架构改进 | Architecture Improvements
- x86_64 UEFI + i386 BIOS 双架构支持

---

## 开发计划 | Roadmap

### 短期 | Short-term
- [ ] i386 完整内核功能移植
- [ ] USB 设备驱动
- [ ] 用户态权限管理

### 长期 | Long-term
- [ ] 图形用户界面
- [ ] 完整 POSIX 兼容性
- [ ] 更多硬件平台支持

---

*版本控制遵循语义化版本 | v0.1.2.3*
