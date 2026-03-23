# argparse-c API Specification (English)

This document describes the public API defined in `include/argparse-c.h`.

## 1. Principles
- All parse APIs return `0` on success and `-1` on failure.
- Error details are stored in `ap_error`.
- The library never calls `exit()`. Application code controls termination.

## 2. Type Definitions

### `ap_type`
- `AP_TYPE_STRING`: string
- `AP_TYPE_INT32`: 32-bit integer
- `AP_TYPE_BOOL`: boolean

### `ap_action`
- `AP_ACTION_STORE`: store a value
- `AP_ACTION_STORE_TRUE`: `true` when present, otherwise `false`
- `AP_ACTION_STORE_FALSE`: `false` when present, otherwise `true`
- `AP_ACTION_APPEND`: append values across repeated uses
- `AP_ACTION_COUNT`: count how many times an option appears
- `AP_ACTION_STORE_CONST`: store `const_value` when present

### `ap_nargs`
- `AP_NARGS_ONE`: exactly one value (default)
- `AP_NARGS_OPTIONAL`: zero or one value (`?`)
- `AP_NARGS_ZERO_OR_MORE`: zero or more values (`*`)
- `AP_NARGS_ONE_OR_MORE`: one or more values (`+`)
- `AP_NARGS_FIXED`: exactly `nargs_count` values

### `ap_error_code`

| Code | Typical trigger | `err.argument` contract | `err.message` family |
| --- | --- | --- | --- |
| `AP_ERR_NONE` | No error recorded | empty string | empty string |
| `AP_ERR_NO_MEMORY` | Allocation failure in parser/build/format paths | usually the owning `dest`, current token, or empty string for parser-wide failures | `out of memory` |
| `AP_ERR_INVALID_DEFINITION` | Invalid API usage or parser definition | invalid flag/name/dest, first conflicting flag, or empty string for parser/group-wide precondition failures | fixed definition-time diagnostics such as `store_const requires const_value` |
| `AP_ERR_UNKNOWN_OPTION` | Strict parse sees an unknown option | the option token exactly as written (for example `--bogus`, `-z`) | `unknown option '...'` |
| `AP_ERR_DUPLICATE_OPTION` | Non-repeatable option is provided multiple times | the primary flag for that option | `duplicate option '...'` |
| `AP_ERR_MISSING_VALUE` | An option/value-bearing argument was seen without a required value | the primary flag for optionals, declared name for positionals | `option '...' requires a value` / `argument '...' requires a value` |
| `AP_ERR_INVALID_NARGS` | Value count violates `nargs` / short-cluster rules | the primary flag for optionals, declared name for positionals | `... requires at least one value`, `... requires exactly N values`, or `option '...' cannot be used in a short option cluster` |
| `AP_ERR_MISSING_REQUIRED` | Required option/argument/subcommand/group missing | primary flag, declared positional name, `subcommand`, or empty string for group-wide failures | `option '...' is required`, `argument '...' is required`, `subcommand is required`, etc. |
| `AP_ERR_INVALID_CHOICE` | Parsed/defaulted value is outside `choices` | the primary flag for optionals, declared name for positionals | `invalid choice 'X' for option '...'` / `... for argument '...'` |
| `AP_ERR_INVALID_INT32` | Integer conversion fails | the primary flag for optionals, declared name for positionals | `argument '...' must be a valid int32: 'X'` |
| `AP_ERR_UNEXPECTED_POSITIONAL` | Extra positional token remains in strict mode | the unexpected token itself | `unexpected positional argument 'TOKEN'` |

### `ap_error`
- `code`: error code
- `argument`: stable argument identifier related to the error
  - optional arguments use the primary flag (`flags[0]`) as stored in the parser definition
  - positional arguments use the declared argument name (`dest` when no flags exist)
  - subcommand-level errors use `subcommand`
  - parser/group-wide errors may leave this empty
  - out-of-memory paths may use a `dest`/token identifier instead of a user-facing label so callers can still correlate the failure source
- `message`: human-readable message with a stable template family
  - optional arguments use `option '...'`
  - positional arguments use `argument '...'`
  - parser/group-wide failures avoid argument wording and use a standalone sentence

### `ap_arg_options`
Options passed to `ap_add_argument`.

