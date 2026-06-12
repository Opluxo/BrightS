# BrightS 貢献ガイド

[EN](CONTRIBUTING.md) · [ZH](CONTRIBUTING_zh_CN.md)

## 貢献方法

1. Fork & clone
2. `git checkout -b feature/name`
3. 変更を加える
4. `make test` — 全テスト通過必須
5. PRを送信

## コミット形式

```
feat: 新機能
fix: バグ修正
docs: ドキュメント
refactor: リファクタリング
perf: パフォーマンス
test: テスト
style: フォーマット
ci: CI/CD
chore: メンテナンス
```

## コードスタイル

- 既存のコード規則に従う
- 公共APIには `brights_` プレフィックスを使用
- 関数は小さく焦点を絞る
- 複雑なロジックにはコメントを追加

## ライセンス

貢献により GPL v2 に同意したものとみなします。

---

[GitHub](https://github.com/Opluxo/BrightS) · [Docs](https://opluxo.github.io/BrightS/)