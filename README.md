# BrightS
### A x86_64 / i386 operating system kernel
<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%20v2-6CC644?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/UEFI+BIOS-Boot-FF6C2C?style=flat-square" alt="Boot">
  <img src="https://img.shields.io/badge/Version-0.1.2.3-4FC08D?style=flat-square" alt="Version">
</p>

---

## ✨ Features / 功能特性
| Category | Features |
|---|---|
| **Boot** | UEFI (x86_64) + Legacy BIOS (i386) |
| **Memory** | 4-level / PAE paging, Slab allocator (8MB heap), PMEM |
| **Process** | O(1) round-robin, 10-tick preemption, SMP |
| **Filesystem** | VFS2, RAMFS, DevFS, Btrfs |
| **Network** | TCP/IP, VirtIO-Net, DHCP, DNS, HTTP |
| **Storage** | AHCI, NVMe, ramdisk |
| **Shell** | LightShell: history, tab, pipes, redirection, jobs |
| **Multi-lang** | Rust, Python, C++ inline runtime |
| **Security** | SMEP/SMAP, heap hardening, syscall boundary |
| **Performance** | SIMD memcpy/memset, LRU cache, LTO |

---

## 📁 Project Structure / 项目结构
```
BrightS/
├── arch/                 # x86_64 UEFI + i386 BIOS
├── kernel/               # Core: sched, memory, fs, net
├── drivers/              # Display/GPU drivers
├── user/                 # Userspace programs
├── include/kernel/       # Kernel headers
├── docs/                 # Documentation
├── scripts/              # Build scripts
├── tests/                # Test suites
└── config/               # Configuration
```

---

## 📦 Build / 构建
```bash
make build   # Build kernel (x86_64 UEFI)
make run     # Run in QEMU
make test    # Run tests
```

---

## 🌍 Documentation / 文档
| Language | Link |
|---|---|
| 🇺🇸 English | [docs/README.md](docs/README.md) |
| 🇨🇳 简体中文 | [docs/README_zh_CN.md](docs/README_zh_CN.md) |
| 🇯🇵 日本語 | [docs/README_ja.md](docs/README_ja.md) |

---

## 📊 Performance / 性能
| Component | Optimization |
|---|---|
| **kmalloc** | Slab allocator, 10 classes, best-fit, 8MB heap |
| **sched** | O(1) runqueue via `__builtin_ctzll`, 10-tick quantum |
| **runtime** | ERMS ≥256B, 64-byte cache-line copy, SSE2 memset |
| **proc** | O(1) PID allocation via bitmap |
| **cache** | LRU: DNS, Path, Inode, Buffer |

---

## 🧪 Tests / 测试
- `test_ramfs` — RAM filesystem (7 cases)
- `test_benchmark` — Performance benchmarks
- `test_extended` — SIMD, cache, monitoring

---

## 📄 License / 许可证
GNU General Public License v2. See [LICENSE](LICENSE).

---

<p align="center">
  <strong>BrightS v0.1.2.3</strong> · GPL v2 License
</p>