- `type`: argument type
- `action`: assignment behavior
- `nargs`: value-count behavior
- `nargs_count`: exact count used by `AP_NARGS_FIXED`
- `required`: whether argument is required
- `help`: help text
- `metavar`: display name in usage/help
- `default_value`: default when not provided (string form)
- `const_value`: value stored by `AP_ACTION_STORE_CONST`
- `choices`: allowed values (`ap_choices`)
- `completion_kind`: completion metadata kind (`ap_completion_kind`)
- `completion_hint`: optional generator-specific hint string for completions
- `completion_callback`: optional application callback for dynamic completion candidates
- `completion_user_data`: opaque pointer passed to `completion_callback`
- `dest`: key used for lookup (auto-generated when omitted)

Use `ap_arg_options_default()` first, then override needed fields.

### `ap_completion_kind`
- `AP_COMPLETION_KIND_NONE`: no explicit completion metadata
- `AP_COMPLETION_KIND_CHOICES`: complete from `choices`
- `AP_COMPLETION_KIND_FILE`: complete file paths
- `AP_COMPLETION_KIND_DIRECTORY`: complete directories
- `AP_COMPLETION_KIND_COMMAND`: complete executable command names

#### Completion metadata rules
- bash / fish generators prefer `completion_kind` when it is not `AP_COMPLETION_KIND_NONE`
- when `completion_kind == AP_COMPLETION_KIND_CHOICES`, generators use `choices` as the completion source
- when `completion_kind == AP_COMPLETION_KIND_NONE` and `choices` is present, generators treat it as the default `CHOICES` behavior
- static metadata remains the baseline contract for parse-time validation and offline generator output
- `completion_hint` is reserved for generator-specific hints and does not change parse-time validation
- callback-style dynamic completion augments, but does not replace, the static metadata rules above

#### Dynamic completion callback contract
The public API also supports runtime completion through `completion_callback`,
`completion_user_data`, and `ap_complete(...)`.

**Callback typedef**

```c
typedef int (*ap_completion_callback)(const ap_completion_request *request,
                                      ap_completion_result *result,
                                      void *user_data,
                                      ap_error *err);
```

The exact helper structs can evolve, but the callback contract should preserve these properties:
- input is read-only and describes the parser path, active option/argument, current token prefix, and shell/generator context
- output is append-only candidate data owned by the caller/runtime, not by the callback after it returns
- return value follows the library convention (`0` success, `-1` failure) with details in `ap_error`
- `user_data` is opaque application state; the library stores and forwards it without interpretation

**`ap_arg_options` callback fields**
- `completion_callback` and `completion_user_data` are part of the public argument definition contract
- `completion_kind` / `choices` / `completion_hint` remain valid even when a callback is present
- precedence is:
  1. if `completion_callback` is set, shells that support dynamic completion query it first
  2. if the callback returns no candidates, completion may fall back to static metadata
  3. parse-time validation still depends only on argument definition fields such as `choices`, not on callback output

**Compatibility rules**
- existing applications that use only static metadata continue to behave exactly the same
- generators that cannot execute application callbacks still emit useful scripts from static metadata alone
- introspection APIs should continue to expose the static fields; callback exposure is optional and should be documented separately because function pointers are not portable serialization data

**Generator strategy**
For bash / fish, generated scripts call back into the application through a completion subcommand.

1. **Embed callback behavior directly into generated scripts**
   - not used by the current public API
   - generated shell code cannot safely serialize a C function pointer
   - shell-specific glue would have to invent a second transport protocol anyway

2. **Call back into the application through a completion subcommand**
   - this is the current direction used by the bash / fish generators
   - generated scripts remain thin wrappers that invoke something like `prog __complete ...`
   - the application reconstructs parse context, calls `ap_completion_callback`, and prints candidates in a shell-neutral format
   - this keeps dynamic policy inside the application binary and lets static metadata remain available for offline fallback

**Contract split**
- library definition layer: declare callback types and store them in `ap_arg_options`
- application/runtime layer: provide a completion entrypoint/subcommand that shells can execute
- generator layer: emit shell adapters that prefer the subcommand path for dynamic candidates and fall back to static metadata when unavailable

This split matches the existing formatter implementations in `lib/format_completion_bash.c` and `lib/format_completion_fish.c`, which currently derive scripts entirely from parser metadata and have no mechanism to embed executable C callbacks.

#### `dest` auto-generation rules
- for optional arguments, the first long flag (for example `--dry-run`) is preferred
- if no long flag exists, the first short flag (for example `-v`) is used
- for positional arguments, the declared name is used
- during auto-generation, `-` is converted to `_` (for example `dry-run` -> `dry_run`)
- when `dest` is provided explicitly, it is kept as-is

## 3. Parser Lifecycle API

