# Options and types

## Supported types

- `AP_TYPE_STRING`
- `AP_TYPE_INT32`
- `AP_TYPE_BOOL`

## `int32` example

```c
ap_arg_options number = ap_arg_options_default();
number.type = AP_TYPE_INT32;
number.help = "integer value";
ap_add_argument(parser, "-n, --num", number, &err);
```

Invalid integer input produces `AP_ERR_INVALID_INT32`.

## `bool` example

```c
ap_arg_options verbose = ap_arg_options_default();
verbose.action = AP_ACTION_STORE_TRUE;
verbose.help = "enable verbose output";
ap_add_argument(parser, "-v, --verbose", verbose, &err);
```

## `default_value`

```c
ap_arg_options name = ap_arg_options_default();
name.default_value = "guest";
ap_add_argument(parser, "--name", name, &err);
```

## `choices`

```c
static const char *choices[] = {"fast", "slow"};

ap_arg_options mode = ap_arg_options_default();
mode.choices.items = choices;
mode.choices.count = 2;
ap_add_argument(parser, "--mode", mode, &err);
```

## `dest`

When `dest` is not explicitly set, the first long flag is preferred and `-` is normalized to `_`.

Examples:

- `--dry-run` → `dry_run`
- positional `input-file` → `input_file`
