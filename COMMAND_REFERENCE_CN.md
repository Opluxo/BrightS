# BrightS 系统参考手册
# 版本: 0.1.2.2

## 概述

BrightS 是一个完整的类Unix操作系统，具有统一的命令框架和全面的系统调用接口。本文档列出了所有可用的用户命令和系统调用。

---

## 用户命令

BrightS 通过 BrightS 包管理器 (BSPM) 系统提供丰富的用户空间命令集。

### 文件操作

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `ls` | 列出目录内容 | `ls [选项] [路径]` | `ls -l /bin` |
| `cat` | 连接并显示文件 | `cat [文件]...` | `cat file.txt` |
| `cp` | 复制文件和目录 | `cp [选项] 源文件 目标` | `cp -r src/ dst/` |
| `mv` | 移动/重命名文件和目录 | `mv 源文件 目标` | `mv old.txt new.txt` |
| `rm` | 删除文件和目录 | `rm [选项] 文件...` | `rm -rf temp/` |
| `mkdir` | 创建目录 | `mkdir [选项] 目录...` | `mkdir -p path/to/dir` |
| `chmod` | 更改文件权限 | `chmod 模式 文件...` | `chmod 755 script.sh` |

### 文本处理

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `echo` | 显示文本 | `echo [选项] 字符串...` | `echo "Hello World"` |
| `grep` | 在文件中搜索模式 | `grep [选项] 模式 [文件]...` | `grep "error" *.log` |
| `find` | 搜索文件 | `find 路径 -name 模式` | `find . -name "*.c"` |

### 系统信息

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `pwd` | 显示当前工作目录 | `pwd` | `pwd` |
| `ps` | 显示进程信息 | `ps` | `ps` |
| `jobs` | 显示作业信息 | `jobs` | `jobs` |

### 网络工具

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `ping` | 发送ICMP回显请求 | `ping 主机` | `ping 192.168.1.1` |

### Shell 内置命令

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `cd` | 更改目录 | `cd [目录]` | `cd /home` |
| `help` | 显示命令帮助 | `help [命令]` | `help ls` |
| `exit` | 退出Shell | `exit` | `exit` |

### 包管理

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `bspm install` | 安装包 | `bspm install 包名` | `bspm install firefox` |
| `bspm remove` | 删除包 | `bspm remove 包名` | `bspm remove firefox` |
| `bspm update` | 更新包列表 | `bspm update` | `bspm update` |
| `bspm upgrade` | 升级所有包 | `bspm upgrade` | `bspm upgrade` |
| `bspm search` | 搜索包 | `bspm search 查询` | `bspm search editor` |
| `bspm list` | 列出已安装包 | `bspm list` | `bspm list` |
| `bspm info` | 显示包信息 | `bspm info 包名` | `bspm info firefox` |
| `bspm clean` | 清理缓存 | `bspm clean` | `bspm clean` |

### 多语言执行

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `rust` | 执行Rust代码 | `rust "代码"` | `rust "println!(\"Hello!\");"` |
| `python` | 执行Python代码 | `python "代码"` | `python "print('Hello!')"` |
| `cpp` | 执行C++代码 | `cpp "代码"` | `cpp "std::cout << \"Hello!\";"` |

### 高级Shell功能

| 功能 | 描述 | 使用方法 |
|------|------|----------|
| 管道 | 命令串联 | `ls \| grep "\.c$"` |
| 重定向 | I/O重定向 | `echo "test" > file.txt` |
| 后台作业 | 后台执行 | `long_command &` |
| 命令历史 | 历史导航 | `↑` / `↓` 方向键 |
| 自动补全 | 命令补全 | `Tab` 键 |

---

## 系统调用 (Syscalls)

BrightS 提供全面的系统调用接口用于低级系统操作。

### 文件操作

| 系统调用 | 描述 | 参数 | 返回值 |
|----------|------|------|--------|
| `sys_open` | 打开文件 | `const char *path, int flags` | `int fd` |
| `sys_close` | 关闭文件描述符 | `int fd` | `int` |
| `sys_read` | 从文件描述符读取 | `int fd, void *buf, size_t count` | `ssize_t` |
| `sys_write` | 写入文件描述符 | `int fd, const void *buf, size_t count` | `ssize_t` |
| `sys_stat` | 获取文件状态 | `const char *path, uint64_t *size, uint32_t *mode` | `int` |
| `sys_unlink` | 删除文件 | `const char *path` | `int` |
| `sys_mkdir` | 创建目录 | `const char *path` | `int` |
| `sys_readdir` | 读取目录条目 | `int fd, char *buf, size_t size` | `ssize_t` |
| `sys_chmod` | 更改文件权限 | `const char *path, uint32_t mode` | `int` |
| `sys_symlink` | 创建符号链接 | `const char *target, const char *linkpath` | `int` |

### 进程管理

| 系统调用 | 描述 | 参数 | 返回值 |
|----------|------|------|--------|
| `sys_fork` | 创建子进程 | `void` | `int64_t pid` |
| `sys_exec` | 执行程序 | `const char *path, char *const argv[], char *const envp[]` | `int` |
| `sys_wait` | 等待进程状态改变 | `int64_t *status` | `int64_t pid` |
| `sys_exit` | 终止调用进程 | `int status` | `void` |
| `sys_getpid` | 获取进程ID | `void` | `int64_t` |
| `sys_getppid` | 获取父进程ID | `void` | `int64_t` |

### 内存管理

| 系统调用 | 描述 | 参数 | 返回值 |
|----------|------|------|--------|
| `sys_realloc` | 重新分配内存 | `void *ptr, size_t size` | `void *` |

### 进程间通信

| 系统调用 | 描述 | 参数 | 返回值 |
|----------|------|------|--------|
| `sys_pipe` | 创建管道 | `int pipefd[2]` | `int` |
| `sys_dup` | 复制文件描述符 | `int oldfd` | `int` |
| `sys_dup2` | 复制文件描述符到指定fd | `int oldfd, int newfd` | `int` |

### 时间和定时

| 系统调用 | 描述 | 参数 | 返回值 |
|----------|------|------|--------|
| `sys_clock_ms` | 获取当前时间（毫秒） | `void` | `int64_t` |
| `sys_sleep_ms` | 睡眠指定毫秒 | `uint32_t milliseconds` | `void` |

### 网络

| 系统调用 | 描述 | 参数 | 返回值 |
|----------|------|------|--------|
| `sys_ip_parse` | 解析IP地址字符串 | `const char *str` | `uint32_t` |
| `sys_icmp_echo` | 发送ICMP回显请求 | `uint32_t dst_ip` | `int` |

---

## 命令分类

命令按以下类别组织：

- **CMD_CAT_FILE** (0): 文件操作 (ls, cp, mv, rm, mkdir, chmod)
- **CMD_CAT_NETWORK** (1): 网络工具 (ping)
- **CMD_CAT_SYSTEM** (2): 系统信息 (ps, jobs)
- **CMD_CAT_UTILITY** (3): 文本处理和实用工具 (echo, grep, find, cat)
- **CMD_CAT_DEVEL** (4): 开发工具
- **CMD_CAT_ADMIN** (5): 管理命令 (需要root权限)

---

## 错误代码

常见系统调用错误代码：

- `0`: 成功
- `-1`: 一般错误
- `-2`: 文件未找到
- `-3`: 权限被拒绝
- `-4`: 无效参数
- `-5`: 内存不足
- `-6`: 资源忙碌

---

## BSPM 包格式

BrightS 包使用 `.bsp` 格式：

```
package-name-version.bsp/
├── package.info    # 包元数据
├── files/          # 包文件
├── scripts/        # 安装脚本
│   ├── pre-install.sh
│   ├── post-install.sh
│   ├── pre-remove.sh
│   └── post-remove.sh
└── deps/           # 依赖项
```

---

## 获取帮助

- 运行 `help` 获取一般帮助
- 运行 `help <命令>` 获取特定命令帮助
- 查看 `docs/runtime/README.md` 获取详细文档
- 访问配置的 BSPM 仓库

---

*本参考手册涵盖 BrightS 版本 0.1.2.2。如需最新信息，请查看官方仓库。*