# BrightS 项目结构 | Project Structure

## 目录布局 | Directory Layout

```
BrightS/
├── CMakeLists.txt         # 主CMake构建配置 | Main CMake build configuration
├── README.md              # 项目主要说明文档 | Project main readme
├── CONTRIBUTING.md        # 贡献指南 | Contributing guidelines
├── LICENSE                # 许可证文件 | License file
│
├── include/               # 公共头文件 | Public header files
│   └── kernel/           # 内核公共API头文件 | Kernel public API headers
│
├── sys/                  # 系统源代码 | System source code
│   └── kernel/           # 内核核心实现 | Kernel core implementation
│       ├── core/         # 核心内核函数 | Core kernel functions
│       │   ├── kmalloc.c/h    # 内存分配器 | Memory allocator
│       │   ├── kernel_util.h  # 内核工具函数 | Kernel utilities
│       │   ├── printf.c       # 格式化输出 | Formatted output
│       │   ├── runtime.c      # 运行时函数 | Runtime functions
│       │   └── *.c/h          # 其他核心组件 | Other core components
│       ├── fs/           # 文件系统实现 | Filesystem implementations
│       │   ├── ramfs.c/h      # RAM文件系统 | RAM filesystem
│       │   ├── btrfs.c        # Btrfs文件系统 | Btrfs filesystem
│       │   └── vfs*.c         # 虚拟文件系统 | Virtual filesystem
│       ├── dev/          # 设备驱动 | Device drivers
│       │   ├── block.c/h      # 块设备接口 | Block device interface
│       │   ├── ahci.c         # AHCI驱动 | AHCI driver
│       │   ├── nvme.c         # NVMe驱动 | NVMe driver
│       │   └── *.c            # 其他设备驱动 | Other device drivers
│       ├── ipc/          # 进程间通信 | Inter-process communication
│       │   ├── pipe.c/h       # 管道实现 | Pipe implementation
│       │   └── *.c/h          # 其他IPC机制 | Other IPC mechanisms
│       ├── net/          # 网络协议栈 | Network stack
│       │   ├── net.c          # 网络核心 | Network core
│       │   ├── dns/           # DNS客户端 | DNS client
│       │   ├── http/          # HTTP客户端 | HTTP client
│       │   └── */             # 其他网络组件 | Other network components
│       └── platform/     # 平台特定代码 | Platform specific code
│           └── x86_64/   # x86_64架构支持 | x86_64 architecture support
│
├── tests/                # 单元测试和集成测试 | Unit tests and integration tests
│   ├── test_ramfs.c      # RAMFS功能测试 | RAMFS functionality tests
│   ├── test_benchmark.c  # 性能基准测试 | Performance benchmark tests
│   └── CMakeLists.txt    # 测试构建配置 | Test build configuration
│
├── tools/                # 构建和工具脚本 | Build and utility scripts
│
├── docs/                 # 项目文档 | Project documentation
│   ├── README.md         # 文档索引 | Documentation index
│   ├── API.md            # API参考文档 | API reference documentation
│   ├── PERFORMANCE.md    # 性能指标 | Performance metrics
│   ├── PROJECT_STRUCTURE.md  # 项目结构（本文件） | Project structure (this file)
│   ├── DEVELOPMENT.md    # 开发环境设置 | Development environment setup
│   ├── CONTRIBUTING_zh_CN.md  # 中文贡献指南 | Chinese contributing guidelines
│   ├── TODO              # 开发任务列表 | Development task list
│   ├── build/            # 构建文档 | Build documentation
│   │   └── build.md      # 详细构建指南 | Detailed build guide
│   └── design/           # 设计文档 | Design documents
│       └── File_Struct_Define  # 文件系统结构定义 | Filesystem structure definition
│
├── build/                # 构建输出目录（不在git中） | Build output directory (not in git)
├── .github/              # GitHub Actions工作流 | GitHub Actions workflows
│   └── workflows/        # CI/CD配置 | CI/CD configuration
└── .kilo/                # Kilo工具配置 | Kilo tool configuration
```

## File Naming Conventions

- Header files: `.h`
- Source files: `.c`
- Assembly files: `.S`

## Coding Standards

- Use 4 spaces for indentation, no tabs
- All public functions prefixed with `brights_` or component specific prefix
- Static functions do not use prefixes
- Use snake_case for function and variable names
- Use UPPER_CASE for macros and constants
- Header files have proper include guards