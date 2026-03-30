# Getting Started

## API 参考（计划翻译）

完整 API 文档当前提供英文与日文版本（本地化版本计划中）：
- [API Reference (English)](../api-spec.en.md)
- [API仕様（日本語）](../api-spec.ja.md)

[首页](index.md) | [基础指南](guides/basic-usage.md) | [FAQ](faq.md)

本页是从仓库 README 进入后的主要上手路径，涵盖 **安装 `argparse-c`、运行第一个示例、以及选择下一步指南**。

## 前置条件

- C99 compiler
- CMake
- Git

## 构建并安装

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
cmake --install build --prefix /usr/local
```

## 示例程序

The repository includes `sample/example1.c`.

## 运行示例

```bash
./build/sample/example1 -t hello -i 42 input.txt extra.txt
```

Expected output:

```text
text=hello
integer=42
arg1=input.txt
arg2=extra.txt
```

## 首先要掌握的 API

### 1. Create a parser

```c
ap_parser *parser = ap_parser_new("demo", "demo parser");
```

### 2. Define arguments

```c
ap_arg_options opts = ap_arg_options_default();
opts.required = true;
opts.help = "input text";
ap_add_argument(parser, "-t, --text", opts, &err);
```

### 3. Parse the command line

```c
ap_parse_args(parser, argc, argv, &ns, &err);
```

### 4. Read values from the namespace

```c
const char *text = NULL;
ap_ns_get_string(ns, "text", &text);
```

## 下一步阅读

- [Basic Guide](guides/basic-usage.md)
- [FAQ](faq.md)
- [Home](index.md)
