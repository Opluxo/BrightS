# BrightS
### x86_64 / i386 操作系统内核

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%20v2-6CC644?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/UEFI+BIOS-Boot-FF6C2C?style=flat-square" alt="Boot">
  <img src="https://img.shields.io/badge/Version-0.1.2.3-4FC08D?style=flat-square" alt="Version">
</p>

---

## 📋 项目结构

```
BrightS/
├── arch/              # x86_64(UEFI) + i386(BIOS) 架构代码
├── kernel/            # 内核：调度/内存/文件系统/网络/IPC
├── drivers/           # 显示/GPU 驱动
├── user/              # 用户态程序
├── include/kernel/    # 内核头文件
├── docs/              # 文档（中/英/日）
├── scripts/           # 构建脚本
├── tools/             # CI 构建工具
├── tests/             # 测试
├── sys/               # x86_64 UEFI CMake 构建
├── config/            # 配置文件
├── CMakeLists.txt     # CMake 入口
└── Makefile           # 构建入口
```

---

## ✨ 功能特性

| 类别 | 功能 |
|------|------|
| **引导** | UEFI (x86_64) + Legacy BIOS (i386) |
| **内存** | 4级/PAE 分页, Slab 分配器 (8MB), PMEM |
| **进程** | O(1) 轮询调度, 10-tick 抢占, SMP |
| **文件系统** | VFS2, RAMFS, DevFS, Btrfs |
| **网络** | TCP/IP, VirtIO-Net, DHCP, DNS, HTTP |
| **存储** | AHCI, NVMe, ramdisk |
| **Shell** | LightShell: 历史/Tab补全/管道/重定向/作业 |
| **多语言** | Rust/Python/C++ 内联运行时 |
| **安全** | SMEP/SMAP, 堆加固, 系统调用边界检查 |
| **性能** | SIMD memcpy/memset, LRU 缓存, LTO |

---

## 📦 构建 & 运行

```bash
# x86_64 UEFI
make build && make run

# i386 BIOS
bash scripts/build-i386.sh && qemu-system-i386 -drive file=build/floppy.img,format=raw -serial stdio

# 运行测试
make test
```

---

## 🌍 文档

| 语言 | 链接 |
|------|------|
| 🇨🇳 简体中文 | [docs/README_zh_CN.md](docs/README_zh_CN.md) |
| 🇺🇸 English | [docs/README.md](docs/README.md) |
| 🇯🇵 日本語 | [docs/README_ja.md](docs/README_ja.md) |

---

## 📄 开源协议

GNU General Public License v2. See [LICENSE](LICENSE).

---

<p align="center">
  <strong>BrightS v0.1.2.3</strong> · GPL v2
</p>
