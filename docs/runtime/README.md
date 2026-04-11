# BrightS 用户空间运行时环境

## 概述

BrightS 现在包含了完整的用户空间运行时环境，包括服务管理系统、守护进程框架、系统服务以及**多语言编程支持**。

## 核心组件

### 1. 多语言运行时系统
- **语言管理器** (lang_runtime.c/h): 统一的语言运行时注册和切换
- **运行时接口**: 标准化的语言执行和编译接口
- **语言检测**: 基于文件扩展名的自动语言识别
- **支持的语言**: Rust, Python, C++

#### Rust运行时 (rust_runtime.c/h)
- **编译器前端**: `rustc` - Rust到C的源代码转换器
- **运行时库**: 基本的内存分配和字符串操作
- **语法支持**: 函数定义、println!宏、变量绑定

#### Python运行时 (python_runtime.c/h)
- **解释器**: `python` - 简单的Python代码解释器
- **内置函数**: print(), len(), type()等
- **对象系统**: 基本的Python对象类型支持

#### C++运行时 (cpp_runtime.c/h)
- **编译器前端**: `g++` - C++到C的源代码转换器
- **标准库**: iostream, string, vector等基础支持
- **异常处理**: 基本的异常抛出和捕获

### 2. 服务管理系统 (service.c)
- **服务配置**: `/bin/config/services.ini`
- **支持的服务类型**: daemon, oneshot, forking, notify
- **依赖管理**: 服务可以声明依赖关系
- **自动重启**: 支持配置服务失败时的自动重启

### 2. 守护进程框架 (daemon.c)
- **进程守护化**: 支持将进程转换为后台守护进程
- **PID文件管理**: 自动创建和清理PID文件
- **健康监控**: 监控守护进程状态，支持自动重启
- **信号处理**: 正确的信号处理和清理

### 3. 系统服务

#### 系统日志服务 (syslogd)
- **日志记录**: 接收UDP日志消息并写入文件
- **日志级别**: 支持ERROR, WARN, INFO, DEBUG级别
- **日志文件**: `/var/log/system.log`
- **网络接口**: 监听UDP端口514

#### DHCP客户端 (dhcpd)
- **自动IP配置**: 通过DHCP获取IP地址
- **网络参数**: 获取子网掩码、网关、DNS服务器
- **租约管理**: 基本的租约续订和过期处理
- **错误恢复**: 网络故障时的自动重试

### 4. 增强的Init系统 (init.c)
- **服务管理器**: 替换简单的进程启动器
- **配置文件加载**: 启动时加载服务配置
- **依赖顺序启动**: 按依赖关系启动服务
- **服务监控**: 监控服务状态和健康

## 配置示例

### 服务配置文件 `/bin/config/services.ini`

```ini
[syslogd]
description=System Logging Daemon
command=/bin/syslogd
type=daemon
restart=yes
restart_delay=1000

[dhcpd]
description=DHCP Client Daemon
command=/bin/dhcpd
type=daemon
restart=yes
restart_delay=5000
depends_on=syslogd

[shell]
description=BrightS Command Shell
command=/bin/shell
type=daemon
restart=yes
restart_delay=1000
```

## 启动流程

1. **Init启动**: PID 1进程启动
2. **服务系统初始化**: 初始化服务管理器
3. **文件系统设置**: 创建必要的目录结构
4. **配置文件创建**: 生成默认的服务和用户配置
5. **服务启动**: 按依赖顺序启动核心服务
6. **监控循环**: 持续监控服务状态

## 网络配置

DHCP客户端会在启动后自动：
- 发送DHCPDISCOVER广播
- 接收DHCPOFFER并发送DHCPREQUEST
- 获取DHCPACK并配置网络接口
- 设置IP地址、网关和DNS

## 日志系统

所有系统组件都可以通过UDP发送日志到syslogd：
- 端口: 514
- 协议: UDP
- 格式: `[LEVEL]: message`

## 多语言编程示例

### Rust代码执行
```bash
# 编译Rust代码
echo 'fn main() { println!("Hello Rust!"); }' > hello.rs
rustc hello.rs hello
gcc hello.c -o hello
./hello
```

### Python代码执行
```bash
# 运行Python代码
python -c "print('Hello Python!')"
# 或创建文件
echo "print('Hello from file!')" > hello.py
python hello.py
```

### C++代码执行
```bash
# 编译C++代码
echo '#include <iostream>
int main() { std::cout << "Hello C++!" << std::endl; }' > hello.cpp
g++ hello.cpp hello
gcc hello.c -o hello
./hello
```

### 语言运行时切换
```c
// 在C代码中切换运行时
lang_switch_runtime("rust");
lang_execute_file("program.rs", NULL, NULL);

lang_switch_runtime("python");
lang_execute_string("print('Hello')", "python", "<inline>");
```

## 开发指南

### 添加新守护进程

1. 实现daemon_info_t结构体
2. 定义init_func, main_loop, cleanup_func
3. 在init.c中注册守护进程
4. 添加到服务配置中

### 服务依赖

在服务配置中使用`depends_on`指定依赖：
```
depends_on=service1,service2
```

系统会确保依赖服务先启动。

## 监控和调试

- **服务状态**: 通过service_status()查询
- **日志文件**: 检查/var/log/system.log
- **进程列表**: 使用ps命令查看运行进程
- **网络状态**: 使用ifconfig检查网络配置

## 未来扩展

- 设备热插拔支持 (udevd)
- 定时任务系统 (cron)
- 包管理系统
- 用户账户和权限管理
- 更完整的网络服务 (SSH, HTTP等)