# 开发环境设置 | Development Environment Setup

## 系统要求 | System Requirements

### 基本要求 | Basic Requirements
- **操作系统**: Linux (推荐Ubuntu 20.04+ 或其他主流发行版)
- **处理器**: x86_64架构
- **内存**: 至少4GB RAM (推荐8GB+)
- **存储**: 至少10GB可用空间

### 开发工具 | Development Tools
- **CMake**: >= 3.20
- **Clang**: >= 12.0 (包含lld-link)
- **QEMU**: >= 6.0 (用于测试)
- **Git**: >= 2.25

## 安装依赖 | Installing Dependencies

### Ubuntu/Debian
```bash
# 更新包管理器
sudo apt update

# 安装基本开发工具
sudo apt install -y build-essential cmake clang lld

# 安装QEMU
sudo apt install -y qemu-system-x86 qemu-efi

# 安装UEFI固件
sudo apt install -y ovmf

# 可选：安装其他有用的工具
sudo apt install -y git gdb valgrind
```

### Fedora/CentOS/RHEL
```bash
# 安装基本开发工具
sudo dnf install -y cmake clang lld

# 安装QEMU
sudo dnf install -y qemu qemu-efi

# 安装UEFI固件
sudo dnf install -y edk2-ovmf

# 可选工具
sudo dnf install -y git gdb valgrind
```

### Arch Linux
```bash
# 安装基本开发工具
sudo pacman -S cmake clang lld

# 安装QEMU
sudo pacman -S qemu qemu-efi

# 安装UEFI固件
sudo pacman -S edk2-ovmf

# 可选工具
sudo pacman -S git gdb valgrind
```

## 验证安装 | Verifying Installation

```bash
# 检查CMake版本
cmake --version

# 检查Clang版本
clang --version

# 检查QEMU版本
qemu-system-x86_64 --version

# 检查UEFI固件位置
ls /usr/share/OVMF/ || ls /usr/share/edk2*/
```

## 项目设置 | Project Setup

```bash
# 克隆仓库
git clone <repository-url>
cd BrightS

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 构建项目
make -j$(nproc)

# 运行测试
make test
```

## 运行和调试 | Running and Debugging

### 基本运行 | Basic Run
```bash
# 在QEMU中运行
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive file=fat:rw:sys/kernel,format=raw \
  -serial stdio \
  -m 512
```

### 调试模式 | Debug Mode
```bash
# 启用GDB调试
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive file=fat:rw:sys/kernel,format=raw \
  -serial stdio \
  -m 512 \
  -s -S

# 在另一个终端中连接GDB
gdb sys/kernel/kernel.elf
(gdb) target remote localhost:1234
```

## 故障排除 | Troubleshooting

### 常见问题 | Common Issues

#### 编译错误 | Build Errors
```bash
# 清理构建缓存
cd build
rm -rf *
cmake ..
make clean
make -j$(nproc)
```

#### QEMU无法启动 | QEMU Won't Start
```bash
# 检查UEFI固件路径
find /usr/share -name "*OVMF*" -type f

# 尝试不同的固件文件
qemu-system-x86_64 -bios /usr/share/edk2/ovmf/OVMF_CODE.fd ...
```

#### 测试失败 | Tests Failing
```bash
# 运行详细测试输出
cd build
ctest --output-on-failure -V
```

### 获取帮助 | Getting Help

- **问题报告**: 使用GitHub Issues
- **讨论**: 使用GitHub Discussions
- **文档**: 查看 [docs/README.md](docs/README.md)

## 性能调优 | Performance Tuning

### 构建优化 | Build Optimization
```bash
# 使用更多并行作业
make -j$(nproc)

# 启用编译缓存（如果安装了ccache）
export CC="ccache clang"
export CXX="ccache clang++"
```

### 运行时优化 | Runtime Optimization
- 使用 `-m 1024` 或更多内存用于测试
- 启用KVM加速（如果可用）：添加 `-enable-kvm` 参数
- 使用固态硬盘存储以提高I/O性能

---

*最后更新：2026年4月9日*</content>
<parameter name="filePath">docs/DEVELOPMENT.md