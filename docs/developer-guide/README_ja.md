# BrightS 開発者ガイド / BrightS 开发者指南 / BrightS Developer Guide

BrightSオペレーティングシステムの開発者ガイドへようこそ！

## 目次 / 目录 / Contents

### 開発を始める / 开始开发 / Getting Started
- **[🛠️ 開発環境](DEVELOPMENT.md)** - 開発環境のセットアップ
- **[🏗️ プロジェクト構造](PROJECT_STRUCTURE.md)** - コードベースアーキテクチャの理解
- **[⚡ パフォーマンスガイド](PERFORMANCE.md)** - パフォーマンス最適化とベンチマーク

### 貢献 / 贡献 / Contributing
- **[🤝 貢献ガイド](CONTRIBUTING.md)** - 貢献方法（英語）
- **[🤝 貢献ガイド](CONTRIBUTING_ja.md)** - 貢献方法（日本語）
- **[🤝 贡献指南](CONTRIBUTING_zh_CN.md)** - 貢献方法（中文）

### 開発ワークフロー / 开发工作流 / Development Workflow
- コードStandardsと規約
- テスト手順
- コードレビューProcess
- リリース管理

## 🚀 開発環境のセットアップ / 开发环境设置 / Development Setup

### 前提条件 / 先决条件 / Prerequisites

```bash
# 必要なツール / 必需工具 / Required tools
sudo apt install build-essential nasm qemu-system-x86 \
                 git cmake clang lld llvm

# オプションのツール / 可选工具 / Optional tools
sudo apt install gdb valgrind clang-format
```

### BrightSのビルド / 构建 BrightS / Building BrightS

```bash
# クローンとビルド / 克隆并构建 / Clone and build
git clone https://github.com/OpenLight-Studio/BrightS.git
cd BrightS
make all

# QEMUで実行 / 在QEMU中运行 / Run in QEMU
make run

# デバッグビルド / 调试构建 / Debug build
make debug
make run-debug
```

### 開発ワークフロー / 开发工作流 / Development Workflow

1. **リポジトリをFork** / Fork仓库 / Fork the repository
2. **機能ブランチを作成** / 创建功能分支 / Create a feature branch
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **変更を加える** / 进行修改 / Make your changes
4. **変更をテスト** / 测试修改 / Test your changes
   ```bash
   make test
   make run
   ```
5. **ベンチマークを実行** / 运行基准测试 / Run benchmarks
   ```bash
   make benchmark
   ```
6. **プルリクエストを送信** / 提交拉取请求 / Submit a pull request

## 🏗️ アーキテクチャ概要 / 架构概览 / Architecture Overview

### カーネル構造 / 内核结构 / Kernel Structure

```
kernel/
├── arch/          # アーキテクチャ固有のコード / 架构特定代码 / Architecture-specific code
├── core/          # カーネルのコアコンポーネント / 核心内核组件 / Core kernel components
├── drivers/       # デバイスドライバ / 设备驱动 / Device drivers
├── fs/           # ファイルシステム実装 / 文件系统实现 / Filesystem implementations
├── lib/          # カーネルライブラリ / 内核库 / Kernel libraries
├── mm/           # メモリ管理 / 内存管理 / Memory management
└── net/          # ネットワーキングスタック / 网络栈 / Networking stack
```

### ユーザー空間 / 用户空间 / User Space

```
user/
├── libc/         # 標準Cライブラリ / 标准C库 / Standard C library
├── shell/        # コマンドシェル / 命令Shell / Command shell
├── commands/     # 組み込みコマンド / 内置命令 / Built-in commands
└── runtimes/     # 言語ランタイム / 语言运行时 / Language runtimes
```

### 主要コンポーネント / 关键组件 / Key Components

- **UEFIブートローダー**: システム初期化を処理
- **カーネルコア**: プロセススケジューリング、メモリ管理
- **VFS Layer**: 統合ファイルシステムインターフェース
- **コマンドフレームワーク**: 統一コマンドシステム
- **言語ランタイム**: マルチ言語サポート

## 🧪 テスト / 测试 / Testing

### ユニットテスト / 单元测试 / Unit Tests

```bash
# カーネルテストを実行 / 运行内核测试 / Run kernel tests
make test-kernel

# ユーザー空間テストを実行 / 运行用户空间测试 / Run user space tests
make test-user

# すべてのテストを実行 / 运行所有测试 / Run all tests
make test
```

