# BrightS 文档索引 | Documentation Index

## 快速开始 | Quick Start

- [README.md](../README.md) - 项目概述、功能特性、构建运行指南
- [docs/build/build.md](build/build.md) - 详细构建和运行说明

## 技术文档 | Technical Documentation

### 系统架构 | System Architecture
- [docs/PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) - 项目结构和代码组织
- [docs/design/File_Struct_Define](design/File_Struct_Define) - 文件系统结构定义

### 性能和优化 | Performance & Optimization
- [docs/PERFORMANCE.md](PERFORMANCE.md) - 性能指标和基准测试结果
- [docs/CHANGELOG.md](CHANGELOG.md) - 版本更新日志

## 开发文档 | Development Documentation

### 环境设置 | Environment Setup
- [docs/DEVELOPMENT.md](DEVELOPMENT.md) - 完整的开发环境设置指南

### 贡献指南 | Contributing
- [CONTRIBUTING.md](../CONTRIBUTING.md) - 英文贡献指南
- [docs/CONTRIBUTING_zh_CN.md](CONTRIBUTING_zh_CN.md) - 中文贡献指南

### 开发工具 | Development Tools
- [docs/TODO](TODO) - 待完成任务和开发计划

## API文档 | API Documentation

### 完整API参考 | Complete API Reference
- **[📖 详细API文档](API.md)** - 完整的API函数参考和示例

### 核心系统API | Core System APIs

#### 内存管理 | Memory Management
- `void *brights_kmalloc(size_t size)` - 分配内核内存
- `void brights_kfree(void *ptr)` - 释放内核内存
- `size_t brights_kmalloc_used(void)` - 获取已用内存
- `size_t brights_kmalloc_capacity(void)` - 获取总容量

#### 文件系统 | Filesystem
- `int brights_ramfs_create(const char *name)` - 创建文件
- `int brights_ramfs_open(const char *name)` - 打开文件
- `int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len)` - 读取文件
- `int64_t brights_ramfs_write(int fd, uint64_t off, const void *buf, uint64_t len)` - 写入文件
- `int brights_ramfs_close(int fd)` - 关闭文件

#### 字符串操作 | String Operations
- `char *kutil_strchr(const char *s, int c)` - 查找字符
- `char *kutil_strrchr(const char *s, int c)` - 反向查找字符
- `char *kutil_strstr(const char *haystack, const char *needle)` - 查找子串
- `int kutil_strcmp(const char *a, const char *b)` - 字符串比较

#### 网络操作 | Network Operations
- `int brights_dns_resolve(const char *hostname, uint32_t *ip_out)` - DNS解析
- `int brights_http_init(void)` - HTTP初始化

## 测试套件 | Test Suite

- `test_ramfs` - RAMFS功能测试（7个测试用例）
- `test_benchmark` - 性能基准测试（字符串、文件、内存操作）

## 许可证 | License

本项目采用 GNU GPL v2 许可证。详见 [LICENSE](../LICENSE) 文件。

---

*最后更新时间：2026年4月9日*</content>
<parameter name="filePath">docs/README.md