# BrightS ユーザーガイド / BrightS 用户指南 / BrightS User Guide

BrightSオペレーティングシステムのユーザーガイドへようこそ！

## 目次 / 目录 / Contents

### はじめに / 入门指南 / Getting Started
- [コマンドリファレンス](COMMAND_REFERENCE.md) - 全コマンド一覧（英語）
- [コマンドリファレンス](COMMAND_REFERENCE_ja.md) - 全コマンド一覧（日本語）
- [命令参考手册](COMMAND_REFERENCE_CN.md) - 全命令列表（中文）

### 基本操作 / 基本使用 / Basic Usage
- **シェル操作**: ナビゲーション、ファイル管理、プロセス制御
- **ファイルシステム**: ファイルとディレクトリの操作
- **パッケージ管理**: BSPMによるソフトウェアのインストール与管理

### 高度な機能 / 高级功能 / Advanced Features
- **パイプとリダイレクト**: `command1 | command2`, `command > file`
- **バックグラウンドジョブ**: `&` でコマンドをバックグラウンド実行
- **コマンド履歴**: 矢印キーで履歴を辿る
- **自動補完**: Tabキーによるコマンドとファイル名の補完
- **マルチ言語対応**: Rust、Python、C++でコードを実行

## 🚀 クイックスタートコマンド / 快速开始命令 / Quick Start Commands

```bash
# 基本ナビゲーション / 基本导航 / Basic navigation
pwd              # 現在のディレクトリを表示 / 显示当前目录
ls               # ファイル一覧 / 列出文件
cd /bin          # ディレクトリ移動 / 更改目录

# ファイル操作 / 文件操作 / File operations
cat file.txt     # ファイル内容を表示 / 显示文件内容
cp src dest      # ファイルをコピー / 复制文件
mv old new       # ファイルの移動/名前変更 / 移动/重命名文件
rm file          # ファイルを削除 / 删除文件

# システム情報 / 系统信息 / System information
ps               # プロセスを表示 / 显示进程
jobs             # バックグラウンドジョブを表示 / 显示后台作业

# パッケージ管理 / 包管理 / Package management
bspm update      # パッケージリストを更新 / 更新包列表
bspm install pkg # パッケージをインストール / 安装包
bspm search term # パッケージを検索 / 搜索包

# マルチ言語 / 多语言 / Multi-language
rust 'println!("Hello!");'     # Rustコードを実行 / 运行Rust代码
python 'print("Hello!")'       # Pythonコードを実行 / 运行Python代码
cpp 'cout << "Hello!";'        # C++コードを実行 / 运行C++代码
```

## 🎯 主な機能 / 主要特性 / Key Features

### 統一コマンドシステム / 统一命令系统 / Unified Command System
BrightSは統一されたコマンドフレームワークを使用し、すべてのコマンドが一貫したパターンに従います。

### 高度なシェル / 高级Shell / Advanced Shell
- **コマンド履歴**: ↑/↓矢印キーで履歴を辿る
- **Tab補完**: Tabキーでコマンドとファイル名を自動補完
- **パイプ**: コマンドを連結: `ls | grep "\.c$"`
- **リダイレクト**: 入出力をリダイレクト: `command > output.txt`
- **バックグラウンドジョブ**: `long_command &` でバックグラウンド実行

### パッケージ管理 / 包管理 / Package Management
BrightSにはBSPM（BrightS Package Manager）が含まれています：
- SHA256検証による安全なパッケージインストール
- 依存関係の解決
- リポジトリ管理
- 自動アップデート

### マルチ言語ランタイム / 多语言运行时 / Multi-language Runtime
BrightSは複数のプログラミング言語でコード実行をネイティブサポート：
- **Rust**: 完全なRustコンパイラ統合
- **Python**: 組み込みPythonインタプリタ
- **C++**: C++コンパイルと実行

## 📚 学習リソース / 学习资源 / Learning Resources

- [コマンドリファレンス](COMMAND_REFERENCE.md) - 詳細なコマンドドキュメント（英語）
- [BSPMドキュメント](../../BrightS_Package_Manager/README.md) - パッケージマネージャーガイド
- [ランタイムドキュメント](../runtime/README.md) - マルチ言語サポート

## ❓ ヘルプの取得 / 获取帮助 / Getting Help

```bash
help             # 一般的なヘルプ / 通用帮助
help command     # 特定のコマンドのヘルプ / 特定命令帮助
bspm --help      # BSPMのヘルプ / BSPM帮助
```

## 🆘 トラブルシューティング / 故障排除 / Troubleshooting

### 一般的な問題 / 常见问题 / Common Issues

1. **コマンドが見つかりません**: BSPMパッケージが正しくインストールされているか確認
2. **権限が拒否されました**: 適切なユーザー権限を使用するか管理者として実行
3. **ネットワーク問題**: パッケージダウンロード用のネットワーク接続を確認

### サポート / 支持 / Support
- [GitHub Issues](https://github.com/OpenLight-Studio/BrightS/issues) で問題を報告

---

*本ユーザーガイドは BrightS バージョン 0.1.2.2 に対応しています*
