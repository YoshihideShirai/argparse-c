# nargs

`nargs` controls how many values an argument consumes.

## Supported variants

- `AP_NARGS_ONE`
- `AP_NARGS_OPTIONAL`
- `AP_NARGS_ZERO_OR_MORE`
- `AP_NARGS_ONE_OR_MORE`
- `AP_NARGS_FIXED`

## Optional argument behavior

- `AP_NARGS_ONE`: requires exactly one value
- `AP_NARGS_OPTIONAL`: accepts zero or one value
- `AP_NARGS_ZERO_OR_MORE`: accepts zero or more values
- `AP_NARGS_ONE_OR_MORE`: accepts one or more values
- `AP_NARGS_FIXED`: requires exactly `nargs_count` values

## Example

```c
ap_arg_options files = ap_arg_options_default();
files.nargs = AP_NARGS_ONE_OR_MORE;
ap_add_argument(parser, "--files", files, &err);
```

```bash
prog --files a.txt b.txt c.txt
```

## Positional argument behavior

For positionals, tokens are assigned while preserving the minimum number of tokens required by later positional arguments.

See the [English API Reference](../../api-spec.en.md) for the full contract.
