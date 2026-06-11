# BrightS コマンドリファレンス / BrightS 命令参考

バージョン: 0.1.2.6

## ユーザーコマンド / 用户命令

### ファイル操作 / 文件操作

| コマンド | 説明 | 構文 |
|---------|------|------|
| `ls` | ディレクトリ内容を表示 | `ls [options] [path]` |
| `cat` | ファイルを表示 | `cat <file>...` |
| `cp` | ファイル/ディレクトリをコピー | `cp <src> <dst>` |
| `mv` | 移動/名前変更 | `mv <src> <dst>` |
| `rm` | ファイル/ディレクトリを削除 | `rm [options] <file>...` |
| `mkdir` | ディレクトリを作成 | `mkdir [options] <dir>...` |
| `chmod` | パーミッションを変更 | `chmod <mode> <file>...` |

### テキスト処理 / 文本处理

| コマンド | 説明 | 構文 |
|---------|------|------|
| `echo` | テキストを表示 | `echo <string>...` |
| `grep` | ファイル内を検索 | `grep [options] <pattern> [file]...` |
| `find` | ファイルを名前で検索 | `find <path> -name <pattern>` |

### システム情報 / 系统信息

| コマンド | 説明 | 構文 |
|---------|------|------|
| `pwd` | カレントディレクトリを表示 | `pwd` |
| `ps` | プロセスを表示 | `ps` |
| `jobs` | バックグラウンドジョブを表示 | `jobs` |

### ネットワーク / 网络

| コマンド | 説明 | 構文 |
|---------|------|------|
| `ping` | ICMPエコー要求 | `ping <host>` |

### シェル組み込み / Shell内置

| コマンド | 説明 | 構文 |
|---------|------|------|
| `cd` | ディレクトリを移動 | `cd [dir]` |
| `help` | ヘルプを表示 | `help [command]` |
| `exit` | シェルを終了 | `exit` |

### パッケージ管理 (BSPM) / 包管理

| コマンド | 説明 | 構文 |
|---------|------|------|
| `bspm install` | パッケージをインストール | `bspm install <package>` |
| `bspm remove` | パッケージを削除 | `bspm remove <package>` |
| `bspm update` | パッケージリストを更新 | `bspm update` |
| `bspm upgrade` | 全パッケージをアップグレード | `bspm upgrade` |
| `bspm search` | パッケージを検索 | `bspm search <query>` |
| `bspm list` | インストール済み一覧 | `bspm list` |
| `bspm info` | パッケージ情報を表示 | `bspm info <package>` |
| `bspm clean` | キャッシュをクリア | `bspm clean` |

### マルチ言語実行 / 多语言执行

| コマンド | 説明 | 構文 | 例 |
|---------|------|------|-----|
| `rust` | Rustコードを実行 | `rust '<code>'` | `rust 'println!("hi");'` |
| `python` | Pythonコードを実行 | `python '<code>'` | `python 'print("hi")'` |
| `cpp` | C++コードを実行 | `cpp '<code>'` | `cpp 'cout << "hi";'` |
