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

## `ap_parser_options` 詳細（`prefix_chars` / `allow_abbrev`）

既存 CLI の流儀に合わせたい場合は、
`ap_parser_options_default()` を基点に必要な項目だけ上書きします。

```c
ap_parser_options options = ap_parser_options_default();
options.prefix_chars = "+";
options.allow_abbrev = true;
ap_parser *parser =
    ap_parser_new_with_options("example1", "custom prefixes", options);
```

### `prefix_chars` を `-` から `+` へ変える例

`sample/example1.c` 相当の短い差分は次のとおりです。

```diff
- ap_parser *parser = ap_parser_new("example1", "example1 command.");
+ ap_parser_options options = ap_parser_options_default();
+ options.prefix_chars = "+";
+ ap_parser *parser =
+     ap_parser_new_with_options("example1", "example1 command.", options);

- ap_add_argument(parser, "-t, --text", text_opts, &err);
+ ap_add_argument(parser, "+t, ++text", text_opts, &err);

- ap_add_argument(parser, "-i, --integer", integer_opts, &err);
+ ap_add_argument(parser, "+i, ++integer", integer_opts, &err);
```

help/usage も `+`/`++` 形式に変わります。

```text
usage: example1 [+h] +t TEXT [+i INTEGER] arg1 [arg2]

optional 引数:
  +h, ++help              show this help message and exit
  +t TEXT, ++text TEXT    text field.
  +i INTEGER, ++integer INTEGER
                          integer field.
```

### `allow_abbrev=true/false` の差分（衝突時動作を含む）

`++verbose` と `++version` の 2 つがあるとします。

- `allow_abbrev = false`（既定）
  - `++ver` は部分一致しないため unknown option になります。
- `allow_abbrev = true`
  - `++ver` は複数候補に一致するため ambiguous option で失敗します。
  - `++verb` のように一意な接頭辞なら `++verbose` として解決されます。

```text
# allow_abbrev=false
$ prog ++ver
error: unknown option '++ver'

# allow_abbrev=true, 衝突ケース
$ prog ++ver
error: ambiguous option '++ver'
```

### 既存 CLI との互換性リスク

`ap_parser_options` の変更は、既存ユーザーに予期しない破壊的影響を与えます。

- **シェルスクリプト / CI**: `-x` / `--long` 前提の既存呼び出しが `prefix_chars` 変更後に失敗する。
- **ドキュメント不整合**: README や運用手順の引数例が古いままになる。
- **省略解決の不安定化**: `allow_abbrev=true` だと、新しい long option 追加時に過去は通っていた短縮形が衝突で失敗し得る。

移行時の実務的な指針:

1. 互換要件がない限り `allow_abbrev=false` を維持する。
2. `prefix_chars` を変える場合は、ラッパー移行期間やメジャーバージョン更新を検討する。
3. 正常系だけでなく「曖昧 prefix で失敗するケース」も parse テストに含める。

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
