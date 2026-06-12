# BrightS Command Reference

Version: 0.1.2.6

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
| `rmdir` | Remove empty directories | `rmdir <dir>` |
| `chmod` | Change permissions | `chmod <mode> <file>...` |
| `touch` | Create empty file | `touch <file>` |
| `stat` | Display file status | `stat <file>` |
| `tree` | Display directory tree | `tree [path]` |
| `realpath` | Resolve path | `realpath <path>` |

### Text Processing

| Command | Description | Syntax |
|---------|-------------|--------|
| `echo` | Display text (supports `$VAR`) | `echo <string>...` |
| `grep` | Search patterns in files | `grep [options] <pattern> [file]...` |
| `find` | Find files by name | `find <path> -name <pattern>` |
| `hexdump` | Display hex dump | `hexdump <file>` |

### System Info

| Command | Description | Syntax |
|---------|-------------|--------|
| `pwd` | Print working directory | `pwd` |
| `ps` | Show processes | `ps` |
| `top` | Process monitor | `top` |
| `jobs` | Show background jobs | `jobs` |
| `uname` | System information | `uname [-a]` |
| `bst` | System info (brights) | `bst` |
| `sysinfo` | Detailed system info | `sysinfo` |
| `date` | Display date/time | `date` |
| `uptime` | System uptime | `uptime` |
| `free` | Memory usage | `free` |
| `df` | Disk usage | `df` |

### Network

| Command | Description | Syntax |
|---------|-------------|--------|
| `ping` | ICMP echo request | `ping <host>` |
| `netget` | Fetch URL content (HTTP) | `netget <url>` |
| `dns` | Configure DNS servers | `dns <ns1> [ns2]` |
| `ifconfig` | Network interface config | `ifconfig` |
| `wifi` | WiFi management | `wifi` |

### Process Control

| Command | Description | Syntax |
|---------|-------------|--------|
| `kill` | Kill process by PID | `kill <pid>` |
| `killall` | Kill processes by name | `killall <name>` |
| `bg` | Background job | `bg` |
| `fg` | Foreground job | `fg` |
| `wait` | Wait for process | `wait` |
| `disown` | Remove job from table | `disown` |
| `nice` | Run with priority | `nice <command>` |
| `renice` | Change priority | `renice <pid> <priority>` |

### Shell Built-ins

| Command | Description | Syntax |
|---------|-------------|--------|
| `cd` | Change directory | `cd [dir]` |
| `help` | Show help | `help [command]` |
| `exit` | Exit shell | `exit` |
| `export` | Set environment variable | `export <KEY>=<VALUE>` |
| `env` | Show environment variables | `env` |
| `history` | Command history | `history` |
| `which` | Find command location | `which <command>` |
| `type` | Display command type | `type <command>` |
| `umask` | File mode mask | `umask` |
| `ulimit` | Resource limits | `ulimit` |

### Utilities

| Command | Description | Syntax |
|---------|-------------|--------|
| `sleep` | Sleep for seconds | `sleep <seconds>` |
| `seq` | Print number sequence | `seq <last>` or `seq <first> <last>` |
| `yes` | Repeated output | `yes` |

### User Management

| Command | Description | Syntax |
|---------|-------------|--------|
| `login` | Login as user | `login` |
| `logout` | Logout current user | `logout` |
| `whoami` | Display current user | `whoami` |
| `passwd` | Change password | `passwd` |
| `useradd` | Add new user | `useradd` |
| `profile` | User profile | `profile` |

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

### System Control

| Command | Description | Syntax |
|---------|-------------|--------|
| `reboot` | Reboot system | `reboot` |
| `halt` | Shutdown system | `halt` |
| `clear` | Clear screen | `clear` |