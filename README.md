# BrightS

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%20v2-6CC644?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/UEFI+BIOS-Boot-FF6C2C?style=flat-square" alt="Boot">
  <img src="https://img.shields.io/badge/Version-0.1.2.4-4FC08D?style=flat-square" alt="Version">
</p>

---

BrightS 是一个从零构建的操作系统内核，同时支持 x86_64 UEFI 和 i386 BIOS 两种引导方式。

BrightS is a ground-up operating system kernel supporting both x86_64 UEFI and i386 BIOS boot.

BrightS は、x86_64 UEFI と i386 BIOS の両方をサポートするスクラッチビルドの OS カーネルです。

---

## ✨ 功能特性 / Features / 機能

| 类别 / Category | 功能 / Features |
|---|---|
| **Boot / 引导** | UEFI (x86_64) + Legacy BIOS (i386) |
| **Memory / 内存** | 4-level / PAE paging, Slab allocator (8MB heap), PMEM |
| **Process / 进程** | O(1) round-robin scheduler, 10-tick preemption, SMP |
| **Filesystem / 文件系统** | VFS2, RAMFS, DevFS, Btrfs |
| **Network / 网络** | TCP/IP, VirtIO-Net, DHCP, DNS, HTTP |
| **Storage / 存储** | AHCI, NVMe, ramdisk, block device interface |
| **Shell** | LightShell: history, tab completion, pipes, redirection, jobs |
| **Multi-lang / 多语言** | Rust, Python, C++ inline runtime |
| **Security / 安全** | SMEP/SMAP, heap hardening, syscall boundary checking |
| **Performance / 性能** | SIMD memcpy/memset, LRU cache (DNS/Path/Inode/Buffer), LTO |

---

## 📦 构建 & 运行 / Build & Run / ビルド & 実行

```bash
# x86_64 UEFI
make build        # 构建内核
make run          # QEMU 中运行
make test         # 运行测试

# i386 BIOS
bash scripts/build-i386.sh
qemu-system-i386 -drive file=build/floppy.img,format=raw -serial stdio
```

### 构建输出 / Build Output

| Target | Image | Size |
|--------|-------|------|
| **x86_64 UEFI** | `build_x64/sys/kernel/kernel.efi` | ~176KB |
| **i386 BIOS** | `build/floppy.img` | 1.44MB (floppy) |

---

## 📁 项目结构 / Project Structure / プロジェクト構造

```
BrightS/
├── arch/x86_64/          # x86_64 UEFI 架构代码
├── arch/i386/            # i386 BIOS 架构代码
├── kernel/               # 内核核心 (core/ fs/ net/ ipc/)
├── drivers/              # 显示/GPU 驱动
├── user/                 # 用户态程序
├── include/kernel/       # 内核头文件
├── docs/                 # 文档 (中/英/日)
├── scripts/              # 构建脚本
├── tools/                # CI 工具
├── tests/                # 测试
├── sys/                  # x86_64 CMake 构建
├── config/               # 配置
├── CMakeLists.txt        # CMake 入口
└── Makefile              # 构建入口
```

---

## 🌍 文档 / Documentation / ドキュメント

| 言語 / Language | Link |
|---|---|
| 🇨🇳 **简体中文** | [docs/README_zh_CN.md](docs/README_zh_CN.md) |
| 🇺🇸 **English** | [docs/README.md](docs/README.md) |
| 🇯🇵 **日本語** | [docs/README_ja.md](docs/README_ja.md) |

各文档包含：用户指南 / 开发者指南 / API 参考 / 运行时文档 / 构建说明

---

## 📄 License / 许可证 / ライセンス

GNU General Public License v2. See [LICENSE](LICENSE).

---

<p align="center">
  <strong>BrightS v0.1.2.4</strong> · Designed by OpenLight Studio · GPL v2
</p>
