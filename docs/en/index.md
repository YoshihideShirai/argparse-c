# argparse-c

`argparse-c` is a **C99 command-line argument parsing library** inspired by Python's `argparse`.

- Supports optional and positional arguments
- Supports `--option=value` and `-o=value`
- Supports subcommands, `nargs`, `choices`, and mutually exclusive groups
- Returns errors to the caller instead of calling `exit()` internally

## What you can read on this site

- **Getting Started**: build the project and run the sample
- **Guides**: practical usage by feature
- **API Reference**: full public API contract

## Minimal example

```c
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new("demo", "demo parser");

  ap_arg_options opts = ap_arg_options_default();
  opts.required = true;
  opts.help = "input text";
  ap_add_argument(parser, "-t, --text", opts, &err);

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *message = ap_format_error(parser, &err);
    fprintf(stderr, "%s", message ? message : err.message);
    free(message);
    ap_parser_free(parser);
    return 1;
  }

  {
    const char *text = NULL;
    if (ap_ns_get_string(ns, "text", &text)) {
      printf("text=%s\n", text);
    }
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
```

## Main features

### Core arguments

- positional arguments
- optional arguments
- required options
- default values

### Value handling

- `string`, `int32`, `bool`
- `choices`
- `append`, `count`, `store_const`

### Advanced CLI features

- `nargs` (`?`, `*`, `+`, fixed)
- nested subcommands
- mutually exclusive groups
- `ap_parse_known_args(...)`

## Next pages

1. [Getting Started](getting-started.md)
2. [Basic usage](guides/basic-usage.md)
3. [Options and types](guides/options-and-types.md)
4. [API Reference (English)](../api-spec.en.md)
5. [日本語ドキュメント](../ja/index.md)
