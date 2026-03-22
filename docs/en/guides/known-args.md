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
