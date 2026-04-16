# BrightS システムリファレンスマニュアル / BrightS 系统参考手册
# バージョン / 版本: 0.1.2.2

## 概要 / 概述

BrightSは完全なUnixライクなオペレーティングシステムで、統一されたコマンドフレームワークと包括的なシステムコールインターフェースを備えています。このドキュメントでは、利用可能なすべてのユーザーコマンドとシステムコールを説明します。

---

## ユーザーコマンド / 用户命令

BrightSはBSPM（BrightS Package Manager）システムを通じて、リッチなユーザー空間コマンドのセットを提供します。

### ファイル操作 / 文件操作

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `ls` | ディレクトリ内容を一覧表示 | `ls [オプション] [パス]` | `ls -l /bin` |
| `cat` | ファイルを連結して表示 | `cat [ファイル]...` | `cat file.txt` |
| `cp` | ファイルとディレクトリをコピー | `cp [オプション] ソース 宛先` | `cp -r src/ dst/` |
| `mv` | ファイルとディレクトリの移動/名前変更 | `mv ソース 宛先` | `mv old.txt new.txt` |
| `rm` | ファイルとディレクトリを削除 | `rm [オプション] ファイル...` | `rm -rf temp/` |
| `mkdir` | ディレクトリを作成 | `mkdir [オプション] ディレクトリ...` | `mkdir -p path/to/dir` |
| `chmod` | ファイル権限を変更 | `chmod モード ファイル...` | `chmod 755 script.sh` |

### テキスト処理 / 文本处理

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `echo` | テキストを表示 | `echo [オプション] 文字列...` | `echo "Hello World"` |
| `grep` | ファイル内でパターンを検索 | `grep [オプション] パターン [ファイル]...` | `grep "error" *.log` |
| `find` | ファイルを検索 | `find パス -name パターン` | `find . -name "*.c"` |

### システム情報 / 系统信息

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `pwd` | 現在のディレクトリを表示 | `pwd` | `pwd` |
| `ps` | プロセス情報を表示 | `ps` | `ps` |
| `jobs` | ジョブ情報を表示 | `jobs` | `jobs` |

### ネットワークツール / 网络工具

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `ping` | ICMPエコー要求を送信 | `ping ホスト` | `ping 192.168.1.1` |

### シェル組み込みコマンド / Shell内置命令

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `cd` | ディレクトリを移動 | `cd [ディレクトリ]` | `cd /home` |
| `help` | コマンドヘルプを表示 | `help [コマンド]` | `help ls` |
| `exit` | シェルを終了 | `exit` | `exit` |

### パッケージ管理 / 包管理

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `bspm install` | パッケージをインストール | `bspm install パッケージ` | `bspm install firefox` |
| `bspm remove` | パッケージを削除 | `bspm remove パッケージ` | `bspm remove firefox` |
| `bspm update` | パッケージリストを更新 | `bspm update` | `bspm update` |
| `bspm upgrade` | すべてのパッケージをアップグレード | `bspm upgrade` | `bspm upgrade` |
| `bspm search` | パッケージを検索 | `bspm search 問い合わせ` | `bspm search editor` |
| `bspm list` | インストール済みパッケージを一覧表示 | `bspm list` | `bspm list` |
| `bspm info` | パッケージ情報を表示 | `bspm info パッケージ` | `bspm info firefox` |
| `bspm clean` | キャッシュをクリア | `bspm clean` | `bspm clean` |

### マルチ言語実行 / 多语言执行

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `rust` | Rustコードを実行 | `rust "コード"` | `rust "println!(\"Hello!\");"` |
| `python` | Pythonコードを実行 | `python "コード"` | `python "print('Hello!')"` |
| `cpp` | C++コードを実行 | `cpp "コード"` | `cpp "std::cout << \"Hello!\";"` |

### 高度なシェル機能 / 高级Shell功能

| 機能 | 説明 | 使用方法 |
|------|------|---------|
| パイプ | コマンド連結 | `ls \| grep "\.c$"` |
| リダイレクト | I/Oリダイレクト | `echo "test" > file.txt` |
| バックグラウンドジョブ | バックグラウンド実行 | `long_command &` |
| コマンド履歴 | 履歴ナビゲーション | `↑` / `↓` 矢印キー |
| 自動補完 | コマンド補完 | `Tab` キー |

---

## システムコール (Syscalls) / 系统调用

BrightSは低レベルシステム操作のための包括的なシステムコールインターフェースを提供します。

### ファイル操作 / 文件操作

