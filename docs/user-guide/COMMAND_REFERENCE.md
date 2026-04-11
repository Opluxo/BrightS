# BrightS System Reference Manual
# Version: 0.1.2.2

## Overview

BrightS is a complete Unix-like operating system with a unified command framework and comprehensive system call interface. This document lists all available user commands and system calls.

---

## User Commands

BrightS provides a rich set of user-space commands through the BrightS Package Manager (BSPM) system.

### File Operations

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `ls` | List directory contents | `ls [OPTIONS] [PATH]` | `ls -l /bin` |
| `cat` | Concatenate and display files | `cat [FILE]...` | `cat file.txt` |
| `cp` | Copy files and directories | `cp [OPTIONS] SOURCE DEST` | `cp -r src/ dst/` |
| `mv` | Move/rename files and directories | `mv SOURCE DEST` | `mv old.txt new.txt` |
| `rm` | Remove files and directories | `rm [OPTIONS] FILE...` | `rm -rf temp/` |
| `mkdir` | Create directories | `mkdir [OPTIONS] DIRECTORY...` | `mkdir -p path/to/dir` |
| `chmod` | Change file permissions | `chmod MODE FILE...` | `chmod 755 script.sh` |

### Text Processing

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `echo` | Display text | `echo [OPTIONS] STRING...` | `echo "Hello World"` |
| `grep` | Search for patterns in files | `grep [OPTIONS] PATTERN [FILE]...` | `grep "error" *.log` |
| `find` | Search for files | `find PATH -name PATTERN` | `find . -name "*.c"` |

### System Information

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `pwd` | Print working directory | `pwd` | `pwd` |
| `ps` | Display process information | `ps` | `ps` |
| `jobs` | Display job information | `jobs` | `jobs` |

### Network Tools

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `ping` | Send ICMP echo requests | `ping HOST` | `ping 192.168.1.1` |

### Shell Built-ins

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `cd` | Change directory | `cd [DIRECTORY]` | `cd /home` |
| `help` | Show command help | `help [COMMAND]` | `help ls` |
| `exit` | Exit shell | `exit` | `exit` |

### Package Management

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `bspm install` | Install packages | `bspm install PACKAGE` | `bspm install firefox` |
| `bspm remove` | Remove packages | `bspm remove PACKAGE` | `bspm remove firefox` |
| `bspm update` | Update package lists | `bspm update` | `bspm update` |
| `bspm upgrade` | Upgrade all packages | `bspm upgrade` | `bspm upgrade` |
| `bspm search` | Search packages | `bspm search QUERY` | `bspm search editor` |
| `bspm list` | List installed packages | `bspm list` | `bspm list` |
| `bspm info` | Show package information | `bspm info PACKAGE` | `bspm info firefox` |
| `bspm clean` | Clean cache | `bspm clean` | `bspm clean` |

### Multi-language Execution

| Command | Description | Syntax | Example |
|---------|-------------|--------|---------|
| `rust` | Execute Rust code | `rust "CODE"` | `rust "println!(\"Hello!\");"` |
| `python` | Execute Python code | `python "CODE"` | `python "print('Hello!')"` |
| `cpp` | Execute C++ code | `cpp "CODE"` | `cpp "std::cout << \"Hello!\";"` |

### Advanced Shell Features

| Feature | Description | Usage |
|---------|-------------|-------|
| Pipes | Command chaining | `ls | grep "\.c$"` |
| Redirection | I/O redirection | `echo "test" > file.txt` |
| Background jobs | Background execution | `long_command &` |
| Command history | History navigation | `↑` / `↓` arrow keys |
| Auto-completion | Command completion | `Tab` key |

---

## System Calls (Syscalls)

BrightS provides a comprehensive system call interface for low-level system operations.

### File Operations

