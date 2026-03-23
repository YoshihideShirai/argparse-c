# オプションと型

## 対応型

- `AP_TYPE_STRING`
- `AP_TYPE_INT32`
- `AP_TYPE_INT64`
- `AP_TYPE_UINT64`
- `AP_TYPE_DOUBLE`
- `AP_TYPE_BOOL`

## `int32` の例

```c
ap_arg_options number = ap_arg_options_default();
number.type = AP_TYPE_INT32;
number.help = "integer value";
ap_add_argument(parser, "-n, --num", number, &err);
```

不正な整数が与えられた場合は、`AP_ERR_INVALID_INT32` になります。

## `int64` の例

```c
ap_arg_options offset = ap_arg_options_default();
offset.type = AP_TYPE_INT64;
offset.help = "64-bit integer value";
ap_add_argument(parser, "--offset", offset, &err);
```

64bit 整数に変換できない入力は `AP_ERR_INVALID_INT64` になります。

## `uint64` の例

```c
ap_arg_options size = ap_arg_options_default();
size.type = AP_TYPE_UINT64;
size.help = "64-bit unsigned value";
ap_add_argument(parser, "--size", size, &err);
```

符号なし64bit整数に変換できない入力は `AP_ERR_INVALID_UINT64` になります。

## `double` の例

```c
ap_arg_options ratio = ap_arg_options_default();
ratio.type = AP_TYPE_DOUBLE;
ratio.help = "floating-point ratio";
ap_add_argument(parser, "--ratio", ratio, &err);
```

浮動小数点に変換できない入力は `AP_ERR_INVALID_DOUBLE` になります。

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


## `append` と型付き配列

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

追加された値は `ap_ns_get_int64_at(...)` / `ap_ns_get_uint64_at(...)` / `ap_ns_get_double_at(...)` で順番に取り出せます。
