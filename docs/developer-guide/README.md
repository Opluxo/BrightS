# BrightS Developer Guide | BrightS 开发者指南

Welcome to the BrightS Developer Guide! This section provides comprehensive information for developers who want to contribute to, modify, or extend the BrightS operating system.

## 🛠️ Contents | 目录

### Getting Started | 开始开发
- **[🛠️ Development Environment](DEVELOPMENT.md)** - Setting up your development environment
- **[🏗️ Project Structure](PROJECT_STRUCTURE.md)** - Understanding the codebase architecture
- **[⚡ Performance Guide](PERFORMANCE.md)** - Performance optimization and benchmarking

### Contributing | 贡献代码
- **[🤝 Contributing Guide](CONTRIBUTING.md)** - How to contribute (English)
- **[🤝 贡献指南](CONTRIBUTING_zh_CN.md)** - 贡献指南 (中文)

### Development Workflow | 开发工作流
- Code standards and conventions
- Testing procedures
- Code review process
- Release management

## 🚀 Development Setup | 开发环境设置

### Prerequisites | 先决条件

```bash
# Required tools | 必需工具
sudo apt install build-essential nasm qemu-system-x86 \
                 git cmake clang lld llvm

# Optional tools | 可选工具
sudo apt install gdb valgrind clang-format
```

### Building BrightS | 构建 BrightS

```bash
# Clone and build | 克隆并构建
git clone https://github.com/OpenLight-Studio/BrightS.git
cd BrightS
make all

# Run in QEMU | 在QEMU中运行
make run

# Debug build | 调试构建
make debug
make run-debug
```

### Development Workflow | 开发工作流

1. **Fork the repository** | Fork仓库
2. **Create a feature branch** | 创建功能分支
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes** | 进行修改
4. **Test your changes** | 测试修改
   ```bash
   make test
   make run
   ```
5. **Run benchmarks** | 运行基准测试
   ```bash
   make benchmark
   ```
6. **Submit a pull request** | 提交拉取请求

## 🏗️ Architecture Overview | 架构概览

### Kernel Structure | 内核结构

```
kernel/
├── arch/          # Architecture-specific code | 架构特定代码
├── core/          # Core kernel components | 核心内核组件
├── drivers/       # Device drivers | 设备驱动
├── fs/           # Filesystem implementations | 文件系统实现
├── lib/          # Kernel libraries | 内核库
├── mm/           # Memory management | 内存管理
└── net/          # Networking stack | 网络栈
```

### User Space | 用户空间

```
user/
├── libc/         # Standard C library | 标准C库
├── shell/        # Command shell | 命令Shell
├── commands/     # Built-in commands | 内置命令
└── runtimes/     # Language runtimes | 语言运行时
```

### Key Components | 关键组件

- **UEFI Bootloader**: Handles system initialization
- **Kernel Core**: Process scheduling, memory management
- **VFS Layer**: Unified filesystem interface
- **Command Framework**: Unified command system
- **Language Runtimes**: Multi-language support

## 🧪 Testing | 测试

### Unit Tests | 单元测试

```bash
# Run kernel tests | 运行内核测试
make test-kernel

# Run user space tests | 运行用户空间测试
make test-user

# Run all tests | 运行所有测试
make test
```

### Integration Tests | 集成测试

```bash
# Boot test | 引导测试
make test-boot

# Filesystem tests | 文件系统测试
make test-fs

# Network tests | 网络测试
make test-net
```

### Performance Benchmarks | 性能基准

```bash
# Memory benchmarks | 内存基准
make bench-memory

# Filesystem benchmarks | 文件系统基准
make bench-fs

# Full benchmark suite | 完整基准套件
make benchmark
```

## 📋 Code Standards | 代码标准

### Naming Conventions | 命名约定

- **Functions**: `snake_case` (e.g., `sys_open`, `kmalloc`)
- **Types**: `PascalCase` (e.g., `Process`, `FileDescriptor`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_PATH`, `PAGE_SIZE`)
- **Files**: `snake_case.c/.h`

### Documentation | 文档

- All public APIs must be documented
- Use Doxygen-style comments for functions
- Include parameter descriptions and return values
- Document error conditions

### Example | 示例

```c
/**
 * Allocates kernel memory
 * @param size Size of memory to allocate in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void *kmalloc(size_t size) {
    // Implementation
}
```

## 🔧 Development Tools | 开发工具

### Debugging | 调试

```bash
# Attach GDB to running kernel | 将GDB附加到运行中的内核
make debug
gdb kernel.elf

# Memory leak detection | 内存泄漏检测
valgrind --tool=memcheck ./test_program

# Code formatting | 代码格式化
clang-format -i *.c *.h
```

### Profiling | 性能分析

```bash
# Kernel profiling | 内核性能分析
make profile
./profile_kernel

# Memory usage analysis | 内存使用分析
make memcheck
```

## 🚨 Common Issues | 常见问题

### Build Issues | 构建问题

1. **Missing dependencies**: Ensure all required tools are installed
2. **Compiler errors**: Check for syntax errors and missing includes
3. **Linker errors**: Verify library dependencies and link order

### Runtime Issues | 运行时问题

1. **Boot failures**: Check QEMU configuration and kernel image
2. **Memory corruption**: Use Valgrind for memory debugging
3. **Filesystem issues**: Test with different filesystem configurations

### Testing Issues | 测试问题

1. **Test failures**: Check test environment and dependencies
2. **Performance regressions**: Compare against baseline benchmarks
3. **Race conditions**: Use thread sanitizers for concurrent code

## 📚 Additional Resources | 其他资源

- [API Reference](../api-reference/API.md) - Complete API documentation
- [Runtime Guide](../runtime/README.md) - Language runtime documentation
- [Changelog](../CHANGELOG.md) - Version history and changes

## 🤝 Community | 社区

- **GitHub Issues**: Report bugs and request features
- **GitHub Discussions**: Ask questions and discuss ideas
- **Contributing Guide**: Learn how to contribute effectively

## 📞 Support | 支持

For development-related questions:
- Check existing issues and discussions
- Create a new issue for bugs or feature requests
- Join our community discussions

---

*This developer guide covers BrightS version 0.1.2.2* | *本开发者指南涵盖 BrightS 版本 0.1.2.2*