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
- `AP_ERR_NONE`
- `AP_ERR_NO_MEMORY`
- `AP_ERR_INVALID_DEFINITION`
- `AP_ERR_UNKNOWN_OPTION`
- `AP_ERR_DUPLICATE_OPTION`
- `AP_ERR_MISSING_VALUE`
- `AP_ERR_INVALID_NARGS`
- `AP_ERR_MISSING_REQUIRED`
- `AP_ERR_INVALID_CHOICE`
- `AP_ERR_INVALID_INT32`
- `AP_ERR_UNEXPECTED_POSITIONAL`

### `ap_error`
- `code`: error code
- `argument`: stable argument identifier related to the error
  - optional arguments use the primary flag (for example `--mode` or `-t`)
  - positional arguments use the declared argument name
  - parser-level errors may leave this empty
- `message`: human-readable message with consistent `option '...'` / `argument '...'` wording

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
- `dest`: key used for lookup (auto-generated when omitted)

Use `ap_arg_options_default()` first, then override needed fields.

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
  - on success, the final selected subcommand name is stored in namespace key `"subcommand"`

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
