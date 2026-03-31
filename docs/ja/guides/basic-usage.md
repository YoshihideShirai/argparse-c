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

## argument group（カスタム help セクション）

関連する引数を help 上でまとまった見出しにしたい場合は、
argument group を使います。

```c
ap_argument_group *output = ap_add_argument_group(
    parser, "output", "output formatting controls", &err);

ap_arg_options color = ap_arg_options_default();
color.help = "color mode";
ap_argument_group_add_argument(output, "--color", color, &err);

ap_arg_options target = ap_arg_options_default();
target.help = "target file";
ap_argument_group_add_argument(output, "target", target, &err);
```

ポイント:

- `ap_add_argument_group(...)` の `title` は必須です
- grouped argument でも parse の挙動は通常の引数と同じです
- `ap_format_help(...)` ではカスタムセクションとして表示されます

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

## サンプル: `example_test_runner`

`sample/example_test_runner.c` は、引数パースの成功/失敗ケースを
単体テスト風に検証する最小サンプルです。

ビルドと実行:

```bash
cmake --build build --target example_test_runner
./build/sample/example_test_runner
```

期待される出力例（PASS/FAIL）:

- `PASS: success case parsed --count=3 name=alice`
- `PASS: invalid-int case failed as expected`
- `PASS: missing-positional case failed as expected`
- `PASS: all tests passed`
