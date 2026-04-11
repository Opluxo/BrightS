# BrightS - Unix-like Operating System

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.2.2-green.svg)]()
[![Documentation](https://img.shields.io/badge/docs-available-brightgreen.svg)](docs/README.md)

BrightS is a complete Unix-like operating system kernel with advanced features including multi-language runtime support, unified command framework, and comprehensive user-space environment.

## ✨ Features | 特性

- **UEFI Boot Support**: Modern bootloader with secure boot
- **Multi-language Runtime**: Native Rust, Python, and C++ execution
- **Unified Command System**: Consistent command framework across all tools
- **Advanced Shell**: Command history, auto-completion, pipes, and redirection
- **Package Management**: BSPM package manager with dependency resolution
- **System Information**: Fast system information display tool

## 🚀 Quick Start | 快速开始

```bash
# Build the kernel | 构建内核
make all

# Run in QEMU | 在QEMU中运行
make run

# Debug build | 调试构建
make debug
make run-debug

# Run tests | 运行测试
make test

# Clean build | 清理构建
make clean

# Get build info | 获取构建信息
make info
```

## 📚 Documentation | 文档

- **[📖 Complete Documentation](docs/README.md)** - Full documentation index
- **[📋 Command Reference](docs/user-guide/COMMAND_REFERENCE.md)** - All available commands
- **[🛠️ Developer Guide](docs/developer-guide/README.md)** - Development and contribution guide

## 🏗️ Build Requirements | 构建要求

- **GCC/Clang compiler** (C compiler)
- **NASM assembler** (for x86 assembly)
- **CMake** (build system generator)
- **Make** (build automation)
- **QEMU** (for testing and emulation)
- **lld** (LLVM linker, optional but recommended)

### Build Options | 构建选项

```bash
# Custom build directory
make BUILD_DIR=mybuild all

# Debug build with symbols
make BUILD_TYPE=Debug all

# Release optimized build
make BUILD_TYPE=Release all

# Parallel build with custom job count
make -j4 all
```

### Supported Platforms | 支持平台

- Linux (primary development platform)
- macOS (with Homebrew dependencies)
- Windows (with WSL or MSYS2)

## 🤝 Related Projects | 相关项目

- **[BrightS Package Manager](https://github.com/s12mcOvO/BrightS_Package_Manager)** - Package management system
- **[SuperFetch](https://github.com/s12mcOvO/SuperFetch)** - System information tool

## 🤝 Contributing | 贡献

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

## 📄 License | 许可证

This project is licensed under the GNU General Public License v2.0 with a special exception for userspace programs. See [LICENSE](LICENSE) for details.

---

**BrightS** - A complete Unix-like operating system 🚀