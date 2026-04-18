# Subcommands

`argparse-c` supports nested subcommands.

## Adding subcommands

```c
ap_parser *config = ap_add_subcommand(parser, "config", "config commands", &err);
ap_parser *set = ap_add_subcommand(config, "set", "set a value", &err);
```

## Parse result

On success, the namespace exposes built-in `"subcommand"` and `"subcommand_path"` keys.

- `subcommand` stores the final selected **leaf subcommand name**
- intermediate subcommands are not stored as separate namespace keys
- `subcommand_path` stores the full selected subcommand chain

## Example

```c
const char *subcommand = NULL;
const char *subcommand_path = NULL;
ap_ns_get_string(ns, "subcommand", &subcommand);
ap_ns_get_string(ns, "subcommand_path", &subcommand_path);
```

```bash
prog config set theme dark
```

In that case, `subcommand` becomes `set`, and `subcommand_path` becomes `config set`.

## Help output

Nested subcommand help uses the full command path.

Example:

```text
usage: prog config set
```
