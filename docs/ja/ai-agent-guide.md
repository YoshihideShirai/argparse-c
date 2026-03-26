# AI Agent Guide

このページは、`argparse-c` を使うコードを AI エージェントが生成・修正するときの最小ルール集です。

## 最初の読む順（推奨）

1. [`../api-spec.json`](../api-spec.json) — 機械可読の API 署名と所有権情報。
2. [`getting-started.md`](getting-started.md) — parser の基本ライフサイクル。
3. [`guides/basic-usage.md`](guides/basic-usage.md) — parse/エラー処理の実践パターン。
4. [`guides/known-args.md`](guides/known-args.md) — wrapper CLI の unknown 引数転送。

## 所有権 / 後始末チェックリスト

### `ap_format_*` の戻り値（`char *`）

formatter API が返す文字列はすべてヒープ確保です。`NULL` でない場合は呼び出し側が `free()` します。

- `ap_format_usage(...)`
- `ap_format_help(...)`
- `ap_format_manpage(...)`
- `ap_format_bash_completion(...)`
- `ap_format_fish_completion(...)`
- `ap_format_zsh_completion(...)`
- `ap_format_error(...)`

### parser / namespace の解放順序

次の順序を一貫して使ってください。

1. `ap_parser *parser = ap_parser_new(...)`
2. parse して `ap_namespace *ns` を取得
3. 先に namespace を解放: `ap_namespace_free(ns)`
4. 最後に parser を解放: `ap_parser_free(parser)`

`ns` は parser から得た派生オブジェクトとして、先に `ns` を片付ける方針に統一します。

### エラーパス後始末マトリクス

| リソース状態 | 必須の後始末 |
| --- | --- |
| parser 作成失敗（`parser == NULL`） | エラー終了（解放不要） |
| parser 作成済み・parse 失敗 | `ap_format_error(...)` の戻り値を `free(...)`、その後 `ap_parser_free(parser)` |
| parser 作成済み・parse 成功 | `ap_namespace_free(ns)` → `ap_parser_free(parser)` |
| known-args parse 成功 | 上記に加えて `ap_free_tokens(unknown, unknown_count)` |

## `ap_parse_args` と `ap_parse_known_args` の使い分け（wrapper CLI 前提）

**`ap_parse_args`** は、CLI 自身が全引数契約を管理し、未知オプションを即座にエラーにしたい場合に使います。

**`ap_parse_known_args`** は、CLI が **wrapper / front controller** として一部だけ解釈し、残りを下流コマンドへ転送する場合に使います。

wrapper CLI では次を基本方針にします。

- wrapper 側の引数だけ `ap_parse_known_args(...)` で処理
- `unknown[]` は順序を保ったまま転送
- 転送処理後に `ap_free_tokens(...)` で解放

## 「良い例 / 悪い例」（`sample/` パターン由来の最小断片）

### 良い例: parse 失敗時にエラー文字列と parser を解放

```c
ap_error err = {0};
ap_namespace *ns = NULL;
ap_parser *parser = ap_parser_new("demo", "demo parser");
if (!parser) return 1;

if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *msg = ap_format_error(parser, &err);
  fprintf(stderr, "%s", msg ? msg : err.message);
  free(msg);
  ap_parser_free(parser);
  return 1;
}

ap_namespace_free(ns);
ap_parser_free(parser);
```

### 良い例: `ap_format_help` の戻り値を解放

```c
char *help = ap_format_help(parser);
if (help) {
  fputs(help, stdout);
  free(help);
}
```

### 良い例: wrapper CLI の known/unknown 分離と後始末

```c
char **unknown = NULL;
int unknown_count = 0;
if (ap_parse_known_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err) == 0) {
  /* unknown[0..unknown_count-1] を下流へ転送 */
  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
}
ap_parser_free(parser);
```

### 悪い例: formatter 文字列リーク

```c
char *msg = ap_format_error(parser, &err);
fprintf(stderr, "%s", msg ? msg : "error");
/* free(msg); がない */
```

### 悪い例: 成功パスで解放順が逆

```c
ap_parser_free(parser);
ap_namespace_free(ns); /* この順は避ける */
```
