# BrightS 文档 | BrightS Documentation

欢迎来到 BrightS 操作系统文档！

## 📚 文档结构

```
docs/
├── user-guide/           # 🚀 用户指南
│   ├── COMMAND_REFERENCE.md      # 命令参考手册（英文）
│   ├── COMMAND_REFERENCE_ja.md   # 命令参考手册（日语）
│   └── COMMAND_REFERENCE_CN.md   # 命令参考手册（中文）
├── developer-guide/     # 🛠️ 开发者指南
│   ├── README.md              # 开发者指南（英文）
│   ├── README_ja.md          # 开发者指南（日语）
│   ├── DEVELOPMENT.md        # 开发环境设置
│   ├── PERFORMANCE.md         # 性能优化
│   ├── PROJECT_STRUCTURE.md   # 项目架构
│   ├── CONTRIBUTING.md        # 贡献指南（英文）
│   ├── CONTRIBUTING_ja.md    # 贡献指南（日语）
│   └── CONTRIBUTING_zh_CN.md # 贡献指南（中文）
├── api-reference/       # 📋 API参考
│   └── API.md                    # 内核API文档
├── runtime/            # 🎯 运行时文档
│   └── README.md                # 多语言运行时
└── README.md           # 📖 文档首页（本文件）
```

## 🚀 快速开始

### 面向用户
- **[📖 命令参考](user-guide/COMMAND_REFERENCE.md)** - 完整命令参考（英文）
- **[📖 命令参考](user-guide/COMMAND_REFERENCE_ja.md)** - 完整命令列表（日语）
- **[📖 命令参考](user-guide/COMMAND_REFERENCE_CN.md)** - 完整命令列表（中文）
- **[🏠 主 README](../README.md)** - 项目概览和设置

### 面向开发者
- **[🛠️ 开发环境](developer-guide/DEVELOPMENT.md)** - 环境设置
- **[🏗️ 项目结构](developer-guide/PROJECT_STRUCTURE.md)** - 代码架构
- **[⚡ 性能指南](developer-guide/PERFORMANCE.md)** - 优化技巧

## 📖 用户指南

### 命令与 Shell
- **[📚 完整命令参考](user-guide/COMMAND_REFERENCE.md)**（英文）
- **[📚 完整命令列表](user-guide/COMMAND_REFERENCE_ja.md)**（日语）
- **[📚 完整命令列表](user-guide/COMMAND_REFERENCE_CN.md)**（中文）

### 高级功能
- **管道与重定向**: `cmd1 | cmd2`, `cmd > file`, `cmd < input`
- **后台作业**: `command &`, `jobs` 命令
- **命令历史**: 方向键导航
- **自动补全**: Tab 键补全
- **多语言**: `rust "code"`, `python "code"`, `cpp "code"`

## 🛠️ 开发者指南

### 开始开发
- **[🛠️ 开发环境](developer-guide/DEVELOPMENT.md)**
  - 所需工具和依赖
  - 构建系统设置
  - 测试环境

### 架构与设计
- **[🏗️ 项目结构](developer-guide/PROJECT_STRUCTURE.md)**
  - 内核架构概览
  - 代码组织
  - 模块结构

### 性能与优化
- **[⚡ 性能指南](developer-guide/PERFORMANCE.md)**
  - 基准测试结果
  - 优化技术
  - 内存管理

### 贡献代码
- **[🤝 贡献指南](developer-guide/CONTRIBUTING.md)**（英文）
- **[🤝 贡献指南](developer-guide/CONTRIBUTING_ja.md)**（日语）
- **[🤝 贡献指南](developer-guide/CONTRIBUTING_zh_CN.md)**（中文）

## 📋 API参考

### 内核API
- **[📖 完整API参考](api-reference/API.md)**
  - 内存管理函数
  - 文件系统操作
  - 字符串工具
  - 网络函数

### 系统调用
所有可用的系统调用及其参数和返回值都在上面的命令参考中。

## 🎯 运行时文档

多语言支持和运行时系统：

- **[🌍 运行时指南](runtime/README.md)**
  - Rust 运行时集成
  - Python 解释器
  - C++ 编译支持

## 📊 其他资源

- **[📝 变更日志](CHANGELOG.md)** - 版本历史
- **[🔧 构建指南](build/build.md)** - 详细构建说明
- **[🎯 TODO 列表](TODO.md)** - 开发路线图

## 🧪 测试

- `test_ramfs` - RAM 文件系统测试（7 个测试用例）
- `test_benchmark` - 性能基准测试（字符串、文件、内存操作）

## 🌍 语言支持

本文档提供以下语言版本：
- **English**（英文）（主要语言）
- **日本語**（日语）
- **简体中文**（简体中文）

## 📞 支持与社区

- **🐛 Issues**: [GitHub Issues](https://github.com/OpenLight-Studio/BrightS/issues)
- **💬 Discussions**: [GitHub Discussions](https://github.com/OpenLight-Studio/BrightS/discussions)
- **📖 文档问题**: [在此报告](https://github.com/OpenLight-Studio/BrightS/issues/new?labels=documentation)

## 📝 贡献文档

我们欢迎文档改进！详见[贡献指南](developer-guide/CONTRIBUTING.md)。

## 📄 许可证

本项目采用 GNU GPL v2 许可证。详见 [LICENSE](../LICENSE)。

---

*最后更新：2026-04-17*
