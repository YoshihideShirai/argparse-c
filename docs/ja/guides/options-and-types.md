# オプションと型

## 対応型

- `AP_TYPE_STRING`
- `AP_TYPE_INT32`
- `AP_TYPE_BOOL`

## `int32` の例

```c
ap_arg_options number = ap_arg_options_default();
number.type = AP_TYPE_INT32;
number.help = "integer value";
ap_add_argument(parser, "-n, --num", number, &err);
```

不正な整数が与えられた場合は、`AP_ERR_INVALID_INT32` になります。

## `bool` の例

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

`dest` を明示しない場合、long flag が優先され、`-` は `_` に正規化されます。

例:

- `--dry-run` → `dry_run`
- positional `input-file` → `input_file`
