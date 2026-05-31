# BrightS API 参考

## 内存管理

```c
void *brights_kmalloc(size_t size);       // 分配内核内存，失败返回 NULL
void brights_kfree(void *ptr);            // 释放内存
size_t brights_kmalloc_used(void);        // 已使用内存量
size_t brights_kmalloc_capacity(void);    // 总容量
```

## 文件系统 (RAMFS)

```c
int brights_ramfs_init(void);                                    // 初始化，成功 0 失败负值
int brights_ramfs_create(const char *name);                      // 创建文件
int brights_ramfs_open(const char *name);                        // 打开文件，返回 fd(>=0)
int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len);
int64_t brights_ramfs_write(int fd, uint64_t off, const void *buf, uint64_t len);
int brights_ramfs_close(int fd);                                 // 关闭文件
uint64_t brights_ramfs_file_size(int fd);                        // 获取文件大小
```

## 字符串操作

```c
char *kutil_strchr(const char *s, int c);       // 查找字符
char *kutil_strrchr(const char *s, int c);      // 反向查找字符
char *kutil_strstr(const char *haystack, const char *needle);  // 查找子串
int kutil_strcmp(const char *a, const char *b); // 比较，a<b 负值 a==b 0 a>b 正值
int kutil_strncmp(const char *a, const char *b, size_t n);  // 比较前 n 字符
```

## 网络

```c
int brights_dns_resolve(const char *hostname, uint32_t *ip_out);  // DNS 解析
int brights_http_init(void);                                       // HTTP 客户端初始化
```

## 错误码

| 值  | 含义             |
|-----|------------------|
| 0   | 成功             |
| -1  | 一般错误         |
| -2  | 无效参数         |
| -3  | 资源不足         |
| -4  | 权限拒绝         |
| -5  | 文件不存在       |
| -6  | 文件已存在       |
| -7  | 设备忙           |

## 类型定义

```c
typedef struct {
    char path[BRIGHTS_RAMFS_MAX_NAME];
    uint64_t size;
    uint32_t mode, uid, gid;
    int is_dir, is_symlink;
} brights_ramfs_stat_t;

typedef uint32_t brights_ip_addr_t;   // IPv4
typedef uint16_t brights_port_t;      // 端口
```

*API v1.0 | 最后更新 2026-04-09*
