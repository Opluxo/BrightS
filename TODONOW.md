# BrightS 近期里程碑

## 1. ✅ VFS + 文件系统（已完成）
- VFS / VFS2 层
- RAMFS、devfs、btrfs（基础）

## 2. ❌ GUI 显示服务器
- 帧缓冲驱动已有
- 需要：窗口管理器、合成器、输入事件路由

## 3. ❌ USB 栈
- 需要：UHCI/EHCI 驱动、设备枚举、HID/存储类驱动

## 4. ❌ 动态链接器 + 共享库
- 需要：ELF 动态段解析、共享对象加载、符号重定位

## 5. ❌ POSIX 兼容层
- 需要：补齐 syscall、完善 libc、信号/进程组/tty 控制

## 6. ❌ 自托管编译器
- 需要：将 TinyCC 或 GCC 移植到 BrightS 用户态
