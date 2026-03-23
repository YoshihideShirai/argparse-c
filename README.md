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

### Parse-time subcommand contract

- nested subcommands are supported
- the namespace exposes only one built-in subcommand key: `"subcommand"`
- `"subcommand"` always stores the **final selected leaf subcommand name**
  - example: parsing `prog config set ...` stores `"set"`
- intermediate subcommand names are **not** added as separate namespace entries
- `"subcommand_path"` stores the full selected subcommand chain
  - example: parsing `prog config set ...` stores `"config set"`
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
  const char *subcommand_path = NULL;
  const char *config_path = NULL;
  ap_ns_get_string(ns, "subcommand", &subcommand);
  ap_ns_get_string(ns, "subcommand_path", &subcommand_path);
  ap_ns_get_string(ns, "config", &config_path);
}
```

For a runnable nested-subcommand example, see [sample/example_subcommands.c](./sample/example_subcommands.c).

## Introspection API quick start

Use the introspection APIs when you want to inspect parser metadata at runtime for custom docs, UI generation, or test assertions.

```c
ap_parser_info parser_info;
ap_arg_info arg_info;
ap_subcommand_info sub_info;

if (ap_parser_get_info(parser, &parser_info) == 0) {
  printf("prog=%s\n", parser_info.prog);
  printf("arguments=%d\n", parser_info.argument_count);
  printf("subcommands=%d\n", parser_info.subcommand_count);
}

if (ap_parser_get_argument(parser, 0, &arg_info) == 0) {
  printf("first dest=%s\n", arg_info.dest);
}

if (ap_parser_get_subcommand(parser, 0, &sub_info) == 0) {
  printf("first subcommand=%s\n", sub_info.name);
}
```

For a complete sample that walks arguments and subcommands, see [sample/example_introspection.c](./sample/example_introspection.c).

## Developer Build (clang)

```bash
cmake -S . -B build \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
ctest --test-dir build --output-on-failure


# formatting / static analysis (requires clang-format and clang-tidy)
cmake --build build --target format
cmake --build build --target tidy

# coverage (requires gcovr; with clang the build uses `llvm-cov gcov` automatically)
cmake -S . -B build-coverage \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DAP_ENABLE_COVERAGE=ON
cmake --build build-coverage
cmake --build build-coverage --target coverage
```

For installation steps aimed at library users, see the Getting Started guides in `docs/en/getting-started.md` and `docs/ja/getting-started.md`.

## Documentation Site

A bilingual GitHub Pages site skeleton for MkDocs + Material for MkDocs is included in this repository.

- GitHub Pages: <https://yoshihideshirai.github.io/argparse-c/>
- landing page with language selection: `docs/index.md`
- Japanese docs: `docs/ja/`
- English docs: `docs/en/`
- local preview: `mkdocs serve`
- static build: `mkdocs build`
- GitHub Pages deployment: `.github/workflows/pages.yml`
- coverage report: <https://yoshihideshirai.github.io/argparse-c/coverage/index.html>
- coverage HTML report is published under `coverage/index.html` on the Pages site

## Example Programs

Use the sample that matches the API surface you want to learn first:

- [sample/example1.c](./sample/example1.c): minimal parse / validation / `ap_format_error(...)` example for options, positionals, and namespace reads
- [sample/example_subcommands.c](./sample/example_subcommands.c): nested subcommands, `subcommand`, and `subcommand_path`
- [sample/example_completion.c](./sample/example_completion.c): formatter APIs for bash / fish / manpage output plus runtime completion callbacks via `__complete`
- [sample/example_manpage.c](./sample/example_manpage.c): subcommand-heavy formatter example for manpage and completion generation from one parser definition
- [sample/example_introspection.c](./sample/example_introspection.c): introspection APIs such as `ap_parser_get_info`, `ap_parser_get_argument`, and `ap_parser_get_subcommand`

## Formatter API quick start

The formatter APIs return heap-allocated strings, so applications can expose generator flags directly:

```c
if (bash_completion) {
  char *script = ap_format_bash_completion(parser);
  printf("%s", script);
  free(script);
  return 0;
}

if (fish_completion) {
  char *script = ap_format_fish_completion(parser);
  printf("%s", script);
  free(script);
  return 0;
}

if (manpage) {
  char *page = ap_format_manpage(parser);
  printf("%s", page);
  free(page);
  return 0;
}
```

### Generate bash completion

```bash
./build/sample/example_completion --generate-bash-completion   > example_completion.bash
source ./example_completion.bash
```

### Generate fish completion

```bash
./build/sample/example_completion --generate-fish-completion   > ~/.config/fish/completions/example_completion.fish
```

### Generate a man page

```bash
./build/sample/example_manpage --generate-manpage > example_manpage.1
man ./example_manpage.1
```

## API Specification

- Japanese: [docs/api-spec.ja.md](./docs/api-spec.ja.md)
- English: [docs/api-spec.en.md](./docs/api-spec.en.md)

## License

MIT License.
