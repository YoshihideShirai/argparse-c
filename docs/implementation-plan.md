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

#### `test/test_parse_core.cpp`
- 基本パース成功（必須 optional + positional の同時解決）
- unknown option の即時エラー
- `int32` 変換失敗
- `choices` 検証
- optional 引数の `nargs=+`
- default value の注入
- 自動 `dest` 解決（long flag 優先、`-` → `_` 正規化）
- positional の `dest` 正規化と explicit `dest` 非正規化
- help 文字列の基本生成
- inline value (`--opt=value`, `-o=value`)
- `required` と `default_value` の優先順位
- default value に対する `choices` 検証

#### `test/test_known_args.cpp`
- `ap_parse_known_args` による unknown option 回収
- unknown option に続く値トークンの回収
- 余剰 positional の回収
- `--` 以降のトークン回収
- known option を unknown option の値として誤回収しないこと
- inline unknown option を 1 トークンのまま回収すること
- `nargs=?` optional が後続 known option を飲み込まないこと
- `nargs=*` positional が後続 required positional を残すこと
- short bool cluster (`-vq`) の正常系

#### `test/test_format_and_api.cpp`
- help / usage / error / manpage の生成
- bool / const / append / positional `nargs` 各種の usage/help 表示
- `required` な `store_true` / `store_const` の検証
- `ap_parse_known_args` API の guard 条件
- namespace getter / free helper の失敗経路
- parser / argument / subcommand introspection
- `--help` が required option / subcommand をバイパスすること
- short option cluster の duplicate / invalid 組み合わせ
- nested subcommand の `subcommand_path`
- dynamic completion callback と `ap_complete`
- bash / fish completion script 生成
- manpage の nested subcommand / choices / default / required 表示

### Additional Test Candidates (Breakdown)

#### A. Large `argc` / long argument vectors
- **大量 known option 反復**: `--tag value` や `-v` を数十〜数百回並べ、`append` / `count` が順序どおり蓄積されること。
- **大量 positional**: `nargs=*` / `+` positional に長い入力列を与え、末尾 required positional の取り分けが崩れないこと。
- **大量 mixed sequence**: optional / positional / unknown を長い列で交互に並べ、`token_index` 進行と `remaining_min_required()` の境界を確認すること。
- **大量 unknown 連続**: known-args モードで unknown option と unknown value を連続投入し、脱落や誤結合がないこと。

#### B. Deep completion-path nesting
- **3〜5 段以上の subcommand 遷移**: `root/a/b/c/...` のような深い階層で parser key が正しく連結されること。
- **深い階層 + option completion**: 葉 parser の choices / file / directory / command completion が親階層に漏れず出力されること。
- **深い階層 + dynamic completion**: 葉 parser でのみ dynamic completion hook が有効になること。
- **同名 option の階層分離**: 親と子で同名 flag を持つ場合でも completion dispatch key が衝突しないこと。

#### C. Generator quoting / escaping
- **bash single-quote escape**: choice 値や `prog` 名に `'` を含むケースで `append_single_quoted()` の展開が壊れないこと。
- **fish double-quote escape**: help / choice / `prog` に `"`, `\`, `$` を含むケースで `append_fish_double_quoted()` が正しくエスケープすること。
- **改行混在 help**: fish の説明文で改行が空白へ正規化され、script 構文を壊さないこと。
- **roff escape**: manpage で `-`, `\`, 行頭の `.` / `'` を含む help や default が roff-safe に出力されること。

### String-generation edge cases by formatter

#### `lib/format_completion_bash.c`
- choice 値に空白を含むケース（`"two words"`）
- choice 値に単一引用符を含むケース（`"it's"`）
- `prog` / subcommand / option 名に `-` や `.` が含まれ、identifier 化で `_` へ置換されるケース
- help ではなく choices / parser key / dispatch key 中に特殊文字が入るケース

#### `lib/format_completion_fish.c`
- help 文に空白、`"`, `\`, `$`, 改行を含むケース
- choice 値に空白や `$HOME` のような展開文字列を含むケース
- metavar fallback が `dest` の `_` を `-` に戻すケース
- 深い parser key と説明文が同時に出るケース

#### `lib/format_manpage.c`
- help / default / choice に `-`, `\`, 行頭 `.` / `'`、改行を含むケース
- option signature の metavar に空白や引用符を含めた場合の roff 出力確認
- positional / optional が混在し、`nargs` 展開と roff escape が同時に入るケース
- nested subcommand の help/synopsis に特殊文字が含まれるケース

### Parser edge cases to add (`lib/core_parser.c`)

#### positional / optional mixed parsing
- `nargs=*` positional の前後に optional を長く挟んだ列で、required positional へ最低必要数を残せること。
- `nargs=?` positional が 0 件を選んだあと、後続 positional が正しく詰められること。
- fixed-count positional が長い列の途中にあるとき、過不足が正確に検出されること。

#### `--` separator handling
- `--` 直前までに optional の値消費が終わっていること。
- `--` 以降に `-x`, `--long`, 通常文字列が長く続くケースでも、known-args では順序どおり unknown に入ること。
- parse-args モードでは `--` 以降の余剰 positional が `unexpected positional` になる境界を確認すること。

#### unknown-argument collection
- unknown option が連続するケース（`--u1 --u2 --u3`）で、それぞれ独立した unknown として回収されること。
- unknown option + 値 + unknown option + 値の交互列で、値のひも付けが 1 トークン先読みの想定どおりであること。
- unknown short cluster 風入力や `-o=value` 風 unknown が known option に誤認されないこと。
- 長い既知/未知混在列で known option の値が unknown 側へ吸われないこと。

### Proposed placement and expected assertions

#### `test/test_parse_core.cpp`
- **大きな `argc` の parse_args 正常系**: 長い mixed sequence を投入し、`append` / `count` / positional の件数と順序を明示確認する。
- **parse_args + `--` 境界**: `--` 後に余剰 positional を置き、`AP_ERR_UNEXPECTED_POSITIONAL` と該当 token を期待する。
- **長い positional/optional 混在**: `nargs=*`, `nargs=?`, fixed positional の配分結果を件数・各 index 値で確認する。

#### `test/test_format_and_api.cpp`
- **bash completion quoting**: choice 値や `prog` 名に空白・`'` を含め、生成 script 内の quoting 断片を文字列一致で確認する。
- **fish completion quoting**: help / choice に `"`, `\`, `$`, 改行を含め、エスケープ結果と改行→空白変換を確認する。
- **manpage escaping**: help / default / choice / subcommand help に roff 特殊文字を含め、`\-`, `\&`, `\\` などの期待断片を確認する。
- **deep nested completion**: 3〜5 段 subcommand を作り、parser key・dispatch case・leaf option completion の存在を確認する。

#### `test/test_known_args.cpp`
- **大量 unknown 連続**: unknown 配列の件数と全要素順序を明示確認する。
- **unknown option/value 交互列**: 値付き unknown のみ 1 トークン追加回収され、後続 known option は正しく parse されることを確認する。
- **`--` 後の長い unknown 列**: 全件が未知配列へ順序保持で回収されることを確認する。
- **長い mixed known/unknown**: known namespace 値と unknown 配列の両方を同時に検証し、誤消費がないことを確認する。

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
