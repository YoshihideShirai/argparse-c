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
  [`ParseIntermixedAliasParsesMixedOrder`](../../../test/test_parse_core.cpp)
- Known-args collection rules remain identical:
  [`ParseKnownArgsCollectsUnknownOptionValueToken`](../../../test/test_known_args.cpp),
  [`ParseKnownArgsCollectsAfterDoubleDash`](../../../test/test_known_args.cpp)

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

- [`ParseKnownArgsCollectsUnknownOptionValueToken`](../../../test/test_known_args.cpp)
- [`ParseKnownArgsCollectsExtraPositionals`](../../../test/test_known_args.cpp)
- [`ParseKnownArgsCollectsAfterDoubleDash`](../../../test/test_known_args.cpp)

## Notes with subcommands and `nargs`

- Subcommand handling is unchanged from base parse APIs.  
  Known-args variants can merge unknowns from nested subcommands while preserving subcommand path metadata.
  See [`ParseKnownArgsMergesNestedSubcommandUnknownsAndPreservesPath`](../../../test/test_known_args.cpp).
- `nargs` interactions are also unchanged.  
  For example, optional nargs should not consume a following known option token, and greedy positional forms still follow the normal binding rules.
  See [`OptionalNargsDoesNotConsumeFollowingKnownOption`](../../../test/test_known_args.cpp) and
  [`PositionalZeroOrMoreLeavesTokensForRequiredPositional`](../../../test/test_known_args.cpp).
