# BrightS Command Reference

Version: 0.1.2.3

## User Commands

### File Operations

| Command | Description | Syntax |
|---------|-------------|--------|
| `ls` | List directory contents | `ls [options] [path]` |
| `cat` | Display files | `cat <file>...` |
| `cp` | Copy files/directories | `cp <src> <dst>` |
| `mv` | Move/rename | `mv <src> <dst>` |
| `rm` | Remove files/directories | `rm [options] <file>...` |
| `mkdir` | Create directories | `mkdir [options] <dir>...` |
| `chmod` | Change permissions | `chmod <mode> <file>...` |

### Text Processing

| Command | Description | Syntax |
|---------|-------------|--------|
| `echo` | Display text | `echo <string>...` |
| `grep` | Search patterns in files | `grep [options] <pattern> [file]...` |
| `find` | Find files by name | `find <path> -name <pattern>` |

### System Info

| Command | Description | Syntax |
|---------|-------------|--------|
| `pwd` | Print working directory | `pwd` |
| `ps` | Show processes | `ps` |
| `jobs` | Show background jobs | `jobs` |

### Network

| Command | Description | Syntax |
|---------|-------------|--------|
| `ping` | ICMP echo request | `ping <host>` |

### Shell Built-ins

| Command | Description | Syntax |
|---------|-------------|--------|
| `cd` | Change directory | `cd [dir]` |
| `help` | Show help | `help [command]` |
| `exit` | Exit shell | `exit` |

### Package Management (BSPM)

| Command | Description | Syntax |
|---------|-------------|--------|
| `bspm install` | Install a package | `bspm install <package>` |
| `bspm remove` | Remove a package | `bspm remove <package>` |
| `bspm update` | Update package lists | `bspm update` |
| `bspm upgrade` | Upgrade all packages | `bspm upgrade` |
| `bspm search` | Search packages | `bspm search <query>` |
| `bspm list` | List installed packages | `bspm list` |
| `bspm info` | Show package info | `bspm info <package>` |
| `bspm clean` | Clean package cache | `bspm clean` |

### Multi-language Execution

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `rust` | Execute Rust code | `rust '<code>'` | `rust 'println!("hi");'` |
| `python` | Execute Python code | `python '<code>'` | `python 'print("hi")'` |
| `cpp` | Execute C++ code | `cpp '<code>'` | `cpp 'cout << "hi";'` |
