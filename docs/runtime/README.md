# BrightS 用户空间运行时环境

BrightS 提供完整的用户空间运行时，包括服务管理、守护进程框架、系统服务及多语言编程支持。

## 核心组件

### 1. 多语言运行时系统
语言管理器 (`lang_runtime.c/h`) 统一注册和切换语言运行时，支持：

- **Rust** (`rust_runtime.c/h`) — `rustc` 转 C 编译，支持函数、println!、变量绑定
- **Python** (`python_runtime.c/h`) — 解释器，支持 print/len/type 等内建函数
- **C++** (`cpp_runtime.c/h`) — `g++` 转 C 编译，支持 iostream/string/vector 及基本异常

运行时通过 `lang_switch_runtime("rust|python|cpp")` 切换，通过文件扩展名自动识别。

### 2. 服务管理系统
- 配置：`/bin/config/services.ini`
- 服务类型：daemon / oneshot / forking / notify
- 支持依赖声明 (`depends_on`) 和失败自动重启
- 守护进程框架 (`daemon.c`)：PID 文件管理、健康监控、信号处理

### 3. 系统服务
- **syslogd** — UDP 514 接收日志，写入 `/var/log/system.log`，级别 ERROR/WARN/INFO/DEBUG
- **dhcpd** — DHCP 客户端，自动获取 IP/掩码/网关/DNS

### 4. Init 系统
PID 1 进程，按依赖顺序启动服务，持续监控服务状态。启动流程：Init 启动 → 服务系统初始化 → 文件系统创建 → 加载配置 → 启动服务 → 监控循环。

## 启动流程

1. Init 启动 (PID 1)
2. 服务系统初始化
3. 创建目录结构
4. 生成默认配置
5. 按依赖顺序启动服务
6. 持续监控

## 配置示例 (`/bin/config/services.ini`)

```ini
[syslogd]
command=/bin/syslogd
type=daemon
restart=yes

[dhcpd]
command=/bin/dhcpd
type=daemon
restart=yes
depends_on=syslogd
```

## Shell 功能

支持命令历史 (↑↓)、Tab 补全、管道 (|)、I/O 重定向 (< > >>)、后台作业 (&)、内联多语言执行。
