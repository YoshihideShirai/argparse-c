# argparse-c

Build C99 command-line tools with a Python `argparse`-style authoring experience.

`argparse-c` is for developers who want more than basic flag parsing: completion, manpage generation, subcommands, `nargs`, and known-args parsing are all available from the same parser definition.

## Why start here?

Use this site when you want to:

- install the library from source or release assets
- copy a minimal CLI example and then expand it
- add completion or manpage generation without inventing custom plumbing
- look up the exact API contract after learning the basics

## Recommended reading order

1. **[Getting Started](getting-started.md)** — install the library, run the first sample, and learn the minimum parser flow
2. **Guides** — dive into features as you need them
   - [Basic usage](guides/basic-usage.md)
   - [Options and types](guides/options-and-types.md)
   - [nargs](guides/nargs.md)
   - [Subcommands](guides/subcommands.md)
   - [Completion callbacks](guides/completion-callbacks.md)
3. **Reference**
   - [API Reference (English)](../api-spec.en.md)
   - [FAQ](faq.md)

## What you can build quickly

- a single-command CLI with required flags and positional inputs
- a nested subcommand CLI
- a wrapper command that forwards unknown arguments via `ap_parse_known_args(...)`
- a CLI that emits bash/fish completion scripts and a manpage

## Example programs in this repository

- [`sample/example1.c`](../repository/sample/example1.c.md) — minimal parse/validate/read flow
- [`sample/example_completion.c`](../repository/sample/example_completion.c.md) — completion and manpage generator entrypoints
- [`sample/example_subcommands.c`](../repository/sample/example_subcommands.c.md) — nested subcommands and `subcommand_path`

## Need Japanese docs?

- [日本語トップ](../ja/index.md)
- [日本語 Getting Started](../ja/getting-started.md)
