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
- Nested subcommands
- Built-in `-h/--help`
- Non-exit error flow (`ap_parse_args` returns error codes)
- Error text helper: `ap_format_error(parser, &err)`
- Consistent error payloads: `err.argument` uses the primary flag for options and the declared name for positionals
- Known/unknown split parser: `ap_parse_known_args(...)`
  - For `ap_parse_known_args`, tokens after `--` are collected into unknown args.

## `nargs` semantics

- `AP_NARGS_ONE`
  - optionals consume exactly one value
  - positionals bind exactly one token
- `AP_NARGS_OPTIONAL`
  - optionals consume the next token only when it is not a known option
  - positionals bind one token only when enough tokens remain for later required positionals
- `AP_NARGS_ZERO_OR_MORE` / `AP_NARGS_ONE_OR_MORE`
  - optionals keep consuming until `--` or the next known option
  - positionals consume as many tokens as possible while leaving the minimum required tokens for later positional arguments
- `AP_NARGS_FIXED`
  - optionals require exactly `nargs_count` values
  - positionals bind exactly `nargs_count` tokens

## `ap_parse_known_args` contract

- known arguments are parsed with the same rules as `ap_parse_args`
- unknown tokens are appended to `out_unknown_args` in encounter order
- collected unknowns include:
  - unknown options
  - the next token after an unknown option when it does not look like another option
  - extra positional tokens left after positional binding
  - all tokens after `--`
- validation still runs for known arguments, so missing required arguments and invalid known values still fail

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

### Namespace / help contract

- nested subcommands are supported
- the namespace exposes only one built-in subcommand key: `"subcommand"`
- `"subcommand"` always stores the **final selected leaf subcommand name**
  - example: parsing `prog config set ...` stores `"set"`
- intermediate subcommand names are **not** added as separate namespace entries
- a separate `"subcommand_path"` key is **not** currently exposed
- help for a nested subcommand uses the full command path
  - example: `ap_format_help(set_parser)` starts with `usage: prog config set`

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

# coverage (requires gcovr; with clang the build uses `llvm-cov gcov` automatically)
cmake -S . -B build-coverage \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DAP_ENABLE_COVERAGE=ON
cmake --build build-coverage
cmake --build build-coverage --target coverage
```

## Documentation Site

A bilingual GitHub Pages site skeleton for MkDocs + Material for MkDocs is included in this repository.

- landing page with language selection: `docs/index.md`
- Japanese docs: `docs/ja/`
- English docs: `docs/en/`
- local preview: `mkdocs serve`
- static build: `mkdocs build`
- GitHub Pages deployment: `.github/workflows/pages.yml`
- coverage HTML report is published under `coverage/index.html` on the Pages site

## Example Program

See [sample/example1.c](./sample/example1.c).

## API Specification

- Japanese: [docs/api-spec.ja.md](./docs/api-spec.ja.md)
- English: [docs/api-spec.en.md](./docs/api-spec.en.md)

## License

MIT License.
