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
- `AP_TYPE_INT64`: 64bit整数
- `AP_TYPE_UINT64`: 64bit符号なし整数
- `AP_TYPE_DOUBLE`: 倍精度浮動小数点
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

| コード | 主な発生契機 | `err.argument` の契約 | `err.message` の系統 |
| --- | --- | --- | --- |
| `AP_ERR_NONE` | エラー未設定 | 空文字 | 空文字 |
| `AP_ERR_NO_MEMORY` | parser/build/format 系の確保失敗 | 失敗元の `dest`、処理中 token、または parser 全体失敗時の空文字 | `out of memory` |
| `AP_ERR_INVALID_DEFINITION` | API 呼び出しや定義内容が不正 | 不正 flag/name/dest、最初の衝突 flag、または parser/group 全体の前提条件違反時の空文字 | `store_const requires const_value` のような定義時診断 |
| `AP_ERR_UNKNOWN_OPTION` | strict parse で未知 option を検出 | 入力された option token そのもの（例: `--bogus`, `-z`） | `unknown option '...'` |
| `AP_ERR_DUPLICATE_OPTION` | 非反復 option の重複指定 | その option の primary flag | `duplicate option '...'` |
| `AP_ERR_MISSING_VALUE` | 値必須の option/argument に値が無い | optional は primary flag、positional は宣言名 | `option '...' requires a value` / `argument '...' requires a value` |
| `AP_ERR_INVALID_NARGS` | `nargs` 個数規則違反や short cluster 不正 | optional は primary flag、positional は宣言名 | `... requires at least one value`、`... requires exactly N values`、`option '...' cannot be used in a short option cluster` |
| `AP_ERR_MISSING_REQUIRED` | 必須 option/argument/subcommand/group が不足 | primary flag、positional 名、`subcommand`、または group 全体失敗時の空文字 | `option '...' is required`、`argument '...' is required`、`subcommand is required` など |
| `AP_ERR_INVALID_CHOICE` | `choices` 外の値を検出 | optional は primary flag、positional は宣言名 | `invalid choice 'X' for option '...'` / `... for argument '...'` |
| `AP_ERR_INVALID_INT32` | 32bit整数変換失敗 | optional は primary flag、positional は宣言名 | `argument '...' must be a valid int32: 'X'` |
| `AP_ERR_INVALID_INT64` | 64bit整数変換失敗 | optional は primary flag、positional は宣言名 | `argument '...' must be a valid int64: 'X'` |
| `AP_ERR_INVALID_UINT64` | 64bit符号なし整数変換失敗 | optional は primary flag、positional は宣言名 | `argument '...' must be a valid uint64: 'X'` |
| `AP_ERR_INVALID_DOUBLE` | 浮動小数点変換失敗 | optional は primary flag、positional は宣言名 | `argument '...' must be a valid double: 'X'` |
| `AP_ERR_UNEXPECTED_POSITIONAL` | strict mode で余分な positional token が残った | 余分だった token そのもの | `unexpected positional argument 'TOKEN'` |

### `ap_error`
- `code`: エラーコード
- `argument`: 問題のあった引数を表す安定した識別子
  - optional 引数では parser 定義に保持された primary flag（`flags[0]`）を使います
  - positional 引数では宣言名（flag を持たない場合の `dest`）を使います
  - subcommand 必須エラーでは `subcommand` を使います
  - parser/group 全体に関わるエラーでは空文字の場合があります
  - out-of-memory 系では、利用者向けラベルではなく失敗源の `dest` や token を入れて相関付けできるようにする場合があります
- `message`: 人間向けメッセージで、テンプレート系統を安定化します
  - optional 引数は `option '...'`
  - positional 引数は `argument '...'`
  - parser/group 全体の失敗は引数ラベルを付けず文として表現します

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
- `completion_kind`: 静的な補完メタデータ（`ap_completion_kind`）
- `completion_hint`: 補完生成器向けの任意ヒント文字列
- `completion_callback`: 動的補完候補を返す任意のアプリケーションコールバック
- `completion_user_data`: `completion_callback` にそのまま渡される任意ポインタ
- `dest`: 取得キー名（未指定時は自動生成）

`ap_arg_options_default()` で初期化してから必要項目を上書きする使い方を推奨します。

### `ap_completion_kind`
- `AP_COMPLETION_KIND_NONE`: 明示的な補完メタデータなし
- `AP_COMPLETION_KIND_CHOICES`: `choices` を補完元として使う
- `AP_COMPLETION_KIND_FILE`: ファイルパスを補完する
- `AP_COMPLETION_KIND_DIRECTORY`: ディレクトリを補完する
- `AP_COMPLETION_KIND_COMMAND`: 実行可能コマンド名を補完する

