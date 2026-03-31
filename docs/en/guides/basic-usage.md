# Basic usage

If you are new to `argparse-c`, the easiest mental model is this four-step flow:

1. Create a parser with `ap_parser_new(...)`
2. Define arguments with `ap_add_argument(...)`
3. Parse the command line with `ap_parse_args(...)`
4. Read results with `ap_ns_get_*`

## Optional arguments

```c
ap_arg_options text = ap_arg_options_default();
text.required = true;
text.help = "text value";
ap_add_argument(parser, "-t, --text", text, &err);
```

- accepts both `-t` and `--text`
- becomes mandatory when `required = true`

## Positional arguments

```c
ap_arg_options input = ap_arg_options_default();
input.help = "input file";
ap_add_argument(parser, "input", input, &err);
```

- positionals are declared without flags
- they appear as positional entries in usage/help

## Argument groups (custom help sections)

Use argument groups when you want related arguments to appear under a titled
section in help output.

```c
ap_argument_group *output = ap_add_argument_group(
    parser, "output", "output formatting controls", &err);

ap_arg_options color = ap_arg_options_default();
color.help = "color mode";
ap_argument_group_add_argument(output, "--color", color, &err);

ap_arg_options target = ap_arg_options_default();
target.help = "target file";
ap_argument_group_add_argument(output, "target", target, &err);
```

Notes:

- `title` is required for `ap_add_argument_group(...)`
- grouped arguments are parsed the same way as regular arguments
- grouped arguments are shown in their custom section in `ap_format_help(...)`

## Error handling

`argparse-c` does not call `exit()` inside the library. On failure, inspect `ap_error` and optionally render a user-facing message with `ap_format_error(...)`.

```c
if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *message = ap_format_error(parser, &err);
  fprintf(stderr, "%s", message ? message : err.message);
  free(message);
}
```

## Help output

`-h/--help` is added automatically.

```c
char *help = ap_format_help(parser);
printf("%s", help);
free(help);
```

## Sample: `example_test_runner`

`sample/example_test_runner.c` is a minimal sample that verifies argument
parsing behavior in a unit-test-like style (success and failure cases in one
small program).

Build and run:

```bash
cmake --build build --target example_test_runner
./build/sample/example_test_runner
```

Expected output includes PASS lines for each case, for example:

- `PASS: success case parsed --count=3 name=alice`
- `PASS: invalid-int case failed as expected`
- `PASS: missing-positional case failed as expected`
- `PASS: all tests passed`
