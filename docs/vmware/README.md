# BrightS VMware 支持

## 概述

BrightS 内核支持 VMware Backdoor 接口，可在 VMware 虚拟机中运行。

## 状态

- ✅ 启动时自动检测 VMware 环境
- ✅ VMware Backdoor I/O 端口通信
- ✅ VMware 时间同步
- ⏳ 拖放 / 共享文件夹 (规划中)

## 实现

- `kernel/core/vmware.c` — backdoor 实现
- `kernel/core/vmware.h` — 接口定义
- 启动时通过 I/O 端口魔术字检测，检测到后初始化 backdoor 并启用时间同步

## 测试

在 VMware Workstation/Player 中创建虚拟机，挂载 ISO 启动，检查串口输出 `vmware: backdoor initialized`。

## 注意事项

- Backdoor 仅在 VMware 环境中激活；QEMU 或物理机显示 `vmware: backdoor not available`
- ISO 镜像同时支持 QEMU 和 VMware
