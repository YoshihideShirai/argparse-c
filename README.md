# argparse-c

`argparse-c` is a C99 command-line argument parsing library inspired by Python `argparse`.

## Features (MVP)

- Optional and positional arguments
- `--option=value` and `-o=value` styles for options
- Short bool flag clusters (example: `-vq` for `-v -q`)
- Types: `string`, `int32`, `bool` (`store_true` / `store_false`)
- `required`, `default_value`, `choices`
- `nargs`: one, `?`, `*`, `+`, fixed arity
- `append`, `count`, `store_const`
- Mutually exclusive groups
- Subcommands
- Built-in `-h/--help`
- Non-exit error flow (`ap_parse_args` returns error codes)
- Error text helper: `ap_format_error(parser, &err)`
- Known/unknown split parser: `ap_parse_known_args(...)`
  - For `ap_parse_known_args`, tokens after `--` are collected into unknown args.

## Quick Example

```c
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("demo", "demo parser");

  ap_arg_options o = ap_arg_options_default();
  o.required = true;
  o.help = "input text";
  ap_add_argument(p, "-t, --text", o, &err);

  if (ap_parse_args(p, argc, argv, &ns, &err) != 0) {
    fprintf(stderr, "error: %s\n", err.message);
    ap_parser_free(p);
    return 1;
  }

ap_namespace_free(ns);
ap_parser_free(p);
return 0;
}
```

## Subcommands

```c
ap_error err = {0};
ap_namespace *ns = NULL;
ap_parser *p = ap_parser_new("demo", "demo parser");
ap_parser *run = ap_add_subcommand(p, "run", "run the job", &err);

ap_arg_options config = ap_arg_options_default();
config.help = "config path";
ap_add_argument(run, "--config", config, &err);

if (ap_parse_args(p, argc, argv, &ns, &err) == 0) {
  const char *subcommand = NULL;
  const char *config_path = NULL;
  ap_ns_get_string(ns, "subcommand", &subcommand);
  ap_ns_get_string(ns, "config", &config_path);
}
```

## Build (clang)

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
ctest --test-dir build --output-on-failure
```

## Example Program

See [sample/example1.c](./sample/example1.c).

## API Specification

- Japanese: [docs/api-spec.ja.md](./docs/api-spec.ja.md)
- English: [docs/api-spec.en.md](./docs/api-spec.en.md)

## License

MIT License.
