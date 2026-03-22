# known args / unknown args

`ap_parse_known_args(...)` は、既知引数だけをパースし、未知トークンを別配列へ回収したいときに使います。

## 使いどころ

- ラッパー CLI を作る場合
- 一部だけ自前で解釈し、残りは別コマンドへ渡す場合
- `--` 以降をそのまま保持したい場合

## API

```c
ap_parse_known_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err);
```

## unknown 側に回収されるもの

- 未知オプション
- 未知オプションの直後にある値らしいトークン
- positional 割り当て後に余ったトークン
- `--` 以降のすべてのトークン

## メモリ解放

unknown 配列は `ap_free_tokens(...)` で解放します。

```c
ap_free_tokens(unknown, unknown_count);
```
