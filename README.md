# BrightS Operating System Kernel

## 项目状态
✅ 内核正常引导
✅ 支持 UEFI 启动
✅ x86_64 架构
✅ 虚拟内存管理
✅ 进程调度
✅ 终端 Shell
✅ PS/2 键盘驱动
✅ AHCI 硬盘驱动
✅ NVMe 支持
✅ TCP/IP 网络栈

## 编译构建
```bash
mkdir build && cd build
cmake ..
make -j8
```

## 运行测试
```bash
./tools/run-qemu.sh
```


## 文档
参见 docs/ 目录查看详细文档
