# Getting Started

このページでは、`argparse-c` を **インストールして、最小サンプルを実行するまで** をまとめます。

## 前提

- C99 コンパイラ
- CMake
- Git

## ビルドとインストール

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
cmake --install build --prefix /usr/local
```

一度ステージング用ディレクトリへ入れたい場合は、`/usr/local` を任意の prefix に置き換えてください。

## サンプルプログラム

リポジトリには `sample/example1.c` が含まれています。

このサンプルでは以下を確認できます。

- required な option (`-t, --text`)
- `int32` option (`-i, --integer`)
- positional 引数 (`arg1`, `arg2`)
- エラー時の `ap_format_error(...)`
- 成功時の `ap_ns_get_*` による値取得

## 実行イメージ

```bash
./build/sample/example1 -t hello -i 42 input.txt extra.txt
```

想定出力:

```text
text=hello
integer=42
arg1=input.txt
arg2=extra.txt
```

## 最初に覚える API

### 1. パーサを作る

```c
ap_parser *parser = ap_parser_new("demo", "demo parser");
```

### 2. 引数を定義する

```c
ap_arg_options opts = ap_arg_options_default();
opts.required = true;
opts.help = "input text";
ap_add_argument(parser, "-t, --text", opts, &err);
```

### 3. パースする

```c
ap_parse_args(parser, argc, argv, &ns, &err);
```

### 4. 値を取り出す

```c
const char *text = NULL;
ap_ns_get_string(ns, "text", &text);
```

## Generator API のサンプル

`sample/example1.c` に加えて、generator API をすぐ試せるサンプルも含まれています。

- `sample/example_completion.c`: `--generate-bash-completion` / `--generate-fish-completion` / `--generate-manpage` と completion callback 用の隠し `__complete` エントリポイントをアプリ側で実装する最小例
- `sample/example_manpage.c`: subcommand を含む parser 定義から man page と shell completion を生成する例

### アプリ側で generator フラグを実装する例

```c
ap_arg_options bash = ap_arg_options_default();
bash.type = AP_TYPE_BOOL;
bash.action = AP_ACTION_STORE_TRUE;
bash.help = "print a bash completion script";
ap_add_argument(parser, "--generate-bash-completion", bash, &err);

ap_arg_options fish = ap_arg_options_default();
fish.type = AP_TYPE_BOOL;
fish.action = AP_ACTION_STORE_TRUE;
fish.help = "print a fish completion script";
ap_add_argument(parser, "--generate-fish-completion", fish, &err);

ap_arg_options manpage = ap_arg_options_default();
manpage.type = AP_TYPE_BOOL;
manpage.action = AP_ACTION_STORE_TRUE;
manpage.help = "print a roff man page";
ap_add_argument(parser, "--generate-manpage", manpage, &err);

if (bash_completion) {
  char *script = ap_format_bash_completion(parser);
  printf("%s", script);
  free(script);
  return 0;
}
```

### bash completion を生成する

```bash
./build/sample/example_completion --generate-bash-completion > example_completion.bash
source ./example_completion.bash
```

### fish completion を生成する

```bash
./build/sample/example_completion --generate-fish-completion   > ~/.config/fish/completions/example_completion.fish
```

実行時の状態に応じた completion callback を組み込みたい場合は、次に [Completion callback ガイド](guides/completion-callbacks.md) を参照してください。`ap_arg_options.completion_callback`、`ap_arg_options.completion_kind`、`ap_complete(...)`、隠し `__complete` エントリポイントの接続方法をまとめています。

### man page を生成する

```bash
./build/sample/example_manpage --generate-manpage > example_manpage.1
man ./example_manpage.1
```

## 次に読むサンプル / ガイド

### サンプル

- [`sample/example1.c`](../../sample/example1.c): required option、positional、`ap_format_error(...)`、namespace 取得の最初の一歩
- [`sample/example_subcommands.c`](../../sample/example_subcommands.c): ネストした subcommand と `subcommand_path` の確認
- [`sample/example_completion.c`](../../sample/example_completion.c): formatter API と実行時 completion callback の実装例
- [`sample/example_manpage.c`](../../sample/example_manpage.c): subcommand を含む parser から manpage / completion を生成する例
- [`sample/example_introspection.c`](../../sample/example_introspection.c): `ap_parser_get_info`、`ap_parser_get_argument`、`ap_parser_get_subcommand` を使った introspection 例

### ガイド

- [基本の使い方](guides/basic-usage.md)
- [オプションと型](guides/options-and-types.md)
- [nargs](guides/nargs.md)
- [Subcommands](guides/subcommands.md)
- [Completion callback](guides/completion-callbacks.md)
- [API仕様](../api-spec.ja.md)
- [English Getting Started](../en/getting-started.md)
