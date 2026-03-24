# FAQ

## `argparse-c` はエラー時に終了しますか？

いいえ。パース API は成功時に `0`、失敗時に `-1` を返し、詳細は `ap_error` に格納されます。

## help はどう出しますか？

`-h/--help` は自動追加されます。文字列として取得したい場合は `ap_format_help(...)` を使います。

## `dest` はどう決まりますか？

- optional は long flag を優先
- long flag が無い場合は short flag
- positional は宣言名
- 自動生成時は `-` が `_` に変換されます

## API の詳しい仕様はどこですか？

- 日本語: [API Reference（日本語）](../api-spec.ja.md)
- English: [API Reference（English）](../api-spec.en.md)

## セキュリティ検証に関する現時点の記載は？

- sanitizer と境界ケースを含むテストを CI で継続実行し、coverage を公開しています。
  - CI ワークフロー（tests / sanitizers / coverage）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/ci.yml>
  - Pages ワークフロー（coverage 公開）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/pages.yml>
- セキュリティテスト手順: [セキュリティテスト手順](./security-testing.md)
- 公開時点の既知状態: 公開時点で未修正の重大脆弱性は把握していません。

### 非保証範囲は？

- `argparse-c` を利用する各アプリケーションの入出力検証は、利用側アプリケーションの責務です。
- OS・ツールチェーン・依存バージョンなど環境差異に起因する挙動やリスクは、本ライブラリ単体では完全には保証できません。

### この記載はいつ更新されますか？

- リリースごとに、最新の CI 実行結果とセキュリティテスト手順に基づいて表現を見直します。
