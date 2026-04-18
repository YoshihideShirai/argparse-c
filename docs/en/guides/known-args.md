# known args / unknown args

Use `ap_parse_known_args(...)` when you want to parse only known arguments and collect the remaining tokens separately.

## Typical use cases

- building wrapper CLIs
- forwarding unparsed arguments to another command
- preserving everything after `--`

## API

```c
ap_parse_known_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err);
```

## What is collected as unknown

- unknown options
- a value-like token immediately following an unknown option
- extra positional tokens left over after positional binding
- every token after `--`

## `--` delimiter: concrete forwarding examples

When building a wrapper CLI, decide whether downstream flags should be treated as
**unknown candidates** or as a **hard passthrough tail**.

Without `--`:

```text
wrapper --config conf.yml --plugin-opt x --verbose input.txt
```

- `--plugin-opt` and `x` are collected as unknown.
- `--verbose` is still parsed by the wrapper if it is a known option there.
- This mode is useful when you want to keep parsing known wrapper options even
  after unknown tokens appear.

With `--`:

```text
wrapper --config conf.yml -- --plugin-opt x --verbose input.txt
```

- every token after `--` is collected as unknown, in order.
- wrapper parsing stops at the delimiter boundary.
- this is the safest default when forwarding to another executable.

See test coverage:

- [`ParseKnownArgsCollectsAfterDoubleDash`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsDoubleDashProtectsKnownFlagsFromAccidentalInjection`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsWithoutDoubleDashStillParsesKnownFlags`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## Unknown token order guarantee policy

`ap_parse_known_args(...)` preserves the unknown token sequence in encounter
order. This applies to:

- consecutive unknown options
- alternating unknown option/value pairs
- tails captured after `--`
- merged unknowns across nested subcommands

Treat this as a stable forwarding contract: downstream tools receive unknown
tokens in the same relative order as input.

## Memory cleanup

Release the unknown token array with `ap_free_tokens(...)`.

```c
ap_free_tokens(unknown, unknown_count);
```

## Intermixed variants (`ap_parse_intermixed_args` / `ap_parse_known_intermixed_args`)

Use these when you want to express "options and positionals may appear in mixed order" in API naming.

## Difference from `ap_parse_args` / `ap_parse_known_args`

- `ap_parse_intermixed_args(...)` currently behaves as an alias of `ap_parse_args(...)`.
- `ap_parse_known_intermixed_args(...)` currently behaves as an alias of `ap_parse_known_args(...)`.
- In other words, parsing/validation behavior and error model are the same; the intermixed names are semantic aliases.

```c
ap_parse_intermixed_args(parser, argc, argv, &ns, &err);
ap_parse_known_intermixed_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err);
```

Reference tests:

- Mixed-order parse (`a.txt --count 2 b.txt`) succeeds with the intermixed API:
  [`ParseIntermixedAliasParsesMixedOrder`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_parse_core.cpp)
- Known-args collection rules remain identical:
  [`ParseKnownArgsCollectsUnknownOptionValueToken`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp),
  [`ParseKnownArgsCollectsAfterDoubleDash`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## Typical use case (alternating options and positionals)

Typical CLI shape:

```text
prog input1 --count 2 input2 --mode fast input3
```

This style is common for wrapper commands and file-processing CLIs where users naturally interleave flags and positional inputs.  
Choosing the intermixed API name can make that intent explicit in your codebase, even though behavior matches the base parse APIs today.

## Behavior when collecting unknown args

With `ap_parse_known_intermixed_args(...)`, unknown token collection follows the same rules as `ap_parse_known_args(...)`:

- unknown options are collected
- a value-like token immediately after an unknown option is collected
- extra positional tokens left after positional binding are collected
- every token after `--` is collected

See:

- [`ParseKnownArgsCollectsUnknownOptionValueToken`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsCollectsExtraPositionals`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)
- [`ParseKnownArgsCollectsAfterDoubleDash`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp)

## Notes with subcommands and `nargs`

- Subcommand handling is unchanged from base parse APIs.  
  Known-args variants can merge unknowns from nested subcommands while preserving subcommand path metadata.
  See [`ParseKnownArgsMergesNestedSubcommandUnknownsAndPreservesPath`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp).
- `nargs` interactions are also unchanged.  
  For example, optional nargs should not consume a following known option token, and greedy positional forms still follow the normal binding rules.
  See [`OptionalNargsDoesNotConsumeFollowingKnownOption`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp) and
  [`PositionalZeroOrMoreLeavesTokensForRequiredPositional`](https://github.com/YoshihideShirai/argparse-c/blob/main/test/test_known_args.cpp).

## Recommended flow when combining known-args + subcommands

1. Parse your wrapper/global options with `ap_parse_known_args(...)` at the root parser.
2. Inspect `subcommand` / `subcommand_path` from namespace for routing.
3. Forward collected unknown tokens to the selected subcommand executor.
4. Prefer inserting `--` at your wrapper boundary when forwarding opaque plugin flags.

This keeps wrapper-owned options explicit while preserving downstream autonomy
for plugin/tool-specific flags.

## Security note: avoid unintended flag injection

If wrapper input is composed from external or partially trusted sources, use
`--` before downstream arguments to prevent accidental reinterpretation by the
wrapper parser.

- Risk without delimiter: downstream-like tokens (for example `--verbose`) may
  still be consumed as wrapper-known flags.
- Mitigation with delimiter: everything after `--` is forwarded untouched as
  unknown tokens.

For production wrappers, documenting and enforcing the delimiter boundary is a
simple, high-impact hardening pattern.
