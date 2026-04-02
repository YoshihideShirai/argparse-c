# fromfile_prefix_chars

`ap_parser_options.fromfile_prefix_chars` を使うと、コマンドライン上の特定プレフィックス付きトークンを「引数ファイル読み込み」として扱えます。

この挙動は `ap_parser_new_with_options(...)` で parser 作成時に設定します。

## `ap_parser_options_default()` から有効化する

```c
ap_error err = {0};
ap_parser_options options = ap_parser_options_default();
options.fromfile_prefix_chars = "@";

ap_parser *parser =
    ap_parser_new_with_options("example_fromfile", "fromfile demo", options);
```

- デフォルト値 `NULL` は無効（展開しない）です。
- `fromfile_prefix_chars` 内の各文字が有効プレフィックスです。
  - 例: `"@+"` を設定すると `@path` と `+path` の両方で読み込みます。

## 引数ファイル内トークンの解釈ルール

`options.fromfile_prefix_chars = "@"` の場合、CLI の `@args.txt` は `args.txt` を開いて、その中のトークンを argv に展開します。

ファイル内の解釈は次のとおりです。

- 空白区切りで分割
- クォート/エスケープは解釈しない（文字列はそのままトークン化）
- 行頭空白を飛ばした位置で `#` が出たら、その行の残りはコメントとして無視

例:

```text
--mode prod
--count 3
# この行は無視される
input.txt
```

展開後は次と同等です。

```text
--mode prod --count 3 input.txt
```

## ネスト、`--`、未知引数との相互作用

- **ネスト:** 1回の展開処理内では再帰展開しません。ファイルから取り出した `@...` も、その段階では通常トークンとして扱われます。
- **`--` セパレータ:** 展開後に通常の argv と同じ規則で処理されます。`--` 以降は positional-only（known-args では unknown 側）になります。
- **未知引数:** `ap_parse_known_args(...)` では、展開後トークンを含めて通常ルールどおり unknown 配列へ回収されます。

## 失敗時の `ap_error` 例

引数ファイルを開けない場合（ファイル不存在、権限不足など）は parse 失敗となり、次が設定されます。

- `err.code = AP_ERR_INVALID_DEFINITION`
- `err.argument = "<path>"`
- `err.message = "failed to open args file '<path>'"`

例:

```text
$ ./example_fromfile @missing.txt
error: [AP_ERR_INVALID_DEFINITION] missing.txt: failed to open args file 'missing.txt'
```

合わせて `sample/example_fromfile.c` も参照してください。
