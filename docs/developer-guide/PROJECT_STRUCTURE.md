# BrightS 项目目录结构

## 当前目录结构

```
BrightS/
├── arch/                          # 架构相关代码
│   ├── x86_64/                    # x86_64 架构（UEFI）
│   │   ├── uefi_entry.c           # UEFI 入口
│   │   ├── idt.c / idt.h         # 中断描述符表
│   │   ├── gdt.c / gdt.h         # 全局描述符表
│   │   ├── trap.c / trap.h       # 异常/陷阱处理
│   │   ├── isr.S                  # 中断服务程序
│   │   ├── paging.c / paging.h   # 4级分页
│   │   ├── apic.c / ioapic.c     # 高级可编程中断控制器
│   │   ├── hpet.c                # 高精度事件定时器
│   │   ├── syscall_entry.S       # 系统调用入口
│   │   ├── syscall_abi.c         # 系统调用ABI
│   │   ├── user_enter.S          # 用户模式进入
│   │   ├── user_stub.S           # 用户态stub
│   │   ├── cpu_local.c           # 每CPU变量
│   │   ├── pci.c                 # PCI配置
│   │   ├── mtrr.c                # MTRR
│   │   ├── pic_pit.c             # PIC + PIT
│   │   ├── uefi_memmap.c         # UEFI内存映射
│   │   └── ...                    # 头文件等
│   └── i386/                      # i386 架构（BIOS）
│       ├── boot/
│       │   ├── bootloader.asm    # BIOS引导程序
│       │   └── test_boot.asm     # 测试引导
│       ├── entry.asm / entry.c   # 内核入口
│       ├── gdt.c / idt.c
│       ├── paging.c / paging.h   # PAE分页
│       ├── pic_pit.c / pit.h
│       ├── isr.asm
│       ├── syscall_abi.c
│       ├── trap.c
│       └── link.ld               # 链接脚本
│
├── kernel/                        # 内核核心源码
│   ├── core/                      # 核心子系统
│   │   ├── kernel_main.c          # 内核主入口
│   │   ├── kmalloc.c              # Slab分配器
│   │   ├── sched.c                # 进程调度器
│   │   ├── proc.c                 # 进程管理
│   │   ├── syscall.c              # 系统调用处理
│   │   ├── sysent.c               # 系统调用表
│   │   ├── syscalls_extended.c    # 扩展系统调用
│   │   ├── signal.c               # 信号处理
│   │   ├── sleep.c                # 睡眠/定时
│   │   ├── clock.c                # 时钟管理
│   │   ├── acpi.c                 # ACPI
│   │   ├── runtime.c              # 运行时优化
│   │   ├── elf.c                  # ELF加载器
│   │   ├── userinit.c             # 用户进程初始化
│   │   ├── lightshell.c           # 内建Shell
│   │   ├── printf.c               # 格式化输出
│   │   ├── pf.c                   # 页面错误
│   │   ├── pmem.c                 # 物理内存管理
│   │   ├── vm.c                   # 虚拟内存
│   │   ├── smp.c                  # 多处理器
│   │   ├── cache.c                # 缓存系统
│   │   ├── storage.c              # 存储管理
│   │   ├── hwinfo.c               # 硬件信息
│   │   ├── simd.c                 # SIMD优化
│   │   ├── syshook.c              # 系统钩子
│   │   ├── vmware.c               # VMware支持
│   │   └── lightshell_cmds/       # Shell命令
│   │       ├── netget.c
│   │       └── fileops.c
│   ├── fs/                        # 文件系统
│   │   ├── vfs.c / vfs2.c        # VFS抽象层
│   │   ├── ramfs.c / ramfs_vfs.c # RAM文件系统
│   │   ├── devfs.c / devfs_vfs.c # 设备文件系统
│   │   └── btrfs.c                # Btrfs
│   ├── dev/                       # 设备驱动（合并自 drivers/）
│   │   ├── ahci.c                 # AHCI存储
│   │   ├── nvme.c                 # NVMe存储
│   │   ├── block.c                # 块设备接口
│   │   ├── ps2kbd.c               # PS/2键盘
│   │   ├── tty.c                  # TTY终端
│   │   ├── serial.c               # 串口
│   │   ├── ramdisk.c              # RAM磁盘
│   │   └── rtc.c                  # 实时时钟
│   ├── ipc/                       # 进程间通信
│   │   ├── pipe.c                 # 管道实现
│   │   └── pipe_vfs.c             # 管道VFS适配
│   └── net/                       # 网络协议栈
│       ├── net.c                  # 网络核心
│       ├── virtionet.c            # VirtIO-Net
│       ├── wifi.c                 # WiFi
│       ├── dhcp/dhcp.c            # DHCP客户端
│       ├── dns/dns.c              # DNS解析
│       └── http/http.c            # HTTP客户端
│
├── drivers/                       # 硬件驱动（显示/GPU）
│   ├── display.c / display.h     # 显示驱动
│   ├── fb.c / fb.h               # Framebuffer
│   ├── font.c / font.h           # 字体渲染
│   ├── gpu_hal.c / gpu_hal.h     # GPU HAL
│   ├── render.c / render.h       # 渲染器
│   ├── shader.c / shader.h       # 着色器
│   ├── vulkan.c / vulkan.h       # Vulkan
│   └── im.c / im.h               # 输入管理器
│
├── user/                          # 用户态程序源码
│   ├── shell.c                    # Shell
│   ├── command.c / command.h     # 命令框架
│   ├── cmd_*.c                    # 各命令实现
│   ├── libc.c / libc.h           # 标准C库
│   ├── init.c                     # init进程
│   ├── daemon.c / daemon.h       # 守护进程框架
│   ├── service.c / service.h     # 服务管理
│   ├── syslogd.c / dhcpd.c       # 系统服务
│   ├── sysinfo.c                  # 系统信息工具
│   ├── rust_runtime.c / python_runtime.c / cpp_runtime.c
│   └── ...
│
├── include/kernel/                # 内核头文件
│   ├── kmalloc.h, sched.h, proc.h, ...
│   ├── vfs2.h, btrfs.h, ...
│   ├── apic.h, ioapic.h, ...
│   └── ...
│
├── docs/                          # 文档
│   ├── README.md                  # 文档首页（英/中/日）
│   ├── CHANGELOG.md              # 更新日志
│   ├── TODO.md                   # 开发路线图
│   ├── PROGRESS.md               # 项目进展
│   ├── SECURITY_STATUS.md        # 安全状态
│   ├── OPTIMIZATION_REPORT.md    # 优化报告
│   ├── I386_PORTING_PLAN.md      # i386移植计划
│   ├── build/build.md            # 构建指南
│   ├── user-guide/               # 用户指南
│   ├── developer-guide/          # 开发者指南
│   ├── api-reference/            # API参考
│   └── runtime/                  # 运行时文档
│
├── scripts/                       # 构建脚本
│   ├── run-qemu.sh
│   ├── run-qemu-kernel.sh
│   ├── make-iso.sh
│   ├── build-i386.sh
│   ├── create-disk-img.py
│   └── create-efi-img.py
│
├── tests/                         # 测试
│   ├── test_ramfs.c
│   ├── test_benchmark.c
│   └── test_extended.c
│
├── config/                        # 配置文件
│   └── userspace/example.pf
│
├── build/                         # 构建输出
├── CMakeLists.txt                 # 顶层构建配置
├── Makefile                       # 构建入口（CMake包装）
├── README.md                      # 项目介绍
├── AGENTS.md                      # 开发规范（禁止修改）
├── LICENSE                        # GPL v2
├── File_Struct_Define             # 文件系统结构定义
└── CONTRIBUTING_zh_CN.md          # 贡献指南
```
