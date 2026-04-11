# BrightS System Suite

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/version-0.1.2.2-green.svg)]()
[![Documentation](https://img.shields.io/badge/docs-available-brightgreen.svg)](docs/README.md)

BrightS System Suite 是一个完整的类Unix操作系统生态系统，包含内核、包管理系统和系统优化工具。

## 📁 项目结构

```
BrightS_System_Suite/
├── BrightS/                    # 🚀 核心操作系统内核
├── BrightS_Package_Manager/    # 📦 包管理系统 (BSPM)
└── SuperFetch/                 # ⚡ 系统优化工具
```

## 🚀 项目概述

### BrightS - 操作系统内核

完整的类Unix操作系统内核，提供：
- UEFI引导支持
- 多语言运行时 (Rust/Python/C++)
- 统一命令框架
- 高级Shell功能
- 完整的用户空间环境

**📍 位置**: `BrightS/`
**📚 文档**: [BrightS README](BrightS/README.md)

### BrightS Package Manager (BSPM)

专业的包管理系统，提供：
- .bsp包格式支持
- SHA256校验和验证
- 依赖关系解析
- 仓库管理和分发
- 自动构建工具

**📍 位置**: `BrightS_Package_Manager/`
**📚 文档**: [BSPM README](BrightS_Package_Manager/README.md)

### SuperFetch - 系统优化工具

系统性能优化和预取工具：
- 智能预取算法
- 系统性能监控
- 缓存优化
- 启动时间优化

**📍 位置**: `SuperFetch/`
**📚 文档**: [SuperFetch README](SuperFetch/README.md)

## 🏗️ 构建和安装

### 系统要求

- **操作系统**: Linux/macOS/Windows (WSL)
- **编译器**: GCC/Clang, NASM
- **工具**: Make, CMake, Git
- **可选**: QEMU (用于测试)

### 快速开始

```bash
# 克隆整个套件
git clone https://github.com/OpenLight-Studio/BrightS.git
cd BrightS_System_Suite

# 构建内核
cd BrightS
make all

# 构建包管理器
cd ../BrightS_Package_Manager
./build-package.sh packages/hello-1.0.0

# 测试系统
cd ../BrightS
make run
```

## 📚 文档导航

### 📖 核心文档

| 文档 | 描述 | 位置 |
|------|------|------|
| [系统参考手册](BrightS/docs/user-guide/COMMAND_REFERENCE.md) | 完整的命令和API参考 | `BrightS/docs/user-guide/COMMAND_REFERENCE.md` |
| [系统参考手册 (中文)](BrightS/docs/user-guide/COMMAND_REFERENCE_CN.md) | 中文版命令参考 | `BrightS/docs/user-guide/COMMAND_REFERENCE_CN.md` |
| [包格式规范](BrightS_Package_Manager/PACKAGE_FORMAT.md) | BSPM包格式标准 | `BrightS_Package_Manager/PACKAGE_FORMAT.md` |
| [项目架构](BrightS/docs/developer-guide/PROJECT_STRUCTURE.md) | 内核架构说明 | `BrightS/docs/developer-guide/PROJECT_STRUCTURE.md` |
| [开发指南](BrightS/docs/developer-guide/DEVELOPMENT.md) | 开发环境设置 | `BrightS/docs/developer-guide/DEVELOPMENT.md` |

### 🛠️ 开发文档

- [贡献指南](BrightS/CONTRIBUTING.md) - 如何贡献代码
- [API文档](BrightS/docs/API.md) - 内核API参考
- [性能优化](BrightS/docs/PERFORMANCE.md) - 性能调优指南
- [变更日志](BrightS/docs/CHANGELOG.md) - 版本更新历史

## 🏃‍♂️ 快速测试

```bash
# 下载并运行
cd BrightS
make run

# 在Shell中测试
ls
echo "Hello BrightS!"
bspm list
```

## 🤝 贡献

我们欢迎各种形式的贡献！

1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

详细贡献指南请见 [CONTRIBUTING.md](BrightS/CONTRIBUTING.md)

## 📄 许可证

本项目基于 [GNU General Public License v3.0](LICENSE) 开源。

## 🌟 特性亮点

### ✨ 内核特性
- **多核支持**: SMP架构，充分利用多核CPU
- **内存管理**: 完整的虚拟内存系统
- **文件系统**: VFS层，支持多种文件系统
- **网络栈**: 内置网络协议支持
- **设备驱动**: 丰富的外设驱动支持

### 🎯 包管理特性
- **安全验证**: SHA256校验和确保包完整性
- **依赖解析**: 自动处理包依赖关系
- **仓库系统**: 支持多个包仓库源
- **构建工具**: 自动化包构建流程

### 🔧 开发特性
- **多语言支持**: 原生支持Rust/Python/C++
- **统一框架**: 一致的命令和API设计
- **文档完善**: 详细的中英文文档
- **测试覆盖**: 完整的测试套件

## 📞 联系我们

- **项目主页**: https://github.com/OpenLight-Studio/BrightS
- **问题反馈**: [GitHub Issues](https://github.com/OpenLight-Studio/BrightS/issues)
- **讨论交流**: [GitHub Discussions](https://github.com/OpenLight-Studio/BrightS/discussions)

## 🙏 致谢

感谢所有为BrightS项目做出贡献的开发者！

---

**BrightS System Suite** - 打造下一代操作系统生态系统 🚀