| System Call | Description | Parameters | Return |
|-------------|-------------|------------|--------|
| `sys_open` | Open a file | `const char *path, int flags` | `int fd` |
| `sys_close` | Close a file descriptor | `int fd` | `int` |
| `sys_read` | Read from file descriptor | `int fd, void *buf, size_t count` | `ssize_t` |
| `sys_write` | Write to file descriptor | `int fd, const void *buf, size_t count` | `ssize_t` |
| `sys_stat` | Get file status | `const char *path, uint64_t *size, uint32_t *mode` | `int` |
| `sys_unlink` | Remove a file | `const char *path` | `int` |
| `sys_mkdir` | Create a directory | `const char *path` | `int` |
| `sys_readdir` | Read directory entries | `int fd, char *buf, size_t size` | `ssize_t` |
| `sys_chmod` | Change file permissions | `const char *path, uint32_t mode` | `int` |
| `sys_symlink` | Create a symbolic link | `const char *target, const char *linkpath` | `int` |

### Process Management

| System Call | Description | Parameters | Return |
|-------------|-------------|------------|--------|
| `sys_fork` | Create a child process | `void` | `int64_t pid` |
| `sys_exec` | Execute a program | `const char *path, char *const argv[], char *const envp[]` | `int` |
| `sys_wait` | Wait for process to change state | `int64_t *status` | `int64_t pid` |
| `sys_exit` | Terminate the calling process | `int status` | `void` |
| `sys_getpid` | Get process ID | `void` | `int64_t` |
| `sys_getppid` | Get parent process ID | `void` | `int64_t` |

### Memory Management

| System Call | Description | Parameters | Return |
|-------------|-------------|------------|--------|
| `sys_realloc` | Reallocate memory | `void *ptr, size_t size` | `void *` |

### Inter-Process Communication

| System Call | Description | Parameters | Return |
|-------------|-------------|------------|--------|
| `sys_pipe` | Create a pipe | `int pipefd[2]` | `int` |
| `sys_dup` | Duplicate a file descriptor | `int oldfd` | `int` |
| `sys_dup2` | Duplicate file descriptor to specific fd | `int oldfd, int newfd` | `int` |

### Time and Timing

| System Call | Description | Parameters | Return |
|-------------|-------------|------------|--------|
| `sys_clock_ms` | Get current time in milliseconds | `void` | `int64_t` |
| `sys_sleep_ms` | Sleep for milliseconds | `uint32_t milliseconds` | `void` |

### Networking

| System Call | Description | Parameters | Return |
|-------------|-------------|------------|--------|
| `sys_ip_parse` | Parse IP address string | `const char *str` | `uint32_t` |
| `sys_icmp_echo` | Send ICMP echo request | `uint32_t dst_ip` | `int` |

---

## Command Categories

Commands are organized into the following categories:

- **CMD_CAT_FILE** (0): File operations (ls, cp, mv, rm, mkdir, chmod)
- **CMD_CAT_NETWORK** (1): Network tools (ping)
- **CMD_CAT_SYSTEM** (2): System information (ps, jobs)
- **CMD_CAT_UTILITY** (3): Text processing and utilities (echo, grep, find, cat)
- **CMD_CAT_DEVEL** (4): Development tools
- **CMD_CAT_ADMIN** (5): Administrative commands (requires root)

---

## Error Codes

Common system call error codes:

- `0`: Success
- `-1`: General error
- `-2`: File not found
- `-3`: Permission denied
- `-4`: Invalid argument
- `-5`: Out of memory
- `-6`: Resource busy

---

## BSPM Package Format

BrightS packages use the `.bsp` format:

```
package-name-version.bsp/
├── package.info    # Package metadata
├── files/          # Package files
├── scripts/        # Installation scripts
│   ├── pre-install.sh
│   ├── post-install.sh
│   ├── pre-remove.sh
│   └── post-remove.sh
└── deps/           # Dependencies
```

---

## Getting Help

- Run `help` for general help
- Run `help <command>` for specific command help
- Check `docs/runtime/README.md` for detailed documentation
- Visit the package repository at configured BSPM repos

---

*This reference manual covers BrightS version 0.1.2.2. For the latest information, check the official repository.*