### インテグレーションテスト / 集成测试 / Integration Tests

```bash
# ブートテスト / 引导测试 / Boot test
make test-boot

# ファイルシステムテスト / 文件系统测试 / Filesystem tests
make test-fs

# ネットワークテスト / 网络测试 / Network tests
make test-net
```

### パフォーマンスベンチマーク / 性能基准 / Performance Benchmarks

```bash
# メモリベンチマーク / 内存基准 / Memory benchmarks
make bench-memory

# ファイルシステムベンチマーク / 文件系统基准 / Filesystem benchmarks
make bench-fs

# 完全ベンチマークスイート / 完整基准套件 / Full benchmark suite
make benchmark
```

## 📋 コードStandards / 代码标准 / Code Standards

### 命名規則 / 命名约定 / Naming Conventions

- **関数**: `snake_case` (例: `sys_open`, `kmalloc`)
- **型**: `PascalCase` (例: `Process`, `FileDescriptor`)
- **定数**: `UPPER_SNAKE_CASE` (例: `MAX_PATH`, `PAGE_SIZE`)
- **ファイル**: `snake_case.c/.h`

### ドキュメンテーション / 文档 / Documentation

- すべての公開APIは文書化されていなければならない
- 関数にはDoxygen形式のコメントを使用
- パラメータの説明と戻り値を含める
- エラー条件を文書化

### 例 / 示例 / Example

```c
/**
 * カーネルメモリを割り当て / 分配内核内存 / Allocates kernel memory
 * @param size 割り当てるメモリのサイズ（バイト）/ 要分配的内存大小（字节）/ Size of memory to allocate in bytes
 * @return 割り当てられたメモリへのポインタ、失敗時はNULL / 指向分配内存的指针，失败时为NULL / Pointer to allocated memory, or NULL on failure
 */
void *kmalloc(size_t size) {
    // 実装 / 实现 / Implementation
}
```

## 🔧 開発ツール / 开发工具 / Development Tools

### デバッグ / 调试 / Debugging

```bash
# 実行中のカーネルにGDBをアタッチ / 将GDB附加到运行中的内核 / Attach GDB to running kernel
make debug
gdb kernel.elf

# メモリリーク検出 / 内存泄漏检测 / Memory leak detection
valgrind --tool=memcheck ./test_program

# コードフォーマット / 代码格式化 / Code formatting
clang-format -i *.c *.h
```

### プロファイリング / 性能分析 / Profiling

```bash
# カーネルプロファイリング / 内核性能分析 / Kernel profiling
make profile
./profile_kernel

# メモリ使用量分析 / 内存使用分析 / Memory usage analysis
make memcheck
```

## 🚨 一般的な問題 / 常见问题 / Common Issues

### ビルド問題 / 构建问题 / Build Issues

1. **依存関係が不足**: 必要なツールがすべてインストールされているか確認
2. **コンパイルエラー**: 構文エラーと不足しているインクルードを確認
3. **リンカーエラー**: ライブラリの依存関係とリンク順序を確認

### ランタイム問題 / 运行时问题 / Runtime Issues

1. **ブート失敗**: QEMU設定とカーネルイメージを確認
2. **メモリ破損**: Valgrindでメモリデバッグを使用
3. **ファイルシステム問題**: 異なるファイルシステム設定でテスト

### テスト問題 / 测试问题 / Testing Issues

1. **テスト失敗**: テスト環境と依存関係を確認
2. **パフォーマンス退回**: ベースラインベンチマークと比較
3. **競合状態**: 並行コードにスレッドサニタイザを使用

## 📚 追加リソース / 其他资源 / Additional Resources

- [APIリファレンス](../api-reference/API.md) - 完全なAPIドキュメンテーション
- [ランタイムガイド](../runtime/README.md) - 言語ランタイムドキュメンテーション
- [変更履歴](../CHANGELOG.md) - バージョン履歴と変更点

## 🤝 コミュニティ / 社区 / Community

- **GitHub Issues**: バグ報告と機能リクエスト
- **GitHub Discussions**: 質問とアイデアの議論
- **貢献ガイド**: 効果的な貢献方法を学ぶ

## 📞 サポート / 支持 / Support

開発関連の質問については：
- 既存のissueとdiscussionを確認
- バグや機能リクエストの新しいissueを作成
- コミュニティdiscussionに参加

---

*本開発者ガイドは BrightS バージョン 0.1.2.2 に対応しています*
