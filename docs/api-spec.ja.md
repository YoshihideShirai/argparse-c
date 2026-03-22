# argparse-c API仕様（日本語）

このドキュメントは `include/argparse-c.h` の公開APIを利用者向けに説明したものです。

## 1. 基本方針
- すべてのパースAPIは、成功時に `0`、失敗時に `-1` を返します。
- 失敗時の詳細は `ap_error` に格納されます。
- ライブラリは `exit()` しません。終了可否は呼び出し側で制御します。

## 2. 型定義

### `ap_type`
- `AP_TYPE_STRING`: 文字列
- `AP_TYPE_INT32`: 32bit整数
- `AP_TYPE_BOOL`: 真偽値

### `ap_action`
- `AP_ACTION_STORE`: 値を格納
- `AP_ACTION_STORE_TRUE`: 指定されたら `true`、未指定なら `false`
- `AP_ACTION_STORE_FALSE`: 指定されたら `false`、未指定なら `true`
- `AP_ACTION_APPEND`: 複数回指定した値を追加
- `AP_ACTION_COUNT`: 指定回数をカウント
- `AP_ACTION_STORE_CONST`: 指定されたら `const_value` を格納

### `ap_nargs`
- `AP_NARGS_ONE`: 値1つ（デフォルト）
- `AP_NARGS_OPTIONAL`: 値0または1つ（`?`）
- `AP_NARGS_ZERO_OR_MORE`: 値0個以上（`*`）
- `AP_NARGS_ONE_OR_MORE`: 値1個以上（`+`）
- `AP_NARGS_FIXED`: `nargs_count` 個ちょうど

### `ap_error_code`
- `AP_ERR_NONE`
- `AP_ERR_NO_MEMORY`
- `AP_ERR_INVALID_DEFINITION`
- `AP_ERR_UNKNOWN_OPTION`
- `AP_ERR_DUPLICATE_OPTION`
- `AP_ERR_MISSING_VALUE`
- `AP_ERR_INVALID_NARGS`
- `AP_ERR_MISSING_REQUIRED`
- `AP_ERR_INVALID_CHOICE`
- `AP_ERR_INVALID_INT32`
- `AP_ERR_UNEXPECTED_POSITIONAL`

### `ap_error`
- `code`: エラーコード
- `argument`: 問題のあった引数名
- `message`: 利用者向けエラーメッセージ

### `ap_arg_options`
`ap_add_argument` で渡すオプションです。

- `type`: 引数の型
- `action`: 代入動作
- `nargs`: 値の個数ルール
- `nargs_count`: `AP_NARGS_FIXED` の固定個数
- `required`: 必須かどうか
- `help`: help出力文
- `metavar`: usage/helpに表示する値名
- `default_value`: 未指定時のデフォルト（文字列として指定）
- `const_value`: `AP_ACTION_STORE_CONST` で格納する値
- `choices`: 許容値一覧（`ap_choices`）
- `dest`: 取得キー名（未指定時は自動生成）

`ap_arg_options_default()` で初期化してから必要項目を上書きする使い方を推奨します。

## 3. パーサ管理API

### `ap_parser *ap_parser_new(const char *prog, const char *description)`
パーサを生成します。

- `prog`: usage先頭に表示するコマンド名
- `description`: help本文の説明
- 戻り値: 生成成功で非NULL、失敗でNULL
- 備考: `-h/--help` は自動で追加されます

### `ap_parser *ap_add_subcommand(ap_parser *parser, const char *name, const char *description, ap_error *err)`
サブコマンド用の子パーサを追加します。

- `name`: サブコマンド名
- `description`: サブコマンドのhelp説明
- 戻り値: 子パーサ（失敗時はNULL）
- 備考:
  - ネストしたサブコマンドを追加できます
  - パース成功時、最上位で選択されたサブコマンド名は namespace の `"subcommand"` に格納されます
  - ネスト経路全体は namespace の `"subcommand_path"` に順序付きで格納されます

### `void ap_parser_free(ap_parser *parser)`
パーサ本体を解放します。

