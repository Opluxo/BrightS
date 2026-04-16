# BrightS Documentation | BrightS 文档 | BrightS ドキュメント

Welcome to the BrightS operating system documentation! This comprehensive documentation is organized into clear sections for users and developers.
BrightSオペレーティングシステムのドキュメントへようこそ！

## 📚 Documentation Structure | 文档结构 | ドキュメント構造

```
docs/
├── user-guide/           # 🚀 用户指南 | ユーザーガイド
│   ├── COMMAND_REFERENCE.md      # Command Reference (English)
│   ├── COMMAND_REFERENCE_ja.md   # コマンドリファレンス (日本語)
│   └── COMMAND_REFERENCE_CN.md   # 命令参考手册 (中文)
├── developer-guide/     # 🛠️ 开发者指南 | 開発者ガイド
│   ├── README.md              # Developer Guide (English)
│   ├── README_ja.md           # 開発者ガイド (日本語)
│   ├── README_zh_CN.md        # 开发者指南 (中文)
│   ├── DEVELOPMENT.md          # Development Setup
│   ├── CONTRIBUTING.md         # Contributing Guide (English)
│   ├── CONTRIBUTING_ja.md      # 貢献ガイド (日本語)
│   ├── CONTRIBUTING_zh_CN.md   # 贡献指南 (中文)
│   ├── PROJECT_STRUCTURE.md     # Project Architecture
│   └── PERFORMANCE.md          # Performance Optimization
├── api-reference/       # 📋 API Reference | API参考 | APIリファレンス
│   └── API.md                    # Kernel API Documentation
├── runtime/            # 🎯 Runtime Documentation | 运行时文档 | ランタイムドキュメント
│   └── README.md                # Multi-language Runtime
└── README.md           # 📖 Documentation Home (This file) | 文档首页
```

## 🚀 Quick Start | 快速开始

### For Users | 面向用户
- **[📖 Command Reference](user-guide/COMMAND_REFERENCE.md)** - Complete command reference
- **[📖 命令参考手册](user-guide/COMMAND_REFERENCE_CN.md)** - 中文命令参考
- **[🏠 Main README](../README.md)** - Project overview and setup

### For Developers | 面向开发者
- **[🛠️ Development Setup](developer-guide/DEVELOPMENT.md)** - Environment setup
- **[🏗️ Project Structure](developer-guide/PROJECT_STRUCTURE.md)** - Code architecture
- **[⚡ Performance Guide](developer-guide/PERFORMANCE.md)** - Optimization tips

## 📖 User Guide | 用户指南

Everything you need to know about using BrightS:

### Commands & Shell | 命令与Shell
- **[📚 Complete Command Reference](user-guide/COMMAND_REFERENCE.md)**
  - File operations: `ls`, `cp`, `mv`, `rm`, `mkdir`, `chmod`
  - Text processing: `echo`, `grep`, `find`
  - System info: `pwd`, `ps`, `jobs`
  - Network: `ping`
  - Package management: `bspm install`, `bspm search`, etc.
- **[📚 完整命令参考](user-guide/COMMAND_REFERENCE_CN.md)** - 中文版

### Advanced Features | 高级功能
- **Pipes & Redirection**: `cmd1 | cmd2`, `cmd > file`, `cmd < input`
- **Background Jobs**: `command &`, `jobs` command
- **Command History**: Arrow keys navigation
- **Auto-completion**: Tab key completion
- **Multi-language**: `rust "code"`, `python "code"`, `cpp "code"`

## 🛠️ Developer Guide | 开发者指南

### Getting Started | 开始开发
- **[🛠️ Development Environment](developer-guide/DEVELOPMENT.md)**
  - Required tools and dependencies
  - Build system setup
  - Testing environment

### Architecture & Design | 架构与设计
- **[🏗️ Project Structure](developer-guide/PROJECT_STRUCTURE.md)**
  - Kernel architecture overview
  - Code organization
  - Module structure

### Performance & Optimization | 性能与优化
- **[⚡ Performance Guide](developer-guide/PERFORMANCE.md)**
  - Benchmarking results
  - Optimization techniques
  - Memory management

### Contributing | 贡献代码
- **[🤝 Contributing Guide](developer-guide/CONTRIBUTING.md)** - English
- **[🤝 贡献指南](developer-guide/CONTRIBUTING_zh_CN.md)** - 中文

## 📋 API Reference | API参考

Technical documentation for BrightS internals:

### Kernel APIs | 内核API
- **[📖 Complete API Reference](api-reference/API.md)**
  - Memory management functions
  - Filesystem operations
  - String utilities
  - Network functions

### System Calls | 系统调用
All available system calls with parameters and return values are documented in the command references above.

## 🎯 Runtime Documentation | 运行时文档

Multi-language support and runtime systems:

- **[🌍 Runtime Guide](runtime/README.md)**
  - Rust runtime integration
  - Python interpreter
  - C++ compilation support

## 📊 Additional Resources | 其他资源

- **[📝 Changelog](CHANGELOG.md)** - Version history
- **[🔧 Build Guide](build/build.md)** - Detailed build instructions
- **[🎯 TODO List](TODO)** - Development roadmap

## 🧪 Testing | 测试

- `test_ramfs` - RAM filesystem tests (7 test cases)
- `test_benchmark` - Performance benchmarks (string, file, memory operations)

## 🌍 Languages | 语言支持

This documentation is available in:
- **English** (primary language)
- **简体中文** (Simplified Chinese)

## 📞 Support & Community | 支持与社区

- **🐛 Issues**: [GitHub Issues](https://github.com/OpenLight-Studio/BrightS/issues)
- **💬 Discussions**: [GitHub Discussions](https://github.com/OpenLight-Studio/BrightS/discussions)
- **📖 Documentation Issues**: Report docs problems [here](https://github.com/OpenLight-Studio/BrightS/issues/new?labels=documentation)

## 📝 Contributing to Docs | 文档贡献

We welcome documentation improvements! See our [Contributing Guide](developer-guide/CONTRIBUTING.md) for details.

## 📄 License | 许可证

This project is licensed under the GNU GPL v2 License. See [LICENSE](../LICENSE) for details.

---

*Last updated: 2026-04-11* | *最后更新：2026年4月11日*</content>
<parameter name="filePath">docs/README.md