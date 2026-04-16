# BrightS

### A lightweight x86_64 operating system kernel

<p align="center">
  <img src="https://img.shields.io/badge/Arch-x86__64-1793D1?style=flat-square&logo=arch-linux" alt="Arch">
  <img src="https://img.shields.io/badge/License-GPL%20v2-6CC644?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/UEFI-Boot-FF6C2C?style=flat-square" alt="UEFI">
  <img src="https://img.shields.io/badge/Version-0.1.2.2-4FC08D?style=flat-square" alt="Version">
</p>

---

## ✨ Features

| Category | Features |
|----------|----------|
| **Boot** | UEFI boot, x86_64 architecture |
| **Memory** | Virtual memory management, Slab allocator (8MB heap) |
| **Process** | Round-robin scheduling, Time-slice preemption (10 tick quantum) |
| **Filesystem** | VFS2 abstraction, RAMFS, DevFS, Btrfs support |
| **Network** | TCP/IP stack, VirtIO-Net |
| **Storage** | AHCI, NVMe, block device interface |
| **Shell** | Built-in shell with pipes, redirection, history, tab completion |
| **Multi-lang** | Rust, Python, C++ runtime support |

---

## 📦 Build

```bash
# Build kernel
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run in QEMU
make run
```

---

## 🌍 Documentation

| Language | Links |
|----------|-------|
| 🇺🇸 English | [docs/README.md](docs/README.md) |
| 🇨🇳 简体中文 | [docs/README_zh_CN.md](docs/README_zh_CN.md) |
| 🇯🇵 日本語 | [docs/README_ja.md](docs/README_ja.md) |

### Quick Links
| User Guide | Developer Guide |
|------------|-----------------|
| [Commands (EN)](docs/user-guide/COMMAND_REFERENCE.md) | [Development (EN)](docs/developer-guide/DEVELOPMENT.md) |
| [命令参考 (中)](docs/user-guide/COMMAND_REFERENCE_CN.md) | [开发指南 (中)](docs/developer-guide/DEVELOPMENT_zh_CN.md) |
| [コマンド (日)](docs/user-guide/COMMAND_REFERENCE_ja.md) | [開発ガイド (日)](docs/developer-guide/DEVELOPMENT_ja.md) |

---

## 📊 Performance Optimizations

| Component | Optimization |
|-----------|-------------|
| **kmalloc** | Slab allocator with 10 size classes, 8MB heap |
| **sched** | O(1) runqueue with `__builtin_ctzll`, 10-tick quantum |
| **runtime** | ERMS for ≥256B, 64-byte cache-line copy, SSE2 memset |
| **proc** | O(1) PID allocation via bitmap, inline helpers |

---

## 📁 Project Structure

```
BrightS/
├── kernel/           # Core kernel
│   ├── arch/        # Architecture (x86_64)
│   ├── fs/          # Filesystems (VFS2, RAMFS, DevFS, Btrfs)
│   ├── net/         # Network stack
│   ├── kmalloc.c    # Memory allocator
│   ├── sched.c      # Process scheduler
│   └── proc.c       # Process management
├── drivers/         # Device drivers
├── docs/            # Documentation
└── scripts/         # Build scripts
```

---

<p align="center">
  <strong>BrightS v0.1.2.2</strong> · GPL v2 License
</p>
