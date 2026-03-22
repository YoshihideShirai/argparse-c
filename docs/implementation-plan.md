# argparse-c Implementation Plan (Progressive MVP)

## 1. Goal
Python `argparse` の主要機能を C ライブラリとして提供し、Linux/Unix CLI で実用できる品質を目指す。

- 対象: C99, CMake
- 方針: Python 風 API への刷新（旧 `argparse_*` API は置換）
- エラー処理: ライブラリは `exit` せず、エラーコードとメッセージを返却

## 2. Public API Plan

### Core API
- `ap_parser_new(const char *prog, const char *description)`
- `ap_add_argument(ap_parser*, const char *name_or_flags, ap_arg_options, ap_error*)`
- `ap_parse_args(ap_parser*, int argc, char **argv, ap_namespace **out_ns, ap_error*)`
- `ap_parse_known_args(ap_parser*, int argc, char **argv, ap_namespace **out_ns, char ***out_unknown_args, int *out_unknown_count, ap_error*)`
- `ap_parser_free(ap_parser*)`
- `ap_namespace_free(ap_namespace*)`
- `ap_free_tokens(char **tokens, int count)`

### Formatting API
- `ap_format_usage(const ap_parser*)`
- `ap_format_help(const ap_parser*)`
- `ap_format_error(const ap_parser*, const ap_error*)`

### Namespace Accessors
- `ap_ns_get_bool`, `ap_ns_get_string`, `ap_ns_get_int32`
- `ap_ns_get_count`, `ap_ns_get_string_at`, `ap_ns_get_int32_at`

## 3. Implemented MVP Scope

### Argument Model
- optional / positional arguments
- nested subcommands
- types: `string`, `int32`, `bool`
- actions: `store`, `store_true`, `store_false`, `append`, `count`, `store_const`
- options: `required`, `default_value`, `const_value`, `choices`, `metavar`, `help`, `nargs(?/*/+ / fixed)`
- mutually exclusive groups

### Parsing Behavior
- short/long option support (`-t`, `--text`)
- inline value support (`--opt=value`, `-o=value`)
- short bool cluster support (`-vq`)
- builtin `-h/--help`
- duplicate option detection
- unknown option detection
- positional/optional mixed parsing
- `--` separator handling

### Known Args Mode
`ap_parse_known_args` では以下を unknown に回収:
- 未知オプション
- 未知オプションの値トークン（推定）
- 余剰 positional
- `--` 以降のトークン

### Validation & Conversion
- required validation
- nargs validation
- choices validation（デフォルト値にも適用）
- int32 range/format validation

### Help & Error Formatting
- usage/help 生成
- help に `choices/default/required` を表示
- `ap_format_error` で `error + usage` を一括整形

## 4. Internal Architecture Plan
- `lib/api.c`: 公開API、メモリ管理、統合フロー
- `lib/core_parser.c`: トークン走査、オプション/位置引数割当
- `lib/core_validate.c`: required/nargs/choices 検証
- `lib/core_convert.c`: 型変換、namespace 構築
- `lib/format_help.c`: usage/help/error の文字列整形補助
- `lib/ap_internal.h`: 内部構造体・内部関数宣言

## 5. Testing Plan and Current Status

### Framework
- CppUTest
- `ctest` 経由で実行

### Covered Tests
- 正常系パース
- unknown option
- invalid int32
- choices
- nargs (`+`)
- default value
- help生成
- inline values
- required + default の優先
- bool action (`store_true`/`store_false`)
- bool action の inline value 拒否
- help に choices/default/required 表示
- format_error 出力
- parse_known_args の unknown 回収（option/value/extra positional/`--` 以降）
- short bool cluster (`-vq`) と不正クラスタ

## 6. CI/CD Plan (GitHub Actions)

### CI (`.github/workflows/ci.yml`)
- trigger: push (`main`/`master`), pull_request
- matrix: `gcc`, `clang`
- steps: dependency install -> cmake configure -> build -> ctest

### CD (`.github/workflows/cd.yml`)
- trigger: tag push (`v*`) / workflow_dispatch
- build: clang
- release assets:
  - source tarball
  - linux build tarball (`libargparse-c.so`, `example1`)
  - `SHA256SUMS.txt`
- publish: GitHub Release

## 7. Next Phase Plan (Recommended)
- `dest` 自動生成規則の追加テスト・ドキュメント強化
- error code/message の一貫性改善（`err.argument` の規則と文言テンプレートを固定）
- `nargs` と unknown 回収の仕様明文化（README強化）
- nested subcommands のヘルプ表現・namespace仕様の改善