### `ap_parser *ap_parser_new(const char *prog, const char *description)`
Creates a parser.

- `prog`: command name shown in usage
- `description`: help description block
- Returns: non-NULL on success, NULL on failure
- Note: `-h/--help` is added automatically

### `ap_parser *ap_add_subcommand(ap_parser *parser, const char *name, const char *description, ap_error *err)`
Adds a child parser for a subcommand.

- `name`: subcommand name
- `description`: help description for the subcommand
- Returns: child parser on success, `NULL` on failure
- Notes:
  - nested subcommands are supported
  - on success, the final selected leaf subcommand name is stored in namespace key `"subcommand"`
  - intermediate subcommand names are not added as separate namespace entries
  - the namespace also stores `"subcommand_path"`, which contains the full selected subcommand chain (for example `"config set"`)
  - `ap_format_help()` for a nested subcommand parser uses the full command path in usage/help text

### `void ap_parser_free(ap_parser *parser)`
Frees the parser.

### `ap_mutually_exclusive_group *ap_add_mutually_exclusive_group(ap_parser *parser, bool required, ap_error *err)`
Adds a mutually exclusive group.

- When `required=true`, at least one member argument must be present

### `int ap_group_add_argument(ap_mutually_exclusive_group *group, const char *name_or_flags, ap_arg_options options, ap_error *err)`
Adds an argument to a mutually exclusive group.

## 4. Argument Definition API

### `int ap_add_argument(ap_parser *parser, const char *name_or_flags, ap_arg_options options, ap_error *err)`
Adds an argument definition.

- `name_or_flags` examples:
  - positional: `"input"`
  - optional: `"-t, --text"`
- Returns: `0` on success, `-1` on failure
- On failure: details in `err`

## 5. Parse API

### `int ap_parse_args(ap_parser *parser, int argc, char **argv, ap_namespace **out_ns, ap_error *err)`
Strict parse mode.

- Unknown options cause an error
- On success, result is stored in `*out_ns`
- Free `*out_ns` with `ap_namespace_free()`

### `int ap_parse_known_args(ap_parser *parser, int argc, char **argv, ap_namespace **out_ns, char ***out_unknown_args, int *out_unknown_count, ap_error *err)`
Known/unknown split mode.

- Unknown tokens are returned via `out_unknown_args`
- Known arguments are returned in `out_ns`
- Free unknown tokens via `ap_free_tokens(out_unknown_args, out_unknown_count)`
- Tokens after `--` are collected as unknown
- Unknown tokens are appended in encounter order
- Collected unknowns include:
  - unknown options
  - the next token after an unknown option when that token is not another option-like token
  - extra positional tokens left after positional binding
  - all tokens after `--`
- Known arguments are still validated, so this API can still fail with missing required arguments or invalid values

#### `nargs` behavior summary
- optional arguments:
  - `AP_NARGS_ONE`: requires exactly one value
  - `AP_NARGS_OPTIONAL`: consumes the next token only when it is not a known option
  - `AP_NARGS_ZERO_OR_MORE` / `AP_NARGS_ONE_OR_MORE`: stop before `--` or the next known option
  - `AP_NARGS_FIXED`: requires exactly `nargs_count` values
- positional arguments:
  - binding proceeds left-to-right
  - `AP_NARGS_OPTIONAL`, `AP_NARGS_ZERO_OR_MORE`, and `AP_NARGS_ONE_OR_MORE` leave enough tokens for the minimum required values of later positional arguments

## 6. Formatting API

### `char *ap_format_usage(const ap_parser *parser)`
Builds usage text.

### `char *ap_format_help(const ap_parser *parser)`
Builds help text.

### `char *ap_format_error(const ap_parser *parser, const ap_error *err)`
Builds combined `error + usage` text.

#### Memory ownership (all above)
Returned strings are heap-allocated. Free them with `free()`.

## 7. Parsed Value Access API

### Single-value access
- `bool ap_ns_get_bool(const ap_namespace *ns, const char *dest, bool *out_value)`
- `bool ap_ns_get_string(const ap_namespace *ns, const char *dest, const char **out_value)`
- `bool ap_ns_get_int32(const ap_namespace *ns, const char *dest, int32_t *out_value)`

### Multi-value access (`nargs=*` / `+`, etc.)
- `int ap_ns_get_count(const ap_namespace *ns, const char *dest)`
- `const char *ap_ns_get_string_at(const ap_namespace *ns, const char *dest, int index)`
- `bool ap_ns_get_int32_at(const ap_namespace *ns, const char *dest, int index, int32_t *out_value)`

### `void ap_namespace_free(ap_namespace *ns)`
Frees namespace results.

### `void ap_free_tokens(char **tokens, int count)`
Frees unknown token arrays returned by `ap_parse_known_args`.

## 8. Memory Management Summary
- `ap_parser_new` -> `ap_parser_free`
- `ap_parse_args` / `ap_parse_known_args` output namespace -> `ap_namespace_free`
- `ap_format_usage/help/error` return values -> `free`
- `ap_parse_known_args` unknown array -> `ap_free_tokens`

## 8.1 Error generation and naming rules

The current implementation in `lib/core_parser.c`, `lib/core_validate.c`, `lib/core_convert.c`, and `lib/api.c` follows these naming rules consistently:

- Optional-argument parse/validation errors use the primary flag (`flags[0]`) for `err.argument`.
  - This means `-t, --text` reports `-t` when the short flag is declared first, and `--mode` when the long flag is declared first.
- Positional-argument parse/validation errors use the declared positional name.
- `invalid choice` messages are label-driven and therefore render as `for option '...'` or `for argument '...'` depending on the argument kind.
- `required`, `missing value`, and `invalid nargs` messages share the same label builder and therefore keep `option '...'` / `argument '...'` wording aligned across parse-time and validation-time failures.
- Integer conversion currently always uses the template `argument '...' must be a valid int32: 'VALUE'` even for optional flags, so the noun is stable but slightly more generic than the other option-specific messages.
- Definition-time API failures in `lib/api.c` often use raw names (`name_or_flags`, conflicting `dest`, subcommand name, or empty string) because the parser definition may not yet have a normalized argument object.
- Parser-wide or mutually exclusive group failures intentionally leave `err.argument` empty and use a sentence-style message.

### `ap_format_error` output contract
`ap_format_error()` prepends `error: ` to `err.message`, appends a newline, and then appends the usage text returned by the same parser.

Example (`prog demo`, missing required option `--mode`):

```text
error: option '--mode' is required
usage: demo [-h] --mode MODE
```

Callers should treat this two-line structure (`error:` line + usage block) as the stable display contract for CLI output.

## 8.2 Regression test plan for error-message consistency
Maintain the following regression coverage so error naming stays stable across future refactors:

- `test/test_parse_core.cpp`
  - strict-parse errors for unknown options, invalid int32 conversion, invalid choice, missing required options, and short-cluster `nargs` failures
- `test/test_known_args.cpp`
  - known/unknown split behavior, especially required-option errors that must still report the primary flag even when unknown tokens are preserved
- `test/test_subcommands_and_validation.cpp`
  - validation-focused cases for missing values, `nargs` cardinality, unexpected positionals, duplicate options, mutually exclusive groups, and subcommand-specific required errors
- `test/test_format_and_api.cpp`
  - public API precondition/definition errors plus `ap_format_error()` golden-output checks that freeze the exact ``error: ...`` + ``usage: ...`` rendering

When extending tests, assert all three fields together where practical: `err.code`, `err.argument`, and `err.message`.

## 8.5 Practical examples and reverse links
- Parse basics and error formatting: [`README.md`](../README.md), [`sample/example1.c`](../sample/example1.c)
- Nested subcommands and `subcommand_path`: [`README.md`](../README.md), [`sample/example_subcommands.c`](../sample/example_subcommands.c)
- Formatter APIs (`ap_format_help`, `ap_format_manpage`, shell completion formatters): [`README.md`](../README.md), [`sample/example_completion.c`](../sample/example_completion.c), [`sample/example_manpage.c`](../sample/example_manpage.c)
- Introspection APIs (`ap_parser_get_info`, `ap_parser_get_argument`, `ap_parser_get_subcommand`): [`README.md`](../README.md), [`sample/example_introspection.c`](../sample/example_introspection.c)

## 9. Minimal Example

```c
ap_error err = {0};
ap_namespace *ns = NULL;
ap_parser *p = ap_parser_new("demo", "demo parser");

ap_arg_options opt = ap_arg_options_default();
opt.required = true;
ap_add_argument(p, "-t, --text", opt, &err);

if (ap_parse_args(p, argc, argv, &ns, &err) != 0) {
  char *msg = ap_format_error(p, &err);
  fprintf(stderr, "%s", msg ? msg : "parse error\n");
  free(msg);
  ap_parser_free(p);
  return 1;
}

/* ... use ap_ns_get_* ... */

ap_namespace_free(ns);
ap_parser_free(p);
```
