# BrightS File System Structure Definition

> v0.1.2.7 · Based on actual kernel implementation

## Overview

BrightS uses a VFS2 (Virtual File System) layer with mount-point-based dispatch.
Each mounted filesystem provides a `vfs_superblock_t` with operation dispatch tables.

**Mount points at boot:**
- `/` → RAMFS (root filesystem, in-memory)
- `/dev` → DevFS (device files)
- `/mnt/drive` → Btrfs (if disk available, fallback: unmounted)

**File type constants (vfs_inode.mode):**

| Constant | Value | Description |
|----------|-------|-------------|
| VFS_S_IFREG | 0x8000 | Regular file |
| VFS_S_IFDIR | 0x4000 | Directory |
| VFS_S_IFCHR | 0x2000 | Character device |
| VFS_S_IFBLK | 0x6000 | Block device |
| VFS_S_IFLNK | 0xA000 | Symbolic link |

---

## Directory Tree

```
/                          (RAMFS root)
├── sys/                   Core system files
│   ├── init.rc            Userspace init script
│   ├── profile            System profile (USER, HOME env vars)
│   └── readme.txt
│
├── usr/                   User files
│   └── home/              User home directories
│       ├── readme.txt
│       ├── notes.txt
│       ├── root/          Root user home
│       │   └── readme.txt
│       └── guest/         Guest user home
│           └── readme.txt
│
├── bin/                   Software packages
│   ├── readme.txt
│   ├── pkg/               Package manager
│   │   ├── bspm           BSPM installer script
│   │   ├── readme.txt
│   │   └── .db/           Package database
│   ├── config/            User configurations
│   │   ├── readme.txt
│   │   ├── root/          Root user config
│   │   │   └── example.pf
│   │   └── guest/         Guest user config
│   │       └── example.pf
│   ├── firmware/          Firmware packages
│   │   └── readme.txt
│   └── runtime/           Runtime environments
│       ├── readme.txt
│       ├── rust/          Rust toolchain
│       │   ├── rustc      Compiler wrapper
│       │   ├── cargo      Package manager wrapper
│       │   └── rustup     Toolchain manager wrapper
│       ├── c/             C/C++ toolchain
│       │   ├── gcc        GCC wrapper
│       │   ├── clang      Clang wrapper
│       │   ├── make       Make wrapper
│       │   └── gdb        GDB wrapper
│       └── python/        Python runtime
│           ├── python3    Interpreter wrapper
│           ├── pip        Package manager wrapper
│           ├── pip3       Pip3 alias
│           └── venv       Virtual environment wrapper
│
├── mnt/                   Mount points
│   ├── readme.txt
│   ├── drive/             External/storage disks
│   │   ├── readme.txt
│   │   ├── .mounted       Mount status (0/1)
│   │   ├── fs             Filesystem type (btrfs/none)
│   │   ├── role           Mount role (system)
│   │   └── backend        Storage backend (nvme/ahci/usb/ramdisk)
│   ├── input/             Input devices
│   │   ├── readme.txt
│   │   ├── keyboard/
│   │   ├── mouse/
│   │   ├── touchpad/
│   │   ├── camera/
│   │   └── biometric/
│   └── output/            Output devices
│       └── readme.txt
│
├── tmp/                   Temporary cache files
│   └── readme.txt
│
├── swp/                   Swap partition area
│   └── readme.txt
│
└── dev/                   Device files (DevFS)
    ├── com1               Serial port
    ├── kbd                Keyboard
    ├── mouse              Mouse
    ├── fb0                Framebuffer
    ├── sda                Block device (system disk)
    ├── null               Null device
    └── zero               Zero device
```

---

## VFS2 Core Structures

### vfs_inode_t (Inode - file/directory representation)

```c
typedef struct vfs_inode {
  uint32_t ino;        /* Inode number */
  uint32_t mode;       /* File type + permissions (VFS_S_IF*) */
  uint64_t size;       /* File size in bytes */
  uint32_t uid;        /* Owner user ID */
  uint32_t gid;        /* Owner group ID */
  uint32_t nlink;      /* Hard link count */
  uint64_t atime;      /* Access time */
  uint64_t mtime;      /* Modification time */
  uint64_t ctime;      /* Change time */
  struct vfs_superblock *sb;  /* Back-pointer to superblock */
  void *fs_private;    /* Filesystem-specific data */
} vfs_inode_t;
```

### vfs_file_t (Open file descriptor)

```c
typedef struct vfs_file {
  vfs_inode_t *inode;  /* Associated inode */
  uint64_t pos;        /* Current read/write position */
  uint32_t flags;      /* Open flags (VFS_O_RDONLY, etc.) */
  vfs_file_ops_t *fops;  /* File operation dispatch table */
  void *private_data;  /* Per-open instance data */
  int in_use;          /* Slot allocation flag */
} vfs_file_t;
```

### vfs_superblock_t (Mounted filesystem)

