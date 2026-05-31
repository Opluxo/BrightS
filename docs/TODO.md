# BrightS 开发路线图 | Development Roadmap

v0.1.2.4 — 所有计划功能已完成

---

| 模块 | 功能 | 状态 |
|------|------|:----:|
| **内核核心** | Slab 分配器（10 大小类，8MB 堆） | ✅ |
| | O(1) 调度器（round-robin，10-tick 量子） | ✅ |
| | ERMS memcpy/memset/memcmp，SSE2 优化 | ✅ |
| | TSC 纳秒级计时 | ✅ |
| | 42 个系统调用（sysent 调度表） | ✅ |
| | 4 级页表管理 | ✅ |
| | PID 位图 O(1) 分配 | ✅ |
| | 信号处理框架 | ✅ |
| | SMP 多处理器引导 | ✅ |
| | 物理内存管理 | ✅ |
| **Shell** | 命令历史、Tab 补全 | ✅ |
| | 管道、重定向、后台作业 | ✅ |
| | 多语言内联执行（Rust/Python/C++） | ✅ |
| **文件系统** | VFS2 抽象层（mount/resolve/read/write） | ✅ |
| | RAMFS、DevFS、Btrfs | ✅ |
| | 管道 IPC（ring buffer + VFS） | ✅ |
| | 块设备接口 | ✅ |
| **中断与定时器** | PIC 重映射、PIT 100Hz、APIC/IOAPIC、HPET | ✅ |
| | 抢占式调度（10-tick 量子） | ✅ |
| | PS/2 键盘中断 | ✅ |
| **设备驱动** | PS/2 键盘、TTY、Framebuffer、字体渲染 | ✅ |
| | 串口、RTC、AHCI、NVMe | ✅ |
| | GPU HAL 框架、Vulkan 着色器 | ✅ |
| **网络** | TCP/IP 协议栈、VirtIO-Net | ✅ |
| | DHCP 客户端、DNS 解析器、HTTP 客户端 | ✅ |
| **用户态** | ELF64 加载器、用户进程上下文 | ✅ |
| | sys_fork/sys_exec、嵌入式 Shell ELF | ✅ |
| | 服务管理框架、Init 系统 | ✅ |
| | syslogd、dhcpd、sysinfo 工具 | ✅ |
| **性能优化** | Shell 自动补全 O(n²)→O(n) | ✅ |
| | Slab 分配器线性→二分搜索 | ✅ |
| | VFS 路径解析缓存（16 条目） | ✅ |
| | SIMD 加速（memcpy/memset/memcmp/CRC32） | ✅ |
| | LTO：96KB→93KB | ✅ |
| | 全局 LRU 缓存系统（DNS/路径/inode/buffer） | ✅ |
| **安全** | SMEP/SMAP 硬件保护 | ✅ |
| | 系统调用边界检查 | ✅ |
| | 堆整数溢出、double-free、use-after-free 防护 | ✅ |
| **架构支持** | x86_64 UEFI 启动 | ✅ |
| | i386 BIOS 启动 + 保护模式 | ✅ |
| | i386 PAE 分页 + 中断处理 | ✅ |

---

*最后更新：2026-05-29*
