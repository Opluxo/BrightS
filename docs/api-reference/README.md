# BrightS API Reference | BrightS API参考

This section contains technical documentation for BrightS system APIs, system calls, and internal interfaces.

## 📋 Contents | 目录

### Core APIs | 核心API
- **[📖 Kernel API](API.md)** - Complete kernel API reference
- **[🔧 System Calls](syscall-reference.md)** - System call documentation

### API Categories | API分类

#### Memory Management | 内存管理
- `kmalloc()` / `kfree()` - Kernel memory allocation
- `vmalloc()` / `vfree()` - Virtual memory management
- Memory pool and slab allocation APIs

#### Filesystem Operations | 文件系统操作
- VFS (Virtual File System) layer APIs
- Filesystem driver interfaces
- File and directory operations

#### Process Management | 进程管理
- Process creation and scheduling
- Thread management APIs
- Synchronization primitives

#### Device Drivers | 设备驱动
- Driver registration and initialization
- Hardware abstraction layer (HAL)
- Interrupt handling APIs

#### Networking | 网络
- Socket APIs and network stack
- Protocol implementations
- Network device drivers

## 🔧 System Calls | 系统调用

BrightS provides a comprehensive set of system calls for user-space applications:

### File Operations | 文件操作
```c
int sys_open(const char *path, int flags);
int sys_close(int fd);
ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);
int sys_stat(const char *path, uint64_t *size, uint32_t *mode);
int sys_unlink(const char *path);
```

### Process Management | 进程管理
```c
int64_t sys_fork(void);
int sys_exec(const char *path, char *const argv[], char *const envp[]);
int64_t sys_wait(int64_t *status);
void sys_exit(int status);
int64_t sys_getpid(void);
int64_t sys_getppid(void);
```

### Memory and IPC | 内存与进程间通信
```c
void *sys_realloc(void *ptr, size_t size);
int sys_pipe(int pipefd[2]);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
```

### Time and Networking | 时间与网络
```c
int64_t sys_clock_ms(void);
void sys_sleep_ms(uint32_t milliseconds);
uint32_t sys_ip_parse(const char *str);
int sys_icmp_echo(uint32_t dst_ip);
```

## 📚 API Documentation Guidelines | API文档指南

### Function Documentation | 函数文档

All API functions should be documented with:

```c
/**
 * Brief description of function purpose
 *
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return Description of return value
 *
 * @note Any important notes
 * @warning Any warnings
 * @see Related functions
 */
return_type function_name(param_type param1, param_type param2);
```

### Error Handling | 错误处理

API functions should clearly document error conditions:

- **Return codes**: Standard error codes (-1, -2, etc.)
- **errno values**: POSIX-compatible error numbers
- **Error messages**: Descriptive error strings

### Thread Safety | 线程安全

Document thread safety guarantees:

- **Thread-safe**: Can be called from multiple threads
- **Not thread-safe**: Requires external synchronization
- **MT-safe**: Multi-thread safe with specific conditions

## 🔍 Finding APIs | 查找API

### By Category | 按类别查找
- Use the table of contents above
- Check the main [API.md](API.md) file for comprehensive listings

### By Function Name | 按函数名查找
- Search within this documentation
- Use grep in the source code: `grep -r "function_name" kernel/`

### By Header File | 按头文件查找
- Check `include/` directory for public headers
- Look for function declarations in `.h` files

## 🛠️ Development Tools | 开发工具

### API Testing | API测试

```bash
# Build and run tests | 构建并运行测试
make test-api

# Run specific API tests | 运行特定API测试
make test-memory
make test-filesystem
```

### API Documentation Generation | API文档生成

```bash
# Generate API docs | 生成API文档
make docs-api

# View generated docs | 查看生成文档
firefox docs/api/index.html
```

## 🚨 Common Issues | 常见问题

### API Usage Errors | API使用错误

1. **Null pointer dereferences**: Always check return values
2. **Buffer overflows**: Validate buffer sizes
3. **Resource leaks**: Properly free allocated resources

### Threading Issues | 线程问题

1. **Race conditions**: Use proper synchronization
2. **Deadlocks**: Avoid circular lock dependencies
3. **Priority inversions**: Design lock hierarchies carefully

## 📞 Support | 支持

For API-related questions:
- Check existing documentation
- Review source code comments
- Create GitHub issues for clarification
- Join developer discussions

## 📝 Contributing | 贡献

When adding new APIs:
1. Document the API following the guidelines above
2. Add appropriate tests
3. Update this reference documentation
4. Ensure backward compatibility

---

*This API reference covers BrightS version 0.1.2.2* | *本API参考涵盖 BrightS 版本 0.1.2.2*