```c
typedef struct vfs_superblock {
  const char *fs_type_name;    /* Filesystem type name */
  vfs_sb_ops_t *sb_ops;        /* Superblock operations */
  vfs_file_ops_t *default_fops;  /* Default file operations */
  void *fs_private;            /* Filesystem-specific data */
  int active;                  /* Mount active flag */
} vfs_superblock_t;
```

### vfs_mount_t (Mount point mapping)

```c
typedef struct vfs_mount {
  char prefix[128];       /* Path prefix (e.g., "/", "/dev") */
  vfs_superblock_t *sb;   /* Associated superblock */
  int active;             /* Mount active flag */
} vfs_mount_t;
```

---

## Operation Dispatch Tables

### vfs_file_ops_t (Per-file operations)

| Function | Description |
|----------|-------------|
| `read(f, buf, count)` | Read bytes from file |
| `write(f, buf, count)` | Write bytes to file |
| `close(f)` | Close file descriptor |
| `lseek(f, offset, whence)` | Seek to position |
| `ioctl(f, request, argp)` | Device-specific control |
| `readdir(f, name_buf, buf_size)` | Read directory entry |

### vfs_sb_ops_t (Superblock operations)

| Function | Description |
|----------|-------------|
| `lookup(sb, path)` | Find inode by path |
| `create(sb, path)` | Create new file |
| `mkdir(sb, path)` | Create new directory |
| `unlink(sb, path)` | Delete file |
| `stat(sb, path, ...)` | Get file status |
| `readdir(sb, dir_path, ...)` | List directory contents |
| `symlink(sb, target, linkpath)` | Create symbolic link |
| `readlink(sb, path, buf, ...)` | Read symbolic link |
| `chmod(sb, path, mode)` | Change permissions |
| `chown(sb, path, uid, gid)` | Change ownership |

---

## Permission Bits

| Bit | Octal | Description |
|-----|-------|-------------|
| VFS_S_IRUSR | 0400 | Owner read |
| VFS_S_IWUSR | 0200 | Owner write |
| VFS_S_IXUSR | 0100 | Owner execute |
| VFS_S_IRGRP | 0040 | Group read |
| VFS_S_IWGRP | 0020 | Group write |
| VFS_S_IXGRP | 0010 | Group execute |
| VFS_S_IROTH | 0004 | Other read |
| VFS_S_IWOTH | 0002 | Other write |
| VFS_S_IXOTH | 0001 | Other execute |

**Default permissions:**
- Directory: `0755` (rwxr-xr-x)
- File: `0644` (rw-r--r--)

---

## Open Flags

| Flag | Value | Description |
|------|-------|-------------|
| VFS_O_RDONLY | 0x00 | Read only |
| VFS_O_WRONLY | 0x01 | Write only |
| VFS_O_RDWR | 0x02 | Read/Write |
| VFS_O_CREAT | 0x40 | Create if not exists |
| VFS_O_TRUNC | 0x200 | Truncate to zero |
| VFS_O_APPEND | 0x400 | Append mode |

---

## Limits

| Constant | Value | Description |
|----------|-------|-------------|
| VFS_MAX_FDS | 32 | Max file descriptors per process |
| VFS_MAX_MOUNTS | 8 | Max simultaneous mounts |
| VFS_MAX_SYMLINK_DEPTH | 8 | Max symlink follow depth |

---

## Filesystem Implementations

### RAMFS (ramfs.c / ramfs_vfs.c)
- In-memory filesystem for root `/`
- Supports: create, open, read, write, mkdir, stat, readdir
- No persistence (lost on reboot)

### DevFS (devfs.c / devfs_vfs.c)
- Device file namespace at `/dev`
- Provides: serial ports, keyboard, mouse, framebuffer, block devices

### Btrfs (btrfs.c)
- Copy-on-Write filesystem for persistent storage
- Mounted at `/mnt/drive` when disk available
- Features: snapshots, checksums, compression

---

## Boot Sequence

1. `brights_boot_fs_init()` → creates RAMFS root with directory tree
2. `brights_devfs_init()` → creates DevFS superblock
3. `brights_vfs2_mount("/", ramfs_sb)` → mount root
4. `brights_vfs2_mount("/dev", devfs_sb)` → mount device files
5. `brights_storage_bootstrap()` → detect NVMe/AHCI/USB/Ramdisk
6. `brights_storage_mount_system()` → mount Btrfs at `/mnt/drive`

---

## Path Resolution

```c
brights_vfs2_open("/dev/com1", flags, &file)
  → brights_vfs2_resolve("/dev/com1", &rel_path)
    → matches mount prefix "/dev"
    → returns devfs superblock
    → rel_path = "com1"
  → devfs_sb->sb_ops->lookup(devfs_sb, "com1")
  → returns inode
  → allocates vfs_file_t
```

---

## BSPM Repository

https://github.com/Opluxo/BrightS_Package_Manager
