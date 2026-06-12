# BrightS 命令参考

版本: 0.1.2.6

## 用户命令

### 文件操作

| 命令 | 描述 | 语法 |
|------|------|------|
| `ls` | 列出目录内容 | `ls [选项] [路径]` |
| `cat` | 显示文件内容 | `cat <文件>...` |
| `cp` | 复制文件/目录 | `cp <源> <目标>` |
| `mv` | 移动/重命名 | `mv <源> <目标>` |
| `rm` | 删除文件/目录 | `rm [选项] <文件>...` |
| `mkdir` | 创建目录 | `mkdir [选项] <目录>...` |
| `rmdir` | 删除空目录 | `rmdir <目录>` |
| `chmod` | 修改权限 | `chmod <模式> <文件>...` |
| `touch` | 创建空文件 | `touch <文件>` |
| `stat` | 显示文件状态 | `stat <文件>` |
| `tree` | 显示目录树 | `tree [路径]` |
| `realpath` | 解析路径 | `realpath <路径>` |

### 文本处理

| 命令 | 描述 | 语法 |
|------|------|------|
| `echo` | 显示文本（支持 `$VAR`） | `echo <字符串>...` |
| `grep` | 搜索文件内容 | `grep [选项] <模式> [文件]...` |
| `find` | 按名称查找文件 | `find <路径> -name <模式>` |
| `hexdump` | 显示十六进制转储 | `hexdump <文件>` |

### 系统信息

| 命令 | 描述 | 语法 |
|------|------|------|
| `pwd` | 显示当前目录 | `pwd` |
| `ps` | 显示进程 | `ps` |
| `top` | 进程监控 | `top` |
| `jobs` | 显示后台作业 | `jobs` |
| `uname` | 系统信息 | `uname [-a]` |
| `bst` | 系统信息（brights） | `bst` |
| `sysinfo` | 详细系统信息 | `sysinfo` |
| `date` | 显示日期/时间 | `date` |
| `uptime` | 系统运行时间 | `uptime` |
| `free` | 内存使用情况 | `free` |
| `df` | 磁盘使用情况 | `df` |

### 网络

| 命令 | 描述 | 语法 |
|------|------|------|
| `ping` | ICMP 回显请求 | `ping <主机>` |
| `netget` | 获取 URL 内容（HTTP） | `netget <url>` |
| `dns` | 配置 DNS 服务器 | `dns <ns1> [ns2]` |
| `ifconfig` | 网络接口配置 | `ifconfig` |
| `wifi` | WiFi 管理 | `wifi` |

### 进程控制

| 命令 | 描述 | 语法 |
|------|------|------|
| `kill` | 按 PID 杀死进程 | `kill <pid>` |
| `killall` | 按名称杀死进程 | `killall <name>` |
| `bg` | 后台作业 | `bg` |
| `fg` | 前台作业 | `fg` |
| `wait` | 等待进程 | `wait` |
| `disown` | 从表中移除作业 | `disown` |
| `nice` | 以优先级运行 | `nice <command>` |
| `renice` | 更改优先级 | `renice <pid> <priority>` |

### Shell 内置

| 命令 | 描述 | 语法 |
|------|------|------|
| `cd` | 切换目录 | `cd [目录]` |
| `help` | 显示帮助 | `help [命令]` |
| `exit` | 退出 Shell | `exit` |
| `export` | 设置环境变量 | `export <KEY>=<VALUE>` |
| `env` | 显示环境变量 | `env` |
| `history` | 命令历史 | `history` |
| `which` | 查找命令位置 | `which <command>` |
| `type` | 显示命令类型 | `type <command>` |
| `umask` | 文件模式掩码 | `umask` |
| `ulimit` | 资源限制 | `ulimit` |

### 工具

| 命令 | 描述 | 语法 |
|------|------|------|
| `sleep` | 休眠秒数 | `sleep <seconds>` |
| `seq` | 打印数字序列 | `seq <last>` 或 `seq <first> <last>` |
| `yes` | 重复输出 | `yes` |

### 用户管理

| 命令 | 描述 | 语法 |
|------|------|------|
| `login` | 登录用户 | `login` |
| `logout` | 注销当前用户 | `logout` |
| `whoami` | 显示当前用户 | `whoami` |
| `passwd` | 修改密码 | `passwd` |
| `useradd` | 添加新用户 | `useradd` |
| `profile` | 用户配置 | `profile` |

### 包管理 (BSPM)

| 命令 | 描述 | 语法 |
|------|------|------|
| `bspm install` | 安装包 | `bspm install <包名>` |
| `bspm remove` | 删除包 | `bspm remove <包名>` |
| `bspm update` | 更新包列表 | `bspm update` |
| `bspm upgrade` | 升级所有包 | `bspm upgrade` |
| `bspm search` | 搜索包 | `bspm search <关键词>` |
| `bspm list` | 列出已安装包 | `bspm list` |
| `bspm info` | 查看包信息 | `bspm info <包名>` |
| `bspm clean` | 清理缓存 | `bspm clean` |

### 多语言执行

| 命令 | 描述 | 语法 | 示例 |
|------|------|------|------|
| `rust` | 执行 Rust 代码 | `rust '<代码>'` | `rust 'println!("hi");'` |
| `python` | 执行 Python 代码 | `python '<代码>'` | `python 'print("hi")'` |
| `cpp` | 执行 C++ 代码 | `cpp '<代码>'` | `cpp 'cout << "hi";'` |

### 系统控制

| 命令 | 描述 | 语法 |
|------|------|------|
| `reboot` | 重启系统 | `reboot` |
| `halt` | 关闭系统 | `halt` |
| `clear` | 清屏 | `clear` |