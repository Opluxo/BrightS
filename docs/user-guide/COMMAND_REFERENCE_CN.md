# BrightS 命令参考

版本: 0.1.2.4

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
| `chmod` | 修改权限 | `chmod <模式> <文件>...` |

### 文本处理

| 命令 | 描述 | 语法 |
|------|------|------|
| `echo` | 显示文本 | `echo <字符串>...` |
| `grep` | 搜索文件内容 | `grep [选项] <模式> [文件]...` |
| `find` | 按名称查找文件 | `find <路径> -name <模式>` |

### 系统信息

| 命令 | 描述 | 语法 |
|------|------|------|
| `pwd` | 显示当前目录 | `pwd` |
| `ps` | 显示进程 | `ps` |
| `jobs` | 显示后台作业 | `jobs` |

### 网络

| 命令 | 描述 | 语法 |
|------|------|------|
| `ping` | ICMP 回显请求 | `ping <主机>` |

### Shell 内置

| 命令 | 描述 | 语法 |
|------|------|------|
| `cd` | 切换目录 | `cd [目录]` |
| `help` | 显示帮助 | `help [命令]` |
| `exit` | 退出 Shell | `exit` |

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
