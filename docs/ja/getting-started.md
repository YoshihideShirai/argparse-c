# Getting Started

このページでは、`argparse-c` を **ビルドして、最小サンプルを実行するまで** をまとめます。

## 前提

- C99 コンパイラ
- CMake
- Git

## ビルド

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
ctest --test-dir build --output-on-failure

# coverage (gcovr が必要。clang 使用時は `llvm-cov gcov` を自動利用します)
cmake -S . -B build-coverage \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DAP_ENABLE_COVERAGE=ON
cmake --build build-coverage
cmake --build build-coverage --target coverage
```

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

## 次に読むとよいページ

- [基本の使い方](guides/basic-usage.md)
- [オプションと型](guides/options-and-types.md)
- [nargs](guides/nargs.md)
- [English Getting Started](../en/getting-started.md)
