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
git clone https://github.com/OpenLight-Studio/BrightS.git
cd BrightS
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 运行 | Run

```bash
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive file=fat:rw:build,format=raw \
  -serial stdio -m 512
```

## 调试 | Debug

```bash
# 终端1: QEMU GDB 服务
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive file=fat:rw:build,format=raw \
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
