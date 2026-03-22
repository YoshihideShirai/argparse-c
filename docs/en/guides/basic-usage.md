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
