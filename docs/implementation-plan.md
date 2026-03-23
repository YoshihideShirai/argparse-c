# argparse-c Implementation Plan (Progressive MVP)

## 1. Goal
Python `argparse` の主要機能を C ライブラリとして提供し、Linux/Unix CLI で実用できる品質を目指す。

- 対象: C99, CMake
- 方針: Python 風 API への刷新（旧 `argparse_*` API は置換）
- エラー処理: ライブラリは `exit` せず、エラーコードとメッセージを返却

## 2. Public API Plan

### Core API
- `ap_parser_new(const char *prog, const char *description)`
- `ap_add_subcommand(ap_parser*, const char *name, const char *description, ap_error*)`
- `ap_add_mutually_exclusive_group(ap_parser*, bool required, ap_error*)`
- `ap_group_add_argument(ap_mutually_exclusive_group*, const char *name_or_flags, ap_arg_options, ap_error*)`
- `ap_add_argument(ap_parser*, const char *name_or_flags, ap_arg_options, ap_error*)`
- `ap_parse_args(ap_parser*, int argc, char **argv, ap_namespace **out_ns, ap_error*)`
- `ap_parse_known_args(ap_parser*, int argc, char **argv, ap_namespace **out_ns, char ***out_unknown_args, int *out_unknown_count, ap_error*)`
- `ap_parser_free(ap_parser*)`
- `ap_namespace_free(ap_namespace*)`
- `ap_free_tokens(char **tokens, int count)`
- `ap_complete(const ap_parser*, int argc, char **argv, const char *shell, ap_completion_result*, ap_error*)`

### Completion / Introspection API
- `ap_completion_result_init`, `ap_completion_result_free`, `ap_completion_result_add`
- `ap_parser_get_info`, `ap_parser_get_argument`, `ap_parser_get_subcommand`
- `ap_arg_short_flag_count`, `ap_arg_short_flag_at`
- `ap_arg_long_flag_count`, `ap_arg_long_flag_at`

### Formatting API
- `ap_format_usage(const ap_parser*)`
- `ap_format_help(const ap_parser*)`
- `ap_format_manpage(const ap_parser*)`
- `ap_format_bash_completion(const ap_parser*)`
- `ap_format_fish_completion(const ap_parser*)`
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
- options: `required`, `default_value`, `const_value`, `choices`, `metavar`, `help`, `dest`, `nargs(?/*/+ / fixed)`
- completion metadata: `completion_kind`, `completion_hint`
- dynamic completion callback: `completion_callback`, `completion_user_data`
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
- manpage / bash completion / fish completion の生成

### Namespace / Introspection
- nested subcommand の leaf 名を `"subcommand"` に格納
- nested subcommand のフルパスを `"subcommand_path"` に格納
- parser / argument / subcommand introspection API を提供

## 4. Internal Architecture Plan
- `lib/api.c`: 公開API、メモリ管理、統合フロー
- `lib/core_parser.c`: トークン走査、オプション/位置引数割当
- `lib/core_validate.c`: required/nargs/choices 検証
- `lib/core_convert.c`: 型変換、namespace 構築
- `lib/format_help.c`: usage/help/error の文字列整形補助
- `lib/format_manpage.c`: manpage 生成
- `lib/format_completion_bash.c`: bash completion 生成
- `lib/format_completion_fish.c`: fish completion 生成
- `lib/ap_internal.h`: 内部構造体・内部関数宣言

## 5. Testing Plan and Current Status

### Framework
- custom self-contained test executable
- `ctest` 経由で実行
- `gcovr` によるテキスト/HTML カバレッジレポートを生成

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
- nested subcommand の `subcommand_path`
- parser / argument / subcommand introspection
- dynamic completion callback と `ap_complete`
- bash / fish completion 生成
- manpage 生成

## 6. CI/CD Plan (GitHub Actions)

### CI (`.github/workflows/ci.yml`)
- trigger: push (`main`/`master`), pull_request
- matrix: `gcc`, `clang`
- steps: dependency install -> cmake configure -> build -> ctest -> gcovr

### CD (`.github/workflows/cd.yml`)
- trigger: tag push (`v*`) / workflow_dispatch
- build: clang
- release assets:
  - source tarball
  - linux build tarball (`libargparse-c.so`, `example1`)
  - `SHA256SUMS.txt`
- publish: GitHub Release

### Docs / Coverage Pages (`.github/workflows/pages.yml`)
- trigger: push (`main`/`master`), workflow_dispatch
- build: MkDocs site + coverage build/test + `gcovr --html-details`
- publish: GitHub Pages (`/coverage/` で HTML レポートを公開)

## 7. Next Phase Plan (Recommended)
- completion callback の利用者向けガイド追加（`__complete` エントリポイント設計、shell 統合例、fallback 方針）
- introspection / formatter API の利用例拡充（README / Getting Started / sample の相互参照整理）
- error code/message の一貫性改善（`err.argument` の規則と文言テンプレートを固定）
- coverage レポートをもとに境界ケースの追加テストを拡充（大きな `argc`、completion path の深いネスト、generator の quoting）
