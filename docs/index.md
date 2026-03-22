# argparse-c

`argparse-c` は、Python の `argparse` に着想を得た **C99 向けコマンドライン引数パーサ**です。

- optional / positional arguments に対応
- `--option=value` と `-o=value` をサポート
- `subcommand`、`nargs`、`choices`、相互排他グループに対応
- エラー時に `exit()` せず、呼び出し側へ制御を返す設計

## このサイトで読めること

- **Getting Started**: 最初のビルドと実行
- **Guides**: 機能別の丁寧な使い方
- **API Reference**: 公開 API の詳細仕様

## 3分でわかる最小例

```c
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new("demo", "demo parser");

  ap_arg_options opts = ap_arg_options_default();
  opts.required = true;
  opts.help = "input text";
  ap_add_argument(parser, "-t, --text", opts, &err);

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *message = ap_format_error(parser, &err);
    fprintf(stderr, "%s", message ? message : err.message);
    free(message);
    ap_parser_free(parser);
    return 1;
  }

  {
    const char *text = NULL;
    if (ap_ns_get_string(ns, "text", &text)) {
      printf("text=%s\n", text);
    }
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
```

## 主な機能

### 基本引数

- positional 引数
- optional 引数
- 必須指定 (`required`)
- デフォルト値 (`default_value`)

### 値の扱い

- `string`, `int32`, `bool`
- `choices`
- `append`, `count`, `store_const`

### 複雑な CLI 向け機能

- `nargs` (`?`, `*`, `+`, fixed)
- nested subcommands
- mutually exclusive groups
- `ap_parse_known_args(...)`

## 次に読むページ

1. [Getting Started](getting-started.md)
2. [基本の使い方](guides/basic-usage.md)
3. [オプションと型](guides/options-and-types.md)
4. [API Reference（日本語）](api-spec.ja.md)
