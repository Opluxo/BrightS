# 更新日志 | Changelog

## [v2.0.0] - 2026-05-29

### 🏗️ 目录结构优化 | Directory Structure Optimization
- 统一内核源码到 `kernel/` 目录，消除重复的 `sys/kernel/core/` 结构
- 删除重复的 `sys/kernel/platform/x86_64/`（与 `arch/x86_64/` 重复）
- 移动 i386 架构代码到 `arch/i386/`
- 清理项目根目录临时文件
- 删除根目录重复的 `OPTIMIZATION_REPORT.md`
- 更新 CMakeLists.txt 路径适配新结构

### 📚 文档全面更新 | Documentation Overhaul
- 更新 TODO.md 反映所有已完成功能
- 更新 PROGRESS.md 修复项目结构图
- 修复 PROJECT_STRUCTURE.md 残缺内容
- 更新 SECURITY_STATUS.md 反映最新安全状态
- 更新 I386_PORTING_PLAN.md 实际进度
- 更新所有 README 系列文档

### ⚡ 持续优化 | Ongoing Optimizations
- 项目结构扁平化，减少嵌套层级
- 消除代码重复，统一构建路径
- 文档与实际代码结构保持一致

---

## [v1.0.0] - 2026-04-09

### 🎉 主要功能 | Major Features
- **完整的操作系统内核**: UEFI 启动、内存管理、文件系统、网络通信
- **多文件系统支持**: RAMFS、DevFS、Btrfs
- **LightShell 命令行**: 40+ 内置命令
- **设备驱动**: NVMe、AHCI、PS/2 键盘、TTY 终端、Framebuffer
- **网络协议栈**: TCP/IP、VirtIO-Net、DNS、HTTP、DHCP
- **进程管理**: fork/exec、信号、SMP、抢占式调度
- **多语言运行时**: Rust、Python、C++ 内联执行

### ⚡ 性能优化 | Performance Optimizations
- LTO 优化、SIMD 加速、LRU 缓存
- O(1) 调度器、Slab 分配器
- Phase 1-4 全部优化完成，预计性能提升 40-60%

### 🔒 安全特性 | Security Features
- SMEP/SMAP 硬件保护
- 堆分配器安全加固
- 系统调用边界检查

### 🧪 测试覆盖 | Testing Coverage
- RAMFS 测试、性能基准测试、扩展测试套件

### 🏗️ 架构改进 | Architecture Improvements
- x86_64 UEFI + i386 BIOS 双架构支持
- VFS2 抽象层
- 设备驱动框架

---

## 开发计划 | Development Roadmap

### 短期目标 | Short-term Goals
- [ ] 完善 i386 平台完整内核功能
- [ ] USB 设备驱动
- [ ] 用户态权限管理
- [ ] 更完善的网络协议栈

### 长期目标 | Long-term Goals
- [ ] 图形用户界面
- [ ] 完整 POSIX 兼容性
- [ ] 更多硬件平台支持

---

*版本控制遵循语义化版本 | Semantic versioning applied*
