# 性能指标 | Performance Metrics

> v0.1.2.7 · 目标硬件: Intel i5 1135G7 · 8GB RAM

## 关键指标 | Key Metrics

| 指标 | 值 | 说明 |
|------|----|------|
| 内核大小 (UEFI) | ~176KB | clang + LLD, LTO优化 |
| 内核大小 (BIOS) | 1.44MB | floppy image |
| UEFI初始化 | <1s | |
| 内核初始化 | <1s | |
| Shell就绪 | <2s | 总计 |
| 内核堆 | 8MB | Slab分配器 |
| 静态分配 | <1MB | |
| 运行时峰值 | <4MB | |

## 性能基准 | Benchmarks

| 项目 | 特性 | 复杂度 |
|------|------|--------|
| 内存分配 | Slab分配器，固定大小块 | O(1) 最坏情况 |
| 多页分配 | 提示缓存，跳过已扫描区域 | O(n) 最坏，O(1) 平均 |
| 调度器 | Round-robin，优先级动态调整 | O(1) |
| PID分配 | 位图 + BSF | O(1) |
| slab释放 | 页→类哈希表查找 | O(1) |
| RAMFS | 零拷贝优化 | O(1) |
| 路径解析 | VFS路径缓存（16条目） | O(n) |
| 字符串操作 | 内联实现，边界检查 | O(n) |
| DNS解析 | LRU缓存 | O(1) 缓存命中 |
| ARP缓存 | TTL过期（30秒） | O(1) |
| 数据包处理 | VirtIO-Net，批量处理 | O(1) |
| TCP重传 | 指数退避，最多5次重试 | O(1) |
| 时钟精度 | RDTSCP序列化，纳秒级 | O(1) |

## SIMD 优化 | SIMD Optimizations

| 指令集 | 用途 |
|--------|------|
| SSE2 | memcpy, memset, memcmp |
| ERMS | 增强 REP MOVSB/STOSB |
| CRC32 | 硬件 CRC32 加速 |

## LRU 缓存系统 | LRU Cache System

| 缓存 | 用途 | 默认大小 |
|------|------|----------|
| DNS | 域名解析结果 | 可配置TTL |
| Path | VFS路径解析 | 16条目 |
| Inode | 索引节点 | 可配置 |
| Buffer | 块设备缓冲 | 可配置 |

## 优化历程 | Optimization History

| 阶段 | 大小 | 说明 |
|------|------|------|
| 1 | 97KB | 基础功能实现 |
| 2 | 96KB | 警告清理 + 代码优化 |
| 3 | 93KB | LTO + 深度优化 |
| 4 | ~176KB | 完整功能（UEFI） |

*最后更新：2026-06-11*