### `ap_mutually_exclusive_group *ap_add_mutually_exclusive_group(ap_parser *parser, bool required, ap_error *err)`
相互排他グループを追加します。

- `required=true` の場合、グループ内のいずれか1つが必須です

### `int ap_group_add_argument(ap_mutually_exclusive_group *group, const char *name_or_flags, ap_arg_options options, ap_error *err)`
相互排他グループに属する引数を追加します。

## 4. 引数定義API

### `int ap_add_argument(ap_parser *parser, const char *name_or_flags, ap_arg_options options, ap_error *err)`
引数定義を追加します。

- `name_or_flags` 例:
  - positional: `"input"`
  - optional: `"-t, --text"`
- 戻り値: 成功 `0` / 失敗 `-1`
- 失敗時: `err` に詳細

## 5. パースAPI

### `int ap_parse_args(ap_parser *parser, int argc, char **argv, ap_namespace **out_ns, ap_error *err)`
厳密モードでパースします。

- 未知オプションはエラー
- 成功時 `*out_ns` に結果が設定される
- `*out_ns` は `ap_namespace_free()` で解放

### `int ap_parse_known_args(ap_parser *parser, int argc, char **argv, ap_namespace **out_ns, char ***out_unknown_args, int *out_unknown_count, ap_error *err)`
known/unknown 分離モードでパースします。

- 未知引数を `out_unknown_args` に回収
- 既知引数は `out_ns` に格納
- unknownは `ap_free_tokens(out_unknown_args, out_unknown_count)` で解放
- `--` 以降のトークンは unknown 側へ回収

## 6. 出力整形API

### `char *ap_format_usage(const ap_parser *parser)`
usage文字列を生成します。

### `char *ap_format_help(const ap_parser *parser)`
help文字列を生成します。

### `char *ap_format_error(const ap_parser *parser, const ap_error *err)`
`error + usage` 形式の文字列を生成します。

#### メモリ所有権（共通）
上記3関数の戻り値はヒープ確保された文字列です。呼び出し側で `free()` してください。

## 7. パース結果取得API

### 単一値取得
- `bool ap_ns_get_bool(const ap_namespace *ns, const char *dest, bool *out_value)`
- `bool ap_ns_get_string(const ap_namespace *ns, const char *dest, const char **out_value)`
- `bool ap_ns_get_int32(const ap_namespace *ns, const char *dest, int32_t *out_value)`

### 複数値取得（`nargs=*` / `+` など）
- `int ap_ns_get_count(const ap_namespace *ns, const char *dest)`
- `const char *ap_ns_get_string_at(const ap_namespace *ns, const char *dest, int index)`
- `bool ap_ns_get_int32_at(const ap_namespace *ns, const char *dest, int index, int32_t *out_value)`

### `void ap_namespace_free(ap_namespace *ns)`
namespaceを解放します。

### `void ap_free_tokens(char **tokens, int count)`
`ap_parse_known_args` で返された unknown 配列を解放します。

## 8. メモリ管理まとめ
- `ap_parser_new` -> `ap_parser_free`
- `ap_parse_args` / `ap_parse_known_args` で得た `ap_namespace*` -> `ap_namespace_free`
- `ap_format_usage/help/error` の戻り値 -> `free`
- `ap_parse_known_args` の `out_unknown_args` -> `ap_free_tokens`

## 9. 最小利用例

```c
ap_error err = {0};
ap_namespace *ns = NULL;
ap_parser *p = ap_parser_new("demo", "demo parser");

ap_arg_options opt = ap_arg_options_default();
opt.required = true;
ap_add_argument(p, "-t, --text", opt, &err);

if (ap_parse_args(p, argc, argv, &ns, &err) != 0) {
  char *msg = ap_format_error(p, &err);
  fprintf(stderr, "%s", msg ? msg : "parse error\n");
  free(msg);
  ap_parser_free(p);
  return 1;
}

/* ... use ap_ns_get_* ... */

ap_namespace_free(ns);
ap_parser_free(p);
```
