# 项目目录结构 | Project Structure

```
BrightS/
├── arch/                          # 架构相关
│   ├── x86_64/                    # x86_64 (UEFI)
│   │   ├── uefi_entry.c           # UEFI入口
│   │   ├── idt.c/h                # 中断描述符表
│   │   ├── gdt.c/h                # 全局描述符表
│   │   ├── trap.c/h               # 异常处理
│   │   ├── isr.S                  # 中断服务程序
│   │   ├── paging.c/h             # 4级分页
│   │   ├── apic.c / ioapic.c      # APIC
│   │   ├── hpet.c                 # 高精度定时器
│   │   ├── syscall_entry.S        # 系统调用入口
│   │   ├── syscall_abi.c          # 系统调用ABI
│   │   ├── user_enter.S           # 用户模式进入
│   │   ├── user_stub.S            # 用户态stub
│   │   ├── cpu_local.c            # 每CPU变量
│   │   ├── pci.c                  # PCI配置
│   │   ├── mtrr.c                 # MTRR
│   │   ├── pic_pit.c              # PIC+PIT
│   │   └── uefi_memmap.c          # UEFI内存映射
│   └── i386/                      # i386 (BIOS)
│       ├── boot/
│       │   ├── bootloader.asm     # BIOS引导
│       │   └── test_boot.asm      # 测试引导
│       ├── entry.asm / entry.c    # 内核入口
│       ├── gdt.c / idt.c
│       ├── paging.c/h             # PAE分页
│       ├── pic_pit.c / pit.h
│       ├── isr.asm
│       ├── syscall_abi.c
│       ├── trap.c
│       └── link.ld                # 链接脚本
│
├── kernel/                        # 内核核心
│   ├── core/                      # 核心子系统
│   │   ├── kernel_main.c          # 内核主入口
│   │   ├── kmalloc.c              # Slab分配器
│   │   ├── sched.c                # 进程调度
│   │   ├── proc.c                 # 进程管理
│   │   ├── syscall.c              # 系统调用
│   │   ├── sysent.c               # 系统调用表
│   │   ├── syscalls_extended.c    # 扩展系统调用
│   │   ├── signal.c               # 信号
│   │   ├── sleep.c                # 睡眠/定时
│   │   ├── clock.c                # 时钟
│   │   ├── acpi.c                 # ACPI
│   │   ├── runtime.c              # 运行时优化
│   │   ├── elf.c                  # ELF加载器
│   │   ├── userinit.c             # 用户进程初始化
│   │   ├── lightshell.c           # 内建Shell
│   │   ├── printf.c               # 格式化输出
│   │   ├── pf.c                   # 页面错误
│   │   ├── pmem.c                 # 物理内存
│   │   ├── vm.c                   # 虚拟内存
│   │   ├── smp.c                  # 多处理器
│   │   ├── cache.c                # 缓存
│   │   ├── storage.c              # 存储
│   │   ├── hwinfo.c               # 硬件信息
│   │   ├── simd.c                 # SIMD
│   │   ├── syshook.c              # 系统钩子
│   │   ├── vmware.c               # VMware
│   │   └── lightshell_cmds/       # Shell命令
│   │       ├── netget.c
│   │       └── fileops.c
│   ├── fs/                        # 文件系统
│   │   ├── vfs.c / vfs2.c         # VFS抽象层
│   │   ├── ramfs.c / ramfs_vfs.c  # RAMFS
│   │   ├── devfs.c / devfs_vfs.c  # 设备文件系统
│   │   └── btrfs.c                # Btrfs
│   ├── dev/                       # 设备驱动
│   │   ├── ahci.c                 # AHCI
│   │   ├── nvme.c                 # NVMe
│   │   ├── block.c                # 块设备
│   │   ├── ps2kbd.c               # PS/2键盘
│   │   ├── tty.c                  # TTY
│   │   ├── serial.c               # 串口
│   │   ├── ramdisk.c              # RAM磁盘
│   │   └── rtc.c                  # 实时时钟
│   ├── ipc/                       # 进程间通信
│   │   ├── pipe.c                 # 管道
│   │   └── pipe_vfs.c             # 管道VFS
│   └── net/                       # 网络
│       ├── net.c                  # 网络核心
│       ├── virtionet.c            # VirtIO-Net
│       ├── wifi.c                 # WiFi
│       ├── dhcp/dhcp.c            # DHCP
│       ├── dns/dns.c              # DNS
│       └── http/http.c            # HTTP
│
├── drivers/                       # 显示/GPU驱动
│   ├── display.c/h                # 显示驱动
│   ├── fb.c/h                     # Framebuffer
│   ├── font.c/h                   # 字体
│   ├── gpu_hal.c/h                # GPU HAL
│   ├── render.c/h                 # 渲染器
│   ├── shader.c/h                 # 着色器
│   ├── vulkan.c/h                 # Vulkan
│   └── im.c/h                     # 输入管理
│
├── user/                          # 用户态程序
│   ├── shell.c                    # Shell
│   ├── command.c/h                # 命令框架
│   ├── cmd_*.c                    # 命令实现
│   ├── libc.c/h                   # 标准C库
│   ├── init.c                     # init进程
│   ├── daemon.c/h                 # 守护进程
│   ├── service.c/h                # 服务管理
│   ├── syslogd.c / dhcpd.c        # 系统服务
│   ├── sysinfo.c                  # 系统信息
│   └── {rust,python,cpp}_runtime.c # 语言运行时
│
├── include/kernel/                # 内核头文件
│   ├── kmalloc.h, sched.h, proc.h ...
│   ├── vfs2.h, btrfs.h ...
│   ├── apic.h, ioapic.h ...
│   └── ...
│
├── docs/                          # 文档
│   ├── README.md                  # 文档首页
│   ├── CHANGELOG.md               # 更新日志
│   ├── TODO.md                    # 路线图
│   ├── PROGRESS.md                # 进展
│   ├── SECURITY_STATUS.md         # 安全状态
│   ├── OPTIMIZATION_REPORT.md     # 优化报告
│   ├── I386_PORTING_PLAN.md       # i386移植
│   ├── build/build.md             # 构建指南
│   ├── user-guide/                # 用户指南
│   ├── developer-guide/           # 开发者指南
│   ├── api-reference/             # API参考
│   └── runtime/                   # 运行时文档
│
├── scripts/                       # 构建脚本
│   ├── run-qemu.sh / run-qemu-kernel.sh
│   ├── make-iso.sh / build-i386.sh
│   ├── create-disk-img.py
│   └── create-efi-img.py
│
├── tests/                         # 测试
│   ├── test_ramfs.c
│   ├── test_benchmark.c
│   └── test_extended.c
│
├── config/                        # 配置
│   └── userspace/example.pf
│
├── build/                         # 构建输出
├── CMakeLists.txt                 # 顶层CMake
├── Makefile                       # 构建入口
├── README.md                      # 项目介绍
├── AGENTS.md                      # 开发规范(禁止修改)
├── LICENSE                        # GPL v2
├── File_Struct_Define             # 文件系统结构定义
└── CONTRIBUTING_zh_CN.md          # 贡献指南
```
