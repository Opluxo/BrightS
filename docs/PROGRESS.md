# BrightS 操作系统 —— 项目进展

## 这是个什么项目？

BrightS 是一个**全新的操作系统**。就像 Windows、macOS、Linux 一样，它可以管理电脑硬件、运行程序、处理文件。

## 目前支持什么？

### 主体系统（x86_64 架构）
已在 64 位 UEFI 环境下完整运行，功能包括：
- **启动**：UEFI 引导 → 内核初始化 → Shell
- **显示**：framebuffer 图形输出，字体渲染
- **输入**：PS/2 键盘中断驱动
- **文件系统**：VFS2 抽象层，Btrfs、RAMFS、DevFS
- **内核**：抢占式调度、Slab 内存管理、进程管理、信号、SMP
- **网络**：TCP/IP、VirtIO-Net、DHCP、DNS、HTTP
- **Shell**：命令历史、Tab 补全、管道、重定向、后台作业
- **多语言运行时**：Rust/Python/C++ 内联执行
- **IPC**：管道（ring buffer + VFS）
- **存储**：AHCI、NVMe、ramdisk

### i386 老平台支持

已将 BrightS 移植到 32 位 x86 平台：
1. ✅ **BIOS 引导**：从软盘/USB 启动
2. ✅ **保护模式切换**：16→32 位
3. ✅ **PAE 分页**：3 级页表
4. ✅ **中断处理**：IDT + 8259A PIC
5. ✅ **内核入口**：跳转到 32 位内核代码
6. ✅ **系统调用**：sysenter/sysexit
7. ⏳ **完整内核功能移植**：将 x86_64 内核功能迁移到 i386

### 安全特性
- SMEP/SMAP 硬件保护
- 堆分配器安全加固（溢出、double-free、use-after-free）
- 系统调用边界检查

### 性能优化
- O(1) 调度器 + Slab 分配器
- SIMD 加速（memcpy/memset/memcmp）
- LRU 缓存系统（DNS、路径、inode、buffer）
- LTO 链接时优化

## 代码结构

```
BrightS/
├── arch/
│   ├── x86_64/            # 64位架构代码
│   └── i386/              # 32位架构代码
├── kernel/                # 内核核心
│   ├── core/              # 核心模块（调度、内存、进程等）
│   ├── dev/               # 设备驱动
│   ├── fs/                # 文件系统
│   ├── ipc/               # 进程间通信
│   └── net/               # 网络协议栈
├── drivers/               # 硬件驱动（display, GPU等）
├── user/                  # 用户态程序源码
├── include/kernel/        # 内核头文件
├── docs/                  # 文档
├── scripts/               # 构建脚本
├── tests/                 # 测试
├── config/                # 配置文件
├── CMakeLists.txt
└── Makefile
```

## 下一步

### 短期
- 完成 i386 完整内核功能移植
- 完善设备驱动（USB 等）
- 用户态权限管理

### 中期
- 图形用户界面（GUI）
- 更完善的网络协议栈
- POSIX 兼容性提升

### 长期
- 更多硬件平台支持
- 包管理系统生态
- 持续优化

*最后更新：2026-05-29*
