# BrightS User Guide | BrightS 用户指南

Welcome to the BrightS User Guide! This section contains everything you need to know about using the BrightS operating system.

## 📖 Contents | 目录

### Getting Started | 入门指南
- [Command Reference](COMMAND_REFERENCE.md) - Complete list of all BrightS commands
- [Command Reference (中文)](COMMAND_REFERENCE_CN.md) - 中文版完整命令列表

### Basic Usage | 基本使用
- **Shell Operations**: Navigation, file management, process control
- **File System**: Working with files and directories
- **Package Management**: Installing and managing software with BSPM

### Advanced Features | 高级功能
- **Pipes & Redirection**: `command1 | command2`, `command > file`
- **Background Jobs**: Running commands in background with `&`
- **Command History**: Using arrow keys to navigate command history
- **Auto-completion**: Tab completion for commands and files
- **Multi-language Support**: Running code in Rust, Python, and C++

## 🚀 Quick Start Commands | 快速开始命令

```bash
# Basic navigation | 基本导航
pwd              # Show current directory | 显示当前目录
ls               # List files | 列出文件
cd /bin          # Change directory | 更改目录

# File operations | 文件操作
cat file.txt     # Display file contents | 显示文件内容
cp src dest      # Copy files | 复制文件
mv old new       # Move/rename files | 移动/重命名文件
rm file          # Remove files | 删除文件

# System information | 系统信息
ps               # Show processes | 显示进程
jobs             # Show background jobs | 显示后台作业

# Package management | 包管理
bspm update      # Update package lists | 更新包列表
bspm install pkg # Install package | 安装包
bspm search term # Search packages | 搜索包

# Multi-language | 多语言支持
rust 'println!("Hello!");'     # Run Rust code | 运行Rust代码
python 'print("Hello!")'       # Run Python code | 运行Python代码
cpp 'cout << "Hello!";'        # Run C++ code | 运行C++代码
```

## 🎯 Key Features | 主要特性

### Unified Command System | 统一命令系统
BrightS uses a unified command framework where all commands follow consistent patterns and provide help through the `help` command.

### Advanced Shell | 高级Shell
- **Command History**: Use ↑/↓ arrows to navigate through command history
- **Tab Completion**: Press Tab to auto-complete commands and filenames
- **Pipes**: Chain commands together: `ls | grep "\.c$"`
- **Redirection**: Redirect input/output: `command > output.txt`
- **Background Jobs**: Run commands in background: `long_command &`

### Package Management | 包管理
BrightS includes BSPM (BrightS Package Manager) for software installation and management:

- Secure package installation with SHA256 verification
- Dependency resolution
- Repository management
- Automatic updates

### Multi-language Runtime | 多语言运行时
BrightS natively supports running code in multiple programming languages:

- **Rust**: Full Rust compiler integration
- **Python**: Embedded Python interpreter
- **C++**: C++ compilation and execution

## 📚 Learning Resources | 学习资源

- [Command Reference](COMMAND_REFERENCE.md) - Detailed command documentation
- [BSPM Documentation](../../BrightS_Package_Manager/README.md) - Package manager guide
- [Runtime Documentation](../runtime/README.md) - Multi-language support

## ❓ Getting Help | 获取帮助

```bash
help             # General help | 通用帮助
help command     # Help for specific command | 特定命令帮助
bspm --help      # BSPM help | BSPM帮助
```

## 🆘 Troubleshooting | 故障排除

### Common Issues | 常见问题

1. **Command not found**: Ensure BSPM packages are properly installed
2. **Permission denied**: Use appropriate user permissions or run as administrator
3. **Network issues**: Check network connectivity for package downloads

### Support | 支持
- Check the [troubleshooting guide](../developer-guide/TROUBLESHOOTING.md)
- Report issues on [GitHub Issues](https://github.com/OpenLight-Studio/BrightS/issues)

---

*This user guide covers BrightS version 0.1.2.2* | *本用户指南涵盖 BrightS 版本 0.1.2.2*