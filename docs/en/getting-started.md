# Getting Started

This page walks through **installing `argparse-c` and running the sample program**.

## Prerequisites

- C99 compiler
- CMake
- Git

## Build and install

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
cmake --install build --prefix /usr/local
```

If you want to install into a temporary staging directory first, replace `/usr/local` with your preferred prefix.

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

## Generator API samples

The repository now includes generator-oriented samples in addition to `sample/example1.c`.

- `sample/example_completion.c`: minimal app-side implementation of `--generate-bash-completion`, `--generate-fish-completion`, `--generate-manpage`, and the hidden `__complete` entrypoint used by completion callbacks
- `sample/example_manpage.c`: subcommand-based parser that emits a man page and shell completions from the same parser definition

### Implement generator flags in your application

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

### Generate bash completion

```bash
./build/sample/example_completion --generate-bash-completion > example_completion.bash
source ./example_completion.bash
```

### Generate fish completion

```bash
./build/sample/example_completion --generate-fish-completion   > ~/.config/fish/completions/example_completion.fish
```

For runtime-aware completion callbacks, continue with [Completion callbacks](guides/completion-callbacks.md), which shows the exact APIs to wire: `ap_arg_options.completion_callback`, `ap_arg_options.completion_kind`, `ap_complete(...)`, and the hidden `__complete` entrypoint.

### Generate a man page

```bash
./build/sample/example_manpage --generate-manpage > example_manpage.1
man ./example_manpage.1
```

## Recommended next sample programs / guides

### Sample programs

- [`sample/example1.c`](../../sample/example1.c): start here for required options, positionals, namespace reads, and `ap_format_error(...)`
- [`sample/example_subcommands.c`](../../sample/example_subcommands.c): continue here for nested subcommands and `subcommand_path`
- [`sample/example_completion.c`](../../sample/example_completion.c): formatter APIs plus runtime completion callbacks
- [`sample/example_manpage.c`](../../sample/example_manpage.c): formatter APIs on a subcommand tree that is suitable for docs output
- [`sample/example_introspection.c`](../../sample/example_introspection.c): parser metadata introspection via `ap_parser_get_info`, `ap_parser_get_argument`, and `ap_parser_get_subcommand`

### Guides

- [Basic usage](guides/basic-usage.md)
- [Options and types](guides/options-and-types.md)
- [nargs](guides/nargs.md)
- [Subcommands](guides/subcommands.md)
- [Completion callbacks](guides/completion-callbacks.md)
- [API Specification](../api-spec.en.md)
- [日本語 Getting Started](../ja/getting-started.md)
