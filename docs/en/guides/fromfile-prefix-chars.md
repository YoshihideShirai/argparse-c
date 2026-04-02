# fromfile_prefix_chars

Use `ap_parser_options.fromfile_prefix_chars` when you want a command-line token to load additional arguments from a file.

This behavior is configured on parser creation via `ap_parser_new_with_options(...)`.

## Enable it from `ap_parser_options_default()`

```c
ap_error err = {0};
ap_parser_options options = ap_parser_options_default();
options.fromfile_prefix_chars = "@";

ap_parser *parser =
    ap_parser_new_with_options("example_fromfile", "fromfile demo", options);
```

- `NULL` (default) disables from-file expansion.
- Each character in `fromfile_prefix_chars` is treated as a valid file-prefix trigger.
  - Example: `"@+"` means both `@path` and `+path` load tokens from files.

## Token interpretation rules inside args files

Given `options.fromfile_prefix_chars = "@"`, a token like `@args.txt` on the command line opens `args.txt` and appends tokens from that file into argv.

The file parser rules are:

- split by whitespace
- no quote/escape processing (tokens are literal slices)
- `#` starts a comment after leading whitespace and ignores the rest of that line

Example file:

```text
--mode prod
--count 3
# this line is ignored
input.txt
```

Equivalent command-line tokens after expansion:

```text
--mode prod --count 3 input.txt
```

## Nesting, `--`, and unknown-args interaction

- **Nesting:** from-file expansion is not recursive within a single expansion pass. A token loaded from file is treated as a normal token at that parse stage.
- **`--` separator:** handled after expansion, exactly like normal argv. Once `--` is reached, subsequent tokens are treated as positional-only (or unknown in known-args mode).
- **Unknown arguments:** when using `ap_parse_known_args(...)`, unknown options/tokens (including expanded tokens) are collected into the unknown array with the same rules as normal argv parsing.

## `ap_error` examples on failure

When an args file cannot be opened (for example, missing file or permission denied), parsing fails and sets:

- `err.code = AP_ERR_INVALID_DEFINITION`
- `err.argument = "<path>"`
- `err.message = "failed to open args file '<path>'"`

Example:

```text
$ ./example_fromfile @missing.txt
error: [AP_ERR_INVALID_DEFINITION] missing.txt: failed to open args file 'missing.txt'
```

See also: `sample/example_fromfile.c`.
