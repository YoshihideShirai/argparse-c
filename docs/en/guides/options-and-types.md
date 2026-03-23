# Options and types

## Supported types

- `AP_TYPE_STRING`
- `AP_TYPE_INT32`
- `AP_TYPE_INT64`
- `AP_TYPE_UINT64`
- `AP_TYPE_DOUBLE`
- `AP_TYPE_BOOL`

## `int32` example

```c
ap_arg_options number = ap_arg_options_default();
number.type = AP_TYPE_INT32;
number.help = "integer value";
ap_add_argument(parser, "-n, --num", number, &err);
```

Invalid integer input produces `AP_ERR_INVALID_INT32`.

## `int64` example

```c
ap_arg_options offset = ap_arg_options_default();
offset.type = AP_TYPE_INT64;
offset.help = "64-bit integer value";
ap_add_argument(parser, "--offset", offset, &err);
```

Invalid 64-bit integer input produces `AP_ERR_INVALID_INT64`.

## `uint64` example

```c
ap_arg_options size = ap_arg_options_default();
size.type = AP_TYPE_UINT64;
size.help = "64-bit unsigned value";
ap_add_argument(parser, "--size", size, &err);
```

Invalid unsigned input produces `AP_ERR_INVALID_UINT64`.

## `double` example

```c
ap_arg_options ratio = ap_arg_options_default();
ratio.type = AP_TYPE_DOUBLE;
ratio.help = "floating-point ratio";
ap_add_argument(parser, "--ratio", ratio, &err);
```

Invalid floating-point input produces `AP_ERR_INVALID_DOUBLE`.

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


## `append` with typed arrays

```c
ap_arg_options ids = ap_arg_options_default();
ids.type = AP_TYPE_INT64;
ids.action = AP_ACTION_APPEND;
ap_add_argument(parser, "--id", ids, &err);

ap_arg_options weights = ap_arg_options_default();
weights.type = AP_TYPE_DOUBLE;
weights.action = AP_ACTION_APPEND;
ap_add_argument(parser, "--weight", weights, &err);
```

Use `ap_ns_get_int64_at(...)`, `ap_ns_get_uint64_at(...)`, and `ap_ns_get_double_at(...)` to read typed appended values.