#### 補完メタデータのルール
- bash / fish 生成器は `completion_kind` が `AP_COMPLETION_KIND_NONE` 以外ならそれを優先します
- `completion_kind == AP_COMPLETION_KIND_CHOICES` の場合、補完元として `choices` を使います
- `completion_kind == AP_COMPLETION_KIND_NONE` で `choices` が設定されている場合は、デフォルトの `CHOICES` 挙動として扱います
- コールバック型の動的補完APIは公開されており、静的メタデータを補完する仕組みとして使えます
- `completion_hint` は生成器固有ヒントのための予約フィールドで、parse時の検証動作は変えません

#### 動的補完コールバック契約
公開 API では、`completion_callback`、`completion_user_data`、`ap_complete(...)`
による実行時補完もサポートします。

- コールバック入力は読み取り専用で、parser path・対象 option/argument・現在のトークン接頭辞・shell 文脈を表します
- コールバック出力は append-only な候補列で、返却後の所有権は呼び出し側にあります
- 戻り値はライブラリ共通規約どおり、成功で `0`、失敗で `-1`、詳細は `ap_error` に入ります
- `completion_user_data` はライブラリが解釈せず、そのまま保存・転送するアプリケーション状態です
- `completion_kind` / `choices` / `completion_hint` は、コールバック併用時も静的フォールバックとして有効です
- 優先順位は、`completion_callback` による動的候補が先、候補が無い場合は静的メタデータへのフォールバックです

#### `dest` 自動生成ルール
- optional 引数では、最初に見つかった long flag（例: `--dry-run`）を優先します
- long flag が無い場合は、最初の short flag（例: `-v`）を使います
- positional 引数では、宣言名そのものを使います
- 自動生成時は `-` を `_` に変換します（例: `dry-run` -> `dry_run`）
- `dest` を明示指定した場合、その値は変更されません

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
  - パース成功時、最終的に選択された **leaf の** サブコマンド名が namespace の `"subcommand"` に格納されます
  - 途中階層のサブコマンド名は別 namespace key として追加されません
  - namespace には `"subcommand_path"` も格納され、選択されたサブコマンド列全体（例: `"config set"`）を保持します
  - ネストしたサブコマンド parser に対する `ap_format_help()` は、usage/help にフルコマンドパスを表示します

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
- unknown は出現順で `out_unknown_args` に追加されます
- unknown として回収されるもの:
  - 未知オプション
  - 未知オプションの直後にある、別のオプションらしくない値トークン
  - positional 割り当て後に余ったトークン
  - `--` 以降のすべてのトークン
- known 側には通常どおり検証がかかるため、必須不足や既知引数の不正値では失敗します

#### `nargs` の挙動まとめ
- optional 引数:
  - `AP_NARGS_ONE`: 値をちょうど1つ要求
  - `AP_NARGS_OPTIONAL`: 次トークンが既知オプションでないときだけ消費
  - `AP_NARGS_ZERO_OR_MORE` / `AP_NARGS_ONE_OR_MORE`: `--` または次の既知オプション直前まで消費
  - `AP_NARGS_FIXED`: `nargs_count` 個ちょうど要求
- positional 引数:
  - 左から順に割り当てます
  - `AP_NARGS_OPTIONAL` / `AP_NARGS_ZERO_OR_MORE` / `AP_NARGS_ONE_OR_MORE` は、後続 positional の最小必要数を残すように割り当てます

## 6. 出力整形API

### `char *ap_format_usage(const ap_parser *parser)`
usage文字列を生成します。

### `char *ap_format_help(const ap_parser *parser)`
help文字列を生成します。

### `char *ap_format_error(const ap_parser *parser, const ap_error *err)`
`error + usage` 形式の文字列を生成します。

### Shell completion formatter API
- `char *ap_format_bash_completion(const ap_parser *parser)`
  - parser メタデータから bash completion script を生成します。
  - 公開導線の代表例は `sample/example_completion.c` / `sample/example_manpage.c` の `--generate-bash-completion` です。
- `char *ap_format_fish_completion(const ap_parser *parser)`
  - 同じ parser 定義から fish completion script を生成します。
  - 公開導線の代表例は `sample/example_completion.c` / `sample/example_manpage.c` の `--generate-fish-completion` です。
- `char *ap_format_zsh_completion(const ap_parser *parser)`
  - 同じ parser 定義から zsh completion script を生成します。
  - 公開導線の代表例は `sample/example_completion.c` / `sample/example_manpage.c` の `--generate-zsh-completion` です。

#### メモリ所有権（共通）
上記3関数の戻り値はヒープ確保された文字列です。呼び出し側で `free()` してください。

## 7. パース結果取得API

### 単一値取得
- `bool ap_ns_get_bool(const ap_namespace *ns, const char *dest, bool *out_value)`
- `bool ap_ns_get_string(const ap_namespace *ns, const char *dest, const char **out_value)`
- `bool ap_ns_get_int32(const ap_namespace *ns, const char *dest, int32_t *out_value)`
- `bool ap_ns_get_int64(const ap_namespace *ns, const char *dest, int64_t *out_value)`
- `bool ap_ns_get_uint64(const ap_namespace *ns, const char *dest, uint64_t *out_value)`
- `bool ap_ns_get_double(const ap_namespace *ns, const char *dest, double *out_value)`

