# 基本の使い方

`argparse-c` を初めて使う場合、まずは以下の 4 ステップを押さえると理解しやすいです。

1. `ap_parser_new(...)` でパーサを作る
2. `ap_add_argument(...)` で引数を定義する
3. `ap_parse_args(...)` でコマンドラインを読む
4. `ap_ns_get_*` で結果を取り出す

## optional 引数

```c
ap_arg_options text = ap_arg_options_default();
text.required = true;
text.help = "text value";
ap_add_argument(parser, "-t, --text", text, &err);
```

- `-t` と `--text` の両方を受け付けられます
- `required = true` を付けると必須になります

## positional 引数

```c
ap_arg_options input = ap_arg_options_default();
input.help = "input file";
ap_add_argument(parser, "input", input, &err);
```

- positional はフラグ無しで宣言します
- usage / help にも positional として表示されます

## エラー処理

`argparse-c` はライブラリ内部で `exit()` しません。失敗時は `ap_error` を確認し、必要なら `ap_format_error(...)` で表示用文字列を生成します。

```c
if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *message = ap_format_error(parser, &err);
  fprintf(stderr, "%s", message ? message : err.message);
  free(message);
}
```

## ヘルプ表示

`-h/--help` は自動で追加されます。

```c
char *help = ap_format_help(parser);
printf("%s", help);
free(help);
```
