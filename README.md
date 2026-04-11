# BrightS - Unix-like Operating System

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
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

# Build tests | 构建测试
make test
```

## 📚 Documentation | 文档

- **[📖 Complete Documentation](docs/README.md)** - Full documentation index
- **[📋 Command Reference](docs/user-guide/COMMAND_REFERENCE.md)** - All available commands
- **[🛠️ Developer Guide](docs/developer-guide/README.md)** - Development and contribution guide

## 🏗️ Build Requirements | 构建要求

- GCC/Clang compiler
- NASM assembler
- Make build system
- QEMU (for testing)

## 🤝 Related Projects | 相关项目

- **[BrightS Package Manager](https://github.com/s12mcOvO/BrightS_Package_Manager)** - Package management system
- **[SuperFetch](https://github.com/s12mcOvO/SuperFetch)** - System information tool

## 🤝 Contributing | 贡献

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

## 📄 License | 许可证

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

---

**BrightS** - A complete Unix-like operating system 🚀