### 複数値取得（`nargs=*` / `+` など）
- `int ap_ns_get_count(const ap_namespace *ns, const char *dest)`
- `const char *ap_ns_get_string_at(const ap_namespace *ns, const char *dest, int index)`
- `bool ap_ns_get_int32_at(const ap_namespace *ns, const char *dest, int index, int32_t *out_value)`
- `bool ap_ns_get_int64_at(const ap_namespace *ns, const char *dest, int index, int64_t *out_value)`
- `bool ap_ns_get_uint64_at(const ap_namespace *ns, const char *dest, int index, uint64_t *out_value)`
- `bool ap_ns_get_double_at(const ap_namespace *ns, const char *dest, int index, double *out_value)`

### `void ap_namespace_free(ap_namespace *ns)`
namespaceを解放します。

### `void ap_free_tokens(char **tokens, int count)`
`ap_parse_known_args` で返された unknown 配列を解放します。

## 8. メモリ管理まとめ
- `ap_parser_new` -> `ap_parser_free`
- `ap_parse_args` / `ap_parse_known_args` で得た `ap_namespace*` -> `ap_namespace_free`
- `ap_format_usage/help/error` の戻り値 -> `free`
- `ap_parse_known_args` の `out_unknown_args` -> `ap_free_tokens`

## 8.1 エラー生成と命名規則

現在の実装 (`lib/core_parser.c`、`lib/core_validate.c`、`lib/core_convert.c`、`lib/api.c`) では、次の命名規則で概ね統一されています。

- optional 引数の parse/validation エラーは、`err.argument` に primary flag（`flags[0]`）を入れます。
  - そのため `-t, --text` は short flag を先に宣言していれば `-t`、`--mode, -m` のように long flag を先に宣言していれば `--mode` が返ります。
- positional 引数の parse/validation エラーは、宣言した positional 名をそのまま使います。
- `invalid choice` はラベル生成関数経由なので、引数種別に応じて `for option '...'` / `for argument '...'` が切り替わります。
- `required`、`missing value`、`invalid nargs` も同じラベル生成を使うため、parse 時と validate 時で `option '...'` / `argument '...'` の言い回しが揃います。
- 整数変換失敗だけは現在 `argument '...' must be a valid int32: 'VALUE'` で固定されており、optional flag でも名詞は `argument` になります。
- `lib/api.c` の定義時エラーは、まだ正規化済み引数オブジェクトが無い場合があるため、`name_or_flags`、衝突した `dest`、subcommand 名、空文字などの生値をそのまま使うことがあります。
- parser 全体や相互排他グループ全体の失敗では、意図的に `err.argument` を空にし、文だけのメッセージを返します。

### `ap_format_error` の出力契約
`ap_format_error()` は `err.message` の前に `error: ` を付け、その後ろに改行を1つ入れ、同じ parser から生成した usage を連結します。

例（`prog=demo`、必須 option `--mode` が不足した場合）:

```text
error: option '--mode' is required
usage: demo [-h] --mode MODE
```

CLI 出力では、この「`error:` 行 + usage ブロック」の2段構成を安定した表示契約として扱ってください。

## 8.2 エラーメッセージ整合性の回帰テスト計画
将来のリファクタリングでも命名規則が崩れないよう、次の観点で回帰カバレッジを維持します。

- `test/test_parse_core.cpp`
  - strict parse の未知 option、int32 変換失敗、invalid choice、必須 option 不足、short cluster の `nargs` エラー
- `test/test_known_args.cpp`
  - known/unknown 分離時の必須 option エラーと、unknown を保持しても primary flag 名が変わらないこと
- `test/test_subcommands_and_validation.cpp`
  - missing value、`nargs` 個数違反、unexpected positional、duplicate option、相互排他グループ、subcommand 必須エラー
- `test/test_format_and_api.cpp`
  - 公開 API の前提条件/定義エラーに加え、`ap_format_error()` の ``error: ...`` + ``usage: ...`` を固定する golden-output 検証

テスト追加時は、可能な限り `err.code`、`err.argument`、`err.message` の3項目をまとめて検証してください。

## 8.5 実例への逆リンク
- 基本的なパースとエラー整形: [`README.md`](../README.md), [`sample/example1.c`](../sample/example1.c)
- ネストした subcommand と `subcommand_path`: [`README.md`](../README.md), [`sample/example_subcommands.c`](../sample/example_subcommands.c)
- formatter API（`ap_format_help`、`ap_format_manpage`、`ap_format_bash_completion`、`ap_format_fish_completion`、`ap_format_zsh_completion`）: [`README.md`](../README.md), [`sample/example_completion.c`](../sample/example_completion.c), [`sample/example_manpage.c`](../sample/example_manpage.c)
- introspection API（`ap_parser_get_info`、`ap_parser_get_argument`、`ap_parser_get_subcommand`）: [`README.md`](../README.md), [`sample/example_introspection.c`](../sample/example_introspection.c)

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
