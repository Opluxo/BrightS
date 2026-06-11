# BrightS ユーザーガイド / BrightS 用户指南 / BrightS User Guide

バージョン: 0.1.2.6

## クイックスタート / 快速开始 / Quick Start

| カテゴリ | コマンド | 説明 |
|----------|---------|------|
| ナビゲーション | `pwd`, `ls`, `cd <dir>` | ディレクトリ操作 |
| ファイル | `cat`, `cp`, `mv`, `rm`, `mkdir` | ファイル管理 |
| テキスト | `echo`, `grep <pat> <file>` | テキスト処理 |
| システム | `ps`, `jobs`, `uname`, `bst` | プロセス・システム情報 |
| ネットワーク | `ping`, `netget <url>` | ICMP・HTTP取得 |
| 環境変数 | `export KEY=VAL`, `env` | 環境変数 |
| パッケージ | `bspm install/search/update` | パッケージ管理 |
| 言語 | `rust`, `python`, `cpp` | コード実行 |

## 目次 / 目录 / Contents

- [コマンドリファレンス (英語)](COMMAND_REFERENCE.md)
- [命令参考手册 (中文)](COMMAND_REFERENCE_CN.md)
- [コマンドリファレンス (日本語)](COMMAND_REFERENCE_ja.md)

## 機能 / 特性 / Features

- **シェル**: パイプ、リダイレクト、バックグラウンド、Tab補完、履歴
- **環境変数**: `export KEY=VALUE`、`env`、echo内の`$VAR`展開
- **ネットワーク**: `netget <url>` でシェルからHTTPコンテンツを取得
- **BSPM**: インストール/削除/更新/検索、依存関係解決
- **多言語**: Rust、Python、C++のインライン実行

## ヘルプ / 帮助 / Help

- `help` — 一般的なヘルプ
- `help <command>` — 特定のコマンドのヘルプ
- `bspm --help` — BSPMのヘルプ