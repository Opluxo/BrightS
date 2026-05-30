# BrightS

### A x86_64 / i386 operating system kernel

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%20v2-6CC644?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/UEFI+BIOS-Boot-FF6C2C?style=flat-square" alt="Boot">
  <img src="https://img.shields.io/badge/Version-2.0.0-4FC08D?style=flat-square" alt="Version">
</p>

---

## ✨ Features

| Category | Features |
|----------|----------|
| **Boot** | UEFI (x86_64) + Legacy BIOS (i386) |
| **Memory** | 4-level / PAE paging, Slab allocator (8MB heap), PMEM |
| **Process** | O(1) round-robin scheduling, 10-tick preemption, SMP |
| **Filesystem** | VFS2 abstraction, RAMFS, DevFS, Btrfs |
| **Network** | TCP/IP, VirtIO-Net, DHCP, DNS, HTTP |
| **Storage** | AHCI, NVMe, ramdisk, block device interface |
| **Shell** | LightShell: history, tab completion, pipes, redirection, jobs |
| **Multi-lang** | Rust, Python, C++ inline runtime |
| **Security** | SMEP/SMAP, heap hardening, syscall boundary checking |
| **Performance** | SIMD-accelerated memcpy/memset, LRU cache, LTO |

---

## 📁 Project Structure

```
BrightS/
├── arch/x86_64/          # x86_64 UEFI arch code
├── arch/i386/            # i386 BIOS arch code
├── kernel/               # Core kernel subsystems
│   ├── core/            # Scheduler, memory, proc, syscalls
│   ├── dev/             # Device drivers
│   ├── fs/              # Filesystems
│   ├── ipc/             # Pipes
│   └── net/             # Network stack
├── drivers/              # Display/GPU drivers
├── user/                 # Userspace programs
├── include/kernel/       # Kernel headers
├── docs/                 # Documentation
├── scripts/              # Build scripts
├── tests/                # Test suites
└── config/               # Configuration files
```

---

## 📦 Build

```bash
# Build kernel (x86_64 UEFI)
make build

# Run in QEMU
make run

# Run tests
make test
```

---

## 🌍 Documentation

| Language | Links |
|----------|-------|
| 🇺🇸 English | [docs/README.md](docs/README.md) |
| 🇨🇳 简体中文 | [docs/README_zh_CN.md](docs/README_zh_CN.md) |
| 🇯🇵 日本語 | [docs/README_ja.md](docs/README_ja.md) |

---

## 📊 Performance

| Component | Optimization |
|-----------|-------------|
| **kmalloc** | Slab allocator, 10 size classes, best-fit, 8MB heap |
| **sched** | O(1) runqueue via `__builtin_ctzll`, 10-tick quantum |
| **runtime** | ERMS ≥256B, 64-byte cache-line copy, SSE2 memset |
| **proc** | O(1) PID allocation via bitmap |
| **cache** | LRU: DNS, Path, Inode, Buffer caches |

---

## 🧪 Tests

- `test_ramfs` — RAM filesystem (7 test cases)
- `test_benchmark` — Performance benchmarks
- `test_extended` — SIMD, cache, monitoring

---

## 📄 License

GNU General Public License v2. See [LICENSE](LICENSE).

---

<p align="center">
  <strong>BrightS v2.0.0</strong> · GPL v2 License
</p>