| システムコール | 説明 | パラメータ | 戻り値 |
|---------------|------|-----------|--------|
| `sys_open` | ファイルを開く | `const char *path, int flags` | `int fd` |
| `sys_close` | ファイルディスクリプタを閉じる | `int fd` | `int` |
| `sys_read` | ファイルディスクリプタから読み込む | `int fd, void *buf, size_t count` | `ssize_t` |
| `sys_write` | ファイルディスクリプタに書き込む | `int fd, const void *buf, size_t count` | `ssize_t` |
| `sys_stat` | ファイル状態を取得 | `const char *path, uint64_t *size, uint32_t *mode` | `int` |
| `sys_unlink` | ファイルを削除 | `const char *path` | `int` |
| `sys_mkdir` | ディレクトリを作成 | `const char *path` | `int` |
| `sys_readdir` | ディレクトリエントリを読み込む | `int fd, char *buf, size_t size` | `ssize_t` |
| `sys_chmod` | ファイル権限を変更 | `const char *path, uint32_t mode` | `int` |
| `sys_symlink` | シンボリックリンクを作成 | `const char *target, const char *linkpath` | `int` |

### プロセス管理 / 进程管理

| システムコール | 説明 | パラメータ | 戻り値 |
|---------------|------|-----------|--------|
| `sys_fork` | 子プロセスを作成 | `void` | `int64_t pid` |
| `sys_exec` | プログラムを実行 | `const char *path, char *const argv[], char *const envp[]` | `int` |
| `sys_wait` | プロセスの状態変化を待機 | `int64_t *status` | `int64_t pid` |
| `sys_exit` | 呼び出し元のプロセスを終了 | `int status` | `void` |
| `sys_getpid` | プロセスIDを取得 | `void` | `int64_t` |
| `sys_getppid` | 親プロセスIDを取得 | `void` | `int64_t` |

### メモリ管理 / 内存管理

| システムコール | 説明 | パラメータ | 戻り値 |
|---------------|------|-----------|--------|
| `sys_realloc` | メモリを再割り当て | `void *ptr, size_t size` | `void *` |

### プロセス間通信 / 进程间通信

| システムコール | 説明 | パラメータ | 戻り値 |
|---------------|------|-----------|--------|
| `sys_pipe` | パイプを作成 | `int pipefd[2]` | `int` |
| `sys_dup` | ファイルディスクリプタを複製 | `int oldfd` | `int` |
| `sys_dup2` | ファイルディスクリプタを特定のfdに複製 | `int oldfd, int newfd` | `int` |

### 時間とタイミング / 时间和计时

| システムコール | 説明 | パラメータ | 戻り値 |
|---------------|------|-----------|--------|
| `sys_clock_ms` | ミリ秒単位で現在の時刻を取得 | `void` | `int64_t` |
| `sys_sleep_ms` | ミリ秒間スリープ | `uint32_t milliseconds` | `void` |

### ネットワーキング / 网络

| システムコール | 説明 | パラメータ | 戻り値 |
|---------------|------|-----------|--------|
| `sys_ip_parse` | IPアドレス文字列を解析 | `const char *str` | `uint32_t` |
| `sys_icmp_echo` | ICMPエコー要求を送信 | `uint32_t dst_ip` | `int` |

---

## コマンドカテゴリ / 命令分类

コマンドは次のカテゴリに分類されます：

- **CMD_CAT_FILE** (0): ファイル操作 (ls, cp, mv, rm, mkdir, chmod)
- **CMD_CAT_NETWORK** (1): ネットワークツール (ping)
- **CMD_CAT_SYSTEM** (2): システム情報 (ps, jobs)
- **CMD_CAT_UTILITY** (3): テキスト処理とユーティリティ (echo, grep, find, cat)
- **CMD_CAT_DEVEL** (4): 開発ツール
- **CMD_CAT_ADMIN** (5): 管理コマンド（root権限が必要）

---

## エラーコード / 错误代码

一般的なシステムコールのエラーコード：

- `0`: 成功
- `-1`: 全般エラー
- `-2`: ファイルが見つかりません
- `-3`: 権限が拒否されました
- `-4`: 無効な引数
- `-5`: メモリ不足
- `-6`: リソース使用中

---

## BSPMパッケージフォーマット / BSPM包格式

BrightSパッケージは `.bsp` 形式を使用します：

```
package-name-version.bsp/
├── package.info    # パッケージメタデータ / 包元数据
├── files/          # パッケージファイル / 包文件
├── scripts/        # インストールスクリプト / 安装脚本
│   ├── pre-install.sh
│   ├── post-install.sh
│   ├── pre-remove.sh
│   └── post-remove.sh
└── deps/           # 依存関係 / 依赖关系
```

---

## ヘルプの取得 / 获取帮助

- 一般的なヘルプには `help` を実行
- 特定のコマンドのヘルプには `help <コマンド>` を実行
- 詳細なドキュメントは `docs/runtime/README.md` を参照
- BSPM reposで設定されたパッケージリポジトリを訪問

---

*このリファレンスマニュアルは BrightS バージョン 0.1.2.2 に対応しています。最新情報は公式リポジトリを確認してください。*
