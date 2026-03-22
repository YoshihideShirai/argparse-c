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

- 日本語: [API Reference（日本語）](api-spec.ja.md)
- English: [API Reference（English）](api-spec.en.md)
