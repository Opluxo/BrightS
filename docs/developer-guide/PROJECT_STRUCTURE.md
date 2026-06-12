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
│   │   ├── boot_splash.c          # 启动画面
│   │   └── lightshell_cmds/       # Shell命令
│   │       ├── netget.c           # HTTP获取
│   │       └── fileops.c          # 文件操作
│   ├── fs/                        # 文件系统
│   │   ├── vfs2.c / vfs2.h        # VFS抽象层
│   │   ├── ramfs.c / ramfs_vfs.c  # RAMFS
│   │   ├── devfs.c / devfs_vfs.c  # 设备文件系统
│   │   ├── btrfs.c                # Btrfs
│   │   └── boot_fs.c              # 引导文件系统
│   ├── ipc/                       # 进程间通信
│   │   ├── pipe.c                 # 管道
│   │   └── pipe_vfs.c             # 管道VFS
│   └── net/                       # 网络
│       ├── net.c                  # 网络核心
│       ├── virtionet.c            # VirtIO-Net
│       ├── wifi.c                 # WiFi
│       ├── dhcp/
│       │   ├── dhcp.c             # DHCP客户端
│       │   └── dhcp.h
│       ├── dns/
│       │   ├── dns.c              # DNS解析器
│       │   └── dns.h
│       └── http/
│           ├── http.c             # HTTP客户端
│           └── http.h
│
├── drivers/                       # 设备驱动
│   ├── ahci.c / ahci.h           # AHCI
│   ├── nvme.c / nvme.h           # NVMe
│   ├── block.c / block.h         # 块设备
│   ├── ps2kbd.c / ps2kbd.h       # PS/2键盘
│   ├── tty.c / tty.h             # TTY
│   ├── serial.c / serial.h       # 串口
│   ├── ramdisk.c / ramdisk.h     # RAM磁盘
│   ├── rtc.c / rtc.h             # 实时时钟
│   ├── display.c / display.h     # 显示驱动
│   ├── fb.c / fb.h               # Framebuffer
│   ├── font.c / font.h           # 字体
│   ├── gpu_hal.c / gpu_hal.h     # GPU HAL
│   ├── render.c / render.h       # 渲染器
│   ├── shader.c / shader.h       # 着色器
│   ├── vulkan.c / vulkan.h       # Vulkan
│   ├── im.c / im.h               # 输入管理
│   ├── tui.c / tui.h             # 文本UI
│   ├── uhci.c / uhci.h           # USB UHCI
│   ├── usb_core.c                # USB核心
│   ├── usb_hid.c                 # USB HID
│   └── usb_msc.c                 # USB MSC
│
├── sys/                           # 用户态程序
│   ├── kernel/
│   │   └── CMakeLists.txt         # 内核CMake
│   └── user/
│       ├── shell.c                # Shell
│       ├── command.c / command.h  # 命令框架
│       ├── cmd_*.c                # 命令实现
│       ├── libc.c / libc.h       # 标准C库
│       ├── init.c                 # init进程
│       ├── daemon.c / daemon.h   # 守护进程
│       ├── service.c / service.h # 服务管理
│       ├── syslogd.c             # 系统日志
│       ├── dhcpd.c               # DHCP服务
│       ├── sysinfo.c             # 系统信息
│       ├── echo.c                # echo命令
│       ├── ping.c                # ping命令
│       ├── lang_runtime.c / .h   # 语言运行时
│       ├── rust_runtime.c / .h   # Rust运行时
│       ├── python_runtime.c / .h # Python运行时
│       └── cpp_runtime.c / .h    # C++运行时
│
├── include/kernel/                # 内核头文件
│   ├── kmalloc.h, sched.h, proc.h
│   ├── vfs2.h, btrfs.h
│   ├── apic.h, ioapic.h
│   ├── net.h, dhcp.h, dns.h, http.h
│   └── ...
│
├── scripts/                       # 构建脚本
│   ├── build-i386.sh              # i386构建
│   └── build-x86_64.sh            # x86_64构建
│
├── tools/                         # 工具
│   ├── create-disk-img.py         # 磁盘镜像
│   ├── create-efi-img.py          # EFI镜像
│   ├── make-iso.sh                # ISO创建
│   ├── run-qemu.sh                # QEMU运行
│   └── run-qemu-kernel.sh         # QEMU内核运行
│
├── tests/                         # 测试
│   ├── CMakeLists.txt
│   ├── test_ramfs.c
│   ├── test_benchmark.c
│   ├── test_extended.c
│   ├── test_kmalloc.c
│   ├── test_sched.c
│   └── test_stubs.c
│
├── config/                        # 配置
│   └── userspace/
│       └── example.pf
│
├── docs/                          # 文档（独立分支）
│   ├── index.html                 # 着陆页
│   ├── view.html                  # Markdown渲染器
│   ├── user-guide/                # 用户指南
│   ├── developer-guide/           # 开发者指南
│   ├── api-reference/             # API参考
│   ├── runtime/                   # 运行时文档
│   └── vmware/                    # VMware文档
│
├── CMakeLists.txt                 # 顶层CMake
├── Makefile                       # 构建入口
├── README.md                      # 项目介绍
├── AGENTS.md                      # 开发规范
├── LICENSE                        # GPL v2
├── LICENSE_COMPATIBILITY.md       # 许可证兼容性
└── File_Struct_Define             # 文件系统结构定义
```