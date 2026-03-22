# Getting Started

This page walks through **building `argparse-c` and running the sample program**.

## Prerequisites

- C99 compiler
- CMake
- Git

## Build

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
ctest --test-dir build --output-on-failure

# coverage (requires gcovr)
cmake -S . -B build-coverage \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DAP_ENABLE_COVERAGE=ON
cmake --build build-coverage
cmake --build build-coverage --target coverage
```

## Sample program

The repository includes `sample/example1.c`.

That sample demonstrates:

- a required option (`-t, --text`)
- an `int32` option (`-i, --integer`)
- positional arguments (`arg1`, `arg2`)
- error formatting with `ap_format_error(...)`
- reading parsed values with `ap_ns_get_*`

## Example run

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

## First APIs to learn

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

## Recommended next pages

- [Basic usage](guides/basic-usage.md)
- [Options and types](guides/options-and-types.md)
- [nargs](guides/nargs.md)
- [日本語 Getting Started](../ja/getting-started.md)
