# BrightS API 文档 | API Documentation

## 概述 | Overview

BrightS 内核提供了完整的系统编程接口，支持文件系统操作、内存管理、网络通信等核心功能。

## 内存管理 API | Memory Management API

### 核心函数 | Core Functions

#### `void *brights_kmalloc(size_t size)`
分配指定大小的内核内存块。

**参数 | Parameters:**
- `size`: 要分配的内存大小（字节）

**返回值 | Returns:**
- 成功：指向分配内存的指针
- 失败：`NULL`

**示例 | Example:**
```c
void *buffer = brights_kmalloc(1024); // 分配1KB内存
if (buffer) {
    // 使用内存
    brights_kfree(buffer); // 释放内存
}
```

#### `void brights_kfree(void *ptr)`
释放之前通过 `brights_kmalloc` 分配的内存。

**参数 | Parameters:**
- `ptr`: 要释放的内存指针（可以为NULL）

#### `size_t brights_kmalloc_used(void)`
获取当前已使用的内存总量。

**返回值 | Returns:**
- 已使用的内存字节数

#### `size_t brights_kmalloc_capacity(void)`
获取内存分配器的总容量。

**返回值 | Returns:**
- 总可用内存字节数

## 文件系统 API | Filesystem API

### RAMFS 文件系统 | RAMFS Filesystem

#### `int brights_ramfs_init(void)`
初始化RAMFS文件系统。

**返回值 | Returns:**
- 成功：0
- 失败：负数

#### `int brights_ramfs_create(const char *name)`
创建新文件。

**参数 | Parameters:**
- `name`: 文件路径

**返回值 | Returns:**
- 成功：0
- 失败：负数

#### `int brights_ramfs_open(const char *name)`
打开现有文件。

**参数 | Parameters:**
- `name`: 文件路径

**返回值 | Returns:**
- 成功：文件描述符 (>= 0)
- 失败：负数

#### `int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len)`
从文件中读取数据。

**参数 | Parameters:**
- `fd`: 文件描述符
- `off`: 读取偏移量
- `buf`: 目标缓冲区
- `len`: 要读取的字节数

**返回值 | Returns:**
- 成功：实际读取的字节数
- 失败：负数

#### `int64_t brights_ramfs_write(int fd, uint64_t off, const void *buf, uint64_t len)`
向文件写入数据。

**参数 | Parameters:**
- `fd`: 文件描述符
- `off`: 写入偏移量
- `buf`: 源数据缓冲区
- `len`: 要写入的字节数

**返回值 | Returns:**
- 成功：实际写入的字节数
- 失败：负数

#### `int brights_ramfs_close(int fd)`
关闭文件描述符。

**参数 | Parameters:**
- `fd`: 文件描述符

**返回值 | Returns:**
- 成功：0
- 失败：负数

#### `uint64_t brights_ramfs_file_size(int fd)`
获取文件大小。

**参数 | Parameters:**
- `fd`: 文件描述符

**返回值 | Returns:**
- 文件大小（字节）

## 字符串操作 API | String Operations API

### 字符串查找 | String Search

#### `char *kutil_strchr(const char *s, int c)`
在字符串中查找指定字符。

**参数 | Parameters:**
- `s`: 要搜索的字符串
- `c`: 要查找的字符

**返回值 | Returns:**
- 找到：指向字符的指针
- 未找到：`NULL`

#### `char *kutil_strrchr(const char *s, int c)`
在字符串中反向查找指定字符。

#### `char *kutil_strstr(const char *haystack, const char *needle)`
在字符串中查找子串。

**参数 | Parameters:**
- `haystack`: 要搜索的字符串
- `needle`: 要查找的子串

**返回值 | Returns:**
- 找到：指向子串起始位置的指针
- 未找到：`NULL`

### 字符串比较 | String Comparison

#### `int kutil_strcmp(const char *a, const char *b)`
比较两个字符串。

**返回值 | Returns:**
- `a < b`: 负数
- `a == b`: 0
- `a > b`: 正数

#### `int kutil_strncmp(const char *a, const char *b, size_t n)`
比较两个字符串的前n个字符。

## 网络 API | Network API

### DNS 解析 | DNS Resolution

#### `int brights_dns_resolve(const char *hostname, uint32_t *ip_out)`
将主机名解析为IP地址。

**参数 | Parameters:**
- `hostname`: 主机名字符串
- `ip_out`: 输出IP地址（网络字节序）

**返回值 | Returns:**
- 成功：0
- 失败：负数

### HTTP 客户端 | HTTP Client

#### `int brights_http_init(void)`
初始化HTTP客户端。

**返回值 | Returns:**
- 成功：0
- 失败：负数

## 错误码 | Error Codes

- `0`: 成功 | Success
- `-1`: 一般错误 | General error
- `-2`: 无效参数 | Invalid argument
- `-3`: 资源不足 | Out of resources
- `-4`: 权限拒绝 | Permission denied
- `-5`: 文件不存在 | File not found
- `-6`: 文件已存在 | File exists
- `-7`: 设备忙 | Device busy

## 类型定义 | Type Definitions

### 文件系统类型 | Filesystem Types

```c
typedef struct {
    char path[BRIGHTS_RAMFS_MAX_NAME];
    uint64_t size;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    int is_dir;
    int is_symlink;
} brights_ramfs_stat_t;
```

### 网络类型 | Network Types

```c
typedef uint32_t brights_ip_addr_t;  // IPv4地址
typedef uint16_t brights_port_t;     // 端口号
```

---

*API版本：1.0 | 最后更新：2026年4月9日*</content>
<parameter name="filePath">docs/API.md