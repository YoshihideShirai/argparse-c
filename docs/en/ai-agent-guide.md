# AI Agent Guide

This page is a quick contract for AI coding agents that generate or modify `argparse-c` consumers.

## Read first (recommended order)

1. [`../api-spec.json`](../api-spec.json) — machine-readable ownership and API signatures.
2. [`getting-started.md`](getting-started.md) — baseline parser lifecycle and build flow.
3. [`guides/basic-usage.md`](guides/basic-usage.md) — practical parse/error handling pattern.
4. [`guides/known-args.md`](guides/known-args.md) — wrapper CLI routing with unknown tokens.

## Ownership / cleanup checklist

### `ap_format_*` return values (`char *`)

All formatter APIs return heap-allocated strings. The caller must release them with `free()` when non-NULL.

- `ap_format_usage(...)`
- `ap_format_help(...)`
- `ap_format_manpage(...)`
- `ap_format_bash_completion(...)`
- `ap_format_fish_completion(...)`
- `ap_format_zsh_completion(...)`
- `ap_format_error(...)`

### Parser / namespace cleanup order

Use this lifecycle order consistently:

1. `ap_parser *parser = ap_parser_new(...)`
2. parse to get `ap_namespace *ns`
3. free namespace first: `ap_namespace_free(ns)`
4. free parser last: `ap_parser_free(parser)`

Rationale: treat `ns` as parse output derived from `parser`; release the derived object first.

### Error-path cleanup matrix

| Resource state | Required cleanup |
| --- | --- |
| parser creation failed (`parser == NULL`) | return error (nothing to free) |
| parser exists, parse failed | `ap_format_error(...)` result -> `free(...)`; then `ap_parser_free(parser)` |
| parser exists, parse succeeded | `ap_namespace_free(ns)` then `ap_parser_free(parser)` |
| known-args parse succeeded | `ap_free_tokens(unknown, unknown_count)` in addition to namespace/parser cleanup |

## `ap_parse_args` vs `ap_parse_known_args` (wrapper CLI policy)

Use **`ap_parse_args`** when your CLI owns the full argument contract and should reject unknown options immediately.

Use **`ap_parse_known_args`** when your CLI is a **wrapper/front controller** that consumes a subset of flags and forwards the rest to another command/runtime.

For wrapper CLIs, default to:

- parse wrapper-owned flags with `ap_parse_known_args(...)`
- pass `unknown[]` through unchanged (order preserved)
- free `unknown[]` with `ap_free_tokens(...)` after forwarding logic is done

## Good / bad examples (minimal snippets from `sample/` patterns)

### Good: parse failure path frees formatted error + parser

```c
ap_error err = {0};
ap_namespace *ns = NULL;
ap_parser *parser = ap_parser_new("demo", "demo parser");
if (!parser) return 1;

if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *msg = ap_format_error(parser, &err);
  fprintf(stderr, "%s", msg ? msg : err.message);
  free(msg);
  ap_parser_free(parser);
  return 1;
}

ap_namespace_free(ns);
ap_parser_free(parser);
```

### Good: help text generation frees `ap_format_help` result

```c
char *help = ap_format_help(parser);
if (help) {
  fputs(help, stdout);
  free(help);
}
```

### Good: wrapper CLI split and cleanup

```c
char **unknown = NULL;
int unknown_count = 0;
if (ap_parse_known_args(parser, argc, argv, &ns, &unknown, &unknown_count, &err) == 0) {
  /* forward unknown[0..unknown_count-1] */
  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
}
ap_parser_free(parser);
```

### Bad: leaked formatter string

```c
char *msg = ap_format_error(parser, &err);
fprintf(stderr, "%s", msg ? msg : "error");
/* missing free(msg); */
```

### Bad: wrong teardown order on success

```c
ap_parser_free(parser);
ap_namespace_free(ns); /* avoid this order */
```
