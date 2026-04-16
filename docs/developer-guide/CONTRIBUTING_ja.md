# BrightS 貢献ガイド / BrightS 贡献指南 / BrightS Contributing Guide

BrightSプロジェクトへの貢献に興味を持っていただきありがとうございます！

## 貢献方法 / 如何贡献 / How to Contribute

### 貢献タイプ / 贡献类型 / Contribution Types

- **バグ修正**: 問題報告と修正
- **新機能**: 新機能の提案と実装
- **ドキュメンテーション**: ドキュメントの改善と翻訳
- **テスト**: テストの作成と改善
- **パフォーマンス**: パフォーマンスの最適化

### 始める前に / 开始之前 / Before You Start

1. プロジェクトの構造を理解する
2. アクティブなissueやPRを確認する
3. 小さな貢献から始める

## 開発ワークフロー / 开发工作流 / Development Workflow

### 1. リポジトリをFork / Fork仓库 / Fork the Repository

```bash
git clone https://github.com/YOUR_USERNAME/BrightS.git
cd BrightS
```

### 2. 機能ブランチを作成 / 创建功能分支 / Create a Feature Branch

```bash
git checkout -b feature/your-feature-name
# または / 或者 / or
git checkout -b fix/your-bug-fix
```

### 3. 変更を加える / 进行修改 / Make Your Changes

```bash
# コードを変更
# Make your code changes

# テストを追加
# Add tests

# ドキュメントを更新
# Update documentation
```

### 4. 変更をテスト / 测试修改 / Test Your Changes

```bash
make test
make run
```

### 5. 変更をコミット / 提交修改 / Commit Your Changes

```bash
git add .
git commit -m "feat: add new feature"
```

### 6. プルリクエストを送信 / 提交拉取请求 / Submit a Pull Request

## コミットメッセージ規則 / 提交消息规则 / Commit Message Rules

```
type: description

types:
- feat: 新機能 / 新功能 / New feature
- fix: バグ修正 / Bug fix
- docs: ドキュメンテーション / Documentation
- style: コードスタイル / Code style
- refactor: リファクタリング / Refactoring
- perf: パフォーマンス / Performance
- test: テスト / Tests
```

## コードStandards / 代码标准 / Code Standards

### 命名規則 / 命名约定 / Naming Conventions

- 関数: `snake_case`
- 型: `PascalCase`
- 定数: `UPPER_SNAKE_CASE`

### ドキュメンテーション / 文档 / Documentation

- すべての公開APIを文書化
- Doxygen形式のコメントを使用
- エラー条件を文書化

## pullリクエストのガイドライン / 拉取请求指南 / Pull Request Guidelines

1. 小さく焦点を当てたPRを作成
2. 関連するテストを含める
3. ドキュメンテーションを更新
4. 清楚なPRの説明を書く

## 報告事項 / 报告问题 / Reporting Issues

- アクティブなissueを確認
- 再現手順を含める
- 期待される動作と実際の動作を説明

## ライセンス / 许可证 / License

貢献することで、コードがGPL v2の下でライセンスされることに同意します。

---

*BrightSへの貢献に感謝します！*
