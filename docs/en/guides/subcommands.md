# Subcommands

`argparse-c` supports nested subcommands.

## Adding subcommands

```c
ap_parser *config = ap_add_subcommand(parser, "config", "config commands", &err);
ap_parser *set = ap_add_subcommand(config, "set", "set a value", &err);
```

## Parse result

On success, the namespace exposes a built-in `"subcommand"` key.

- it stores the final selected **leaf subcommand name**
- intermediate subcommands are not stored as separate namespace keys
- `subcommand_path` is not part of the current public contract

## Example

```c
const char *subcommand = NULL;
ap_ns_get_string(ns, "subcommand", &subcommand);
```

```bash
prog config set theme dark
```

In that case, `subcommand` becomes `set`.

## Help output

Nested subcommand help uses the full command path.

Example:

```text
usage: prog config set
```
