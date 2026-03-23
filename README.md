# argparse-c

`argparse-c` is a C99 CLI parsing library with a Python `argparse`-style authoring experience.

> Build C99 CLIs with Python `argparse`-like ergonomics — including completion, manpage generation, subcommands, and known-args parsing.

[![README (日本語)](https://img.shields.io/badge/README-日本語-0A66C2?style=for-the-badge)](./README.ja.md)
[![Docs (English)](https://img.shields.io/badge/GitHub%20Pages-English-0A66C2?style=for-the-badge)](https://yoshihideshirai.github.io/argparse-c/en/)
[![Docs (日本語)](https://img.shields.io/badge/GitHub%20Pages-日本語-0A66C2?style=for-the-badge)](https://yoshihideshirai.github.io/argparse-c/ja/)

## Start here

- **README / 日本語**: [README.ja.md](./README.ja.md)
- **GitHub Pages / English**: <https://yoshihideshirai.github.io/argparse-c/en/>
- **GitHub Pages / 日本語**: <https://yoshihideshirai.github.io/argparse-c/ja/>
- **Getting Started / English**: <https://yoshihideshirai.github.io/argparse-c/en/getting-started/>
- **Getting Started / 日本語**: <https://yoshihideshirai.github.io/argparse-c/ja/getting-started/>

If you want the installation steps, completion setup, package metadata, or API details, jump to GitHub Pages first. The repository README now stays focused on **what this library is**, **why it is useful**, and **which examples to open next**.

## What is `argparse-c`?

The English and Japanese READMEs are kept at the same level of detail for the overview, benefits, and feature list. Detailed setup and feature usage live in the docs site.

`argparse-c` helps you define command-line interfaces in C99 without hand-writing low-level argument scanning and validation logic. You declare a parser, add options and positionals, parse `argv`, and read values from a namespace.

It is designed for applications that want modern CLI behavior while keeping control in the application:

- Python `argparse`-inspired parser construction
- Optional arguments, positional arguments, defaults, required flags, and `choices`
- `nargs`, append/count/store-const actions, and mutually exclusive groups
- Nested subcommands
- Shell completion and manpage generation from the same parser definition
- `ap_parse_known_args(...)` for forwarding unknown arguments
- Non-exit error handling so your app decides what to print and when to return

## Why it is convenient

### 1. Write CLIs in a familiar style

If you already know Python `argparse`, the overall flow will feel familiar: create a parser, add arguments, parse, then read typed values.

### 2. Cover real CLI needs without bolting on extra tooling

A single parser definition can power:

- user-facing help
- shell completion entrypoints
- manpage generation
- nested command trees
- known/unknown argument splitting for wrapper commands

### 3. Keep control in the application

`argparse-c` does not force `exit()` on parse failure. Your program receives structured errors and can decide whether to format them, recover, or continue.

## What kind of CLI can you build quickly?

Examples that are easy to assemble with `argparse-c`:

- a single-command tool with required flags and positional files
- a Git-style multi-command CLI with nested subcommands
- a wrapper command that parses known flags and forwards the rest
- a CLI that generates bash/fish completion scripts and a manpage from one definition

See these runnable samples in this repository:

- [`sample/example1.c`](./sample/example1.c): minimal options + positionals + validation flow
- [`sample/example_completion.c`](./sample/example_completion.c): completion / manpage generation entrypoints
- [`sample/example_subcommands.c`](./sample/example_subcommands.c): nested subcommands and `subcommand_path`

For the walkthrough behind those examples, use GitHub Pages:

- English guides: <https://yoshihideshirai.github.io/argparse-c/en/>
- 日本語ガイド: <https://yoshihideshirai.github.io/argparse-c/ja/>

## Minimal example

```c
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new("demo", "demo parser");

  ap_arg_options text = ap_arg_options_default();
  text.required = true;
  text.help = "input text";
  ap_add_argument(parser, "-t, --text", text, &err);

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *message = ap_format_error(parser, &err);
    fprintf(stderr, "%s", message ? message : err.message);
    free(message);
    ap_parser_free(parser);
    return 1;
  }

  {
    const char *value = NULL;
    if (ap_ns_get_string(ns, "text", &value)) {
      printf("text=%s\n", value);
    }
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
```

Want the full setup, build commands, and next APIs to learn? Go to:

- **English Getting Started**: <https://yoshihideshirai.github.io/argparse-c/en/getting-started/>
- **日本語 Getting Started**: <https://yoshihideshirai.github.io/argparse-c/ja/getting-started/>

## Installation and packaging

`argparse-c` provides both CMake package metadata and a pkg-config file after installation.

- Build/install details: `docs/en/getting-started.md`, `docs/ja/getting-started.md`
- GitHub Pages / English: <https://yoshihideshirai.github.io/argparse-c/en/getting-started/>
- GitHub Pages / 日本語: <https://yoshihideshirai.github.io/argparse-c/ja/getting-started/>

GitHub Releases also publish an install-tree tarball for library consumers. The README keeps this short on purpose; use Getting Started for the exact commands and layout details.

## Completion, manpages, and API reference

The README no longer carries the detailed setup steps. Use the docs site for complete guidance:

- Completion guide / English: <https://yoshihideshirai.github.io/argparse-c/en/guides/completion-callbacks/>
- Completion guide / 日本語: <https://yoshihideshirai.github.io/argparse-c/ja/guides/completion-callbacks/>
- API reference / English: <https://yoshihideshirai.github.io/argparse-c/api-spec.en/>
- API reference / 日本語: <https://yoshihideshirai.github.io/argparse-c/api-spec.ja/>
- Docs language selector: <https://yoshihideshirai.github.io/argparse-c/>

## Documentation map

### English

- [Docs home](./docs/en/index.md)
- [Getting Started](./docs/en/getting-started.md)
- [Guides](./docs/en/guides/)
- [API spec](./docs/api-spec.en.md)

### 日本語

- [ドキュメント入口](./docs/ja/index.md)
- [Getting Started](./docs/ja/getting-started.md)
- [ガイド](./docs/ja/guides/)
- [API仕様](./docs/api-spec.ja.md)

## Development

Before finishing changes in this repository, run the formatter:

```bash
cmake -S . -B build
cmake --build build --target format
```

After formatting, rerun any relevant build/tests for your change.
