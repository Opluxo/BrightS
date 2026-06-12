# 开发环境设置 | Development Setup

## 系统要求 | Requirements

- **OS**: Linux (Ubuntu 20.04+)
- **CPU**: x86_64
- **RAM**: ≥4GB (推荐8GB+)
- **Disk**: ≥10GB

## 安装依赖 | Dependencies

### Ubuntu/Debian
```bash
sudo apt install build-essential cmake clang lld qemu-system-x86 ovmf git
```

### Fedora
```bash
sudo dnf install cmake clang lld qemu edk2-ovmf git
```

### Arch
```bash
sudo pacman -S cmake clang lld qemu edk2-ovmf git
```

## 验证安装 | Verify

```bash
cmake --version
clang --version
qemu-system-x86_64 --version
ls /usr/share/OVMF/OVMF_CODE.fd
```

## 项目设置 | Setup

```bash
git clone https://github.com/Opluxo/BrightS.git
cd BrightS
```

## 构建 | Build

### x86_64 UEFI

```bash
CC=clang LD=lld cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./tools/make-iso.sh                      # → build/brights.iso
```

### i386 BIOS

```bash
bash scripts/build-i386.sh               # → build/floppy.img
```

## 运行 | Run

### x86_64 UEFI

```bash
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive file=build/brights.iso,format=raw \
  -serial stdio -m 512
```

### i386 BIOS

```bash
qemu-system-i386 \
  -drive file=build/floppy.img,format=raw \
  -serial stdio
```

## 调试 | Debug

```bash
# 终端1: QEMU GDB 服务
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive file=build/brights.iso,format=raw \
  -serial stdio -m 512 -s -S

# 终端2: GDB 连接
gdb build/kernel.elf -ex "target remote localhost:1234"
```

## 性能优化 | Performance

```bash
# 并行构建
make -j$(nproc)

# ccache 加速编译
export CC="ccache clang"
export CXX="ccache clang++"

# KVM 加速
qemu-system-x86_64 ... -enable-kvm
```

## 测试 | Tests

```bash
# 运行所有测试
cd build && ctest
```

*最后更新：2026-06-11*