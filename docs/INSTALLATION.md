# BrightS 安装与烧录指南

## 获取镜像

从 GitHub Releases 下载：

| 文件 | 架构 | 启动方式 |
|------|------|----------|
| `BrightS-v*-x86_64.iso` | x86_64 (UEFI) | UEFI 启动 |
| `BrightS-v*-i386.iso` | i386 (BIOS) | 传统 BIOS 启动 |

## 烧录到 USB

### Linux

```bash
# 确认设备（切勿写错！）
lsblk

# 写入镜像
sudo dd if=BrightS-v0.1.2.6-x86_64.iso of=/dev/sdX bs=4M status=progress conv=fsync
```

### Windows

推荐 [Rufus](https://rufus.ie)：
- 选择 ISO 文件
- 分区类型：GPT（x86_64）/ MBR（i386）
- 写入方式：DD 镜像

### Ventoy

直接将 `.iso` 文件复制到 Ventoy 分区即可。

## QEMU 测试

```bash
# x86_64 UEFI
qemu-system-x86_64 -bios /usr/share/edk2/x64/OVMF.4m.fd \
  -drive file=BrightS-v0.1.2.6-x86_64.iso,format=raw -m 512 -serial stdio

# i386 BIOS
qemu-system-i386 -drive file=BrightS-v0.1.2.6-i386.iso,format=raw -serial stdio
```

## 构建镜像

### x86_64

```bash
CC=clang LD=lld cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./tools/make-iso.sh                      # → build/brights.iso
```

### i386

```bash
bash scripts/build-i386.sh               # → build/floppy.img
```

*构建产物在 `build/` 目录下。*
