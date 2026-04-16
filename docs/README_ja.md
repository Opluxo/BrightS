# BrightS ドキュメント / BrightS 文档 / BrightS Documentation

BrightSオペレーティングシステムのドキュメントへようこそ！この包括的なドキュメントは、用户と開発者のために明確なセクションで構成されています。

## 📚 ドキュメント構造 / 文档结构

```
docs/
├── user-guide/           # 🚀 ユーザーガイド / 用户指南
│   ├── COMMAND_REFERENCE.md      # コマンドリファレンス（英語）/ 命令参考手册（英文）
│   ├── COMMAND_REFERENCE_ja.md   # コマンドリファレンス（日本語）/ 命令参考手册（日文）
│   └── COMMAND_REFERENCE_CN.md   # コマンドリファレンス（中国語）/ 命令参考手册（中文）
├── developer-guide/     # 🛠️ 開発者ガイド / 开发者指南
│   ├── README.md              # 開発者ガイド（英語）/ 开发者指南（英文）
│   ├── README_ja.md          # 開発者ガイド（日本語）/ 开发者指南（日文）
│   ├── README_zh_CN.md       # 開発者ガイド（中国語）/ 开发者指南（中文）
│   ├── DEVELOPMENT.md         # 開発環境設定 / 开发环境设置
│   ├── CONTRIBUTING.md        # 貢献ガイド（英語）/ 贡献指南（英文）
│   ├── CONTRIBUTING_ja.md    # 貢献ガイド（日本語）/ 贡献指南（日文）
│   └── CONTRIBUTING_zh_CN.md  # 貢献ガイド（中国語）/ 贡献指南（中文）
├── api-reference/       # 📋 APIリファレンス / API参考
│   └── API.md                    # カーネルAPIドキュメント / 内核API文档
├── runtime/            # 🎯 ランタイムドキュメント / 运行时文档
│   └── README.md                # マルチ言語ランタイム / 多语言运行时
└── README.md           # 📖 ドキュメント首页 / 文档首页
```

## 🚀 クイックスタート / 快速开始

### ユーザー向け / 面向用户
- **[📖 コマンドリファレンス](user-guide/COMMAND_REFERENCE.md)** - 完全なコマンドリファレンス（英語）
- **[📖 命令参考手册](user-guide/COMMAND_REFERENCE_ja.md)** - 完全な命令列表（日文）
- **[📖 命令参考手册](user-guide/COMMAND_REFERENCE_CN.md)** - 完全な命令列表（中文）
- **[🏠 メインREADME](../README.md)** - プロジェクト概要とセットアップ

### 開発者向け / 面向开发者
- **[🛠️ 開発環境](developer-guide/DEVELOPMENT.md)** - 環境セットアップ
- **[🏗️ プロジェクト構造](developer-guide/PROJECT_STRUCTURE.md)** - コードアーキテクチャ
- **[⚡ パフォーマンスガイド](developer-guide/PERFORMANCE.md)** - 最適化Tips

## 📖 ユーザーガイド / 用户指南

BrightSの使用に必要なすべて：

### コマンドとシェル / 命令与Shell
- **[📚 完全なコマンドリファレンス](user-guide/COMMAND_REFERENCE.md)**（英語）
- **[📚 完全な命令列表](user-guide/COMMAND_REFERENCE_ja.md)**（日本語）
- **[📚 完整命令列表](user-guide/COMMAND_REFERENCE_CN.md)**（中文）

### 高度な機能 / 高级功能
- **パイプとリダイレクト**: `cmd1 | cmd2`, `cmd > file`, `cmd < input`
- **バックグラウンドジョブ**: `command &`, `jobs` コマンド
- **コマンド履歴**: 矢印キーナビゲーション
- **自動補完**: Tabキー補完
- **マルチ言語**: `rust "code"`, `python "code"`, `cpp "code"`

## 🛠️ 開発者ガイド / 开发者指南

### 開発を始める / 开始开发
- **[🛠️ 開発環境](developer-guide/DEVELOPMENT.md)**
  - 必要なツールと依存関係
  - ビルドシステムセットアップ
  - テスト環境

### アーキテクチャと設計 / 架构与设计
- **[🏗️ プロジェクト構造](developer-guide/PROJECT_STRUCTURE.md)**
  - カーネリアーキテクチャ概要
  - コード編成
  - モジュール構造

### パフォーマンスと最適化 / 性能与优化
- **[⚡ パフォーマンスガイド](developer-guide/PERFORMANCE.md)**
  - ベンチマーク結果
  - 最適化技法
  - メモリ管理

### コード貢献 / 贡献代码
- **[🤝 貢献ガイド](developer-guide/CONTRIBUTING.md)**（英語）
- **[🤝 貢献ガイド](developer-guide/CONTRIBUTING_ja.md)**（日本語）
- **[🤝 贡献指南](developer-guide/CONTRIBUTING_zh_CN.md)**（中文）

## 📋 APIリファレンス / API参考

### カーネルAPI / 内核API
- **[📖 完全なAPIリファレンス](api-reference/API.md)**
  - メモリ管理関数
  - ファイルシステム操作
  - 文字列ユーティリティ
  - ネットワーク関数

### システムコール / 系统调用
上記のコマンドリファレンスには、すべてのパラメータと戻り値を含む利用可能なシステムコールが文書化されています。

## 🎯 ランタイムドキュメント / 运行时文档

マルチ言語サポートとランタイムシステム：

- **[🌍 ランタイムガイド](runtime/README.md)**
  - Rustランタイム統合
  - Pythonインタプリタ
  - C++コンパイルサポート

## 📊 追加リソース / 其他资源

- **[📝 変更履歴](CHANGELOG.md)** - バージョン履歴
- **[🔧 ビルドガイド](build/build.md)** - 詳細なビルド手順
- **[🎯 TODOリスト](TODO)** - 開発ロードマップ

## 🧪 テスト / 测试

- `test_ramfs` - RAMファイルシステムテスト（7テストケース）
- `test_benchmark` - パフォーマンスベンチマーク（文字列、ファイル、メモリ操作）

## 🌍 言語サポート / 语言支持

このドキュメントは次の言語で利用可能です：
- **English**（英語）（主言語）
- **日本語**（日本語）
- **简体中文**（簡体字中国語）

## 📞 サポートとコミュニティ / 支持与社区

- **🐛 Issues**: [GitHub Issues](https://github.com/OpenLight-Studio/BrightS/issues)
- **💬 Discussions**: [GitHub Discussions](https://github.com/OpenLight-Studio/BrightS/discussions)
- **📖 ドキュメントの問題**: [ここに報告](https://github.com/OpenLight-Studio/BrightS/issues/new?labels=documentation)

## 📝 ドキュメントへの貢献 / 文档贡献

ドキュメントの改善を歓迎します！詳細は[貢献ガイド](developer-guide/CONTRIBUTING.md)を参照。

## 📄 ライセンス / 许可证

このプロジェクトはGNU GPL v2ライセンスの下でライセンスされています。詳細については[LICENSE](../LICENSE)を参照。

---

*最終更新: 2026-04-17*
