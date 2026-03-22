# nargs

`nargs` は、引数がいくつ値を受け取るかを指定する仕組みです。

## 対応している値

- `AP_NARGS_ONE`
- `AP_NARGS_OPTIONAL`
- `AP_NARGS_ZERO_OR_MORE`
- `AP_NARGS_ONE_OR_MORE`
- `AP_NARGS_FIXED`

## optional 引数での考え方

- `AP_NARGS_ONE`: ちょうど 1 つ必要
- `AP_NARGS_OPTIONAL`: 0 または 1 つ
- `AP_NARGS_ZERO_OR_MORE`: 0 個以上
- `AP_NARGS_ONE_OR_MORE`: 1 個以上
- `AP_NARGS_FIXED`: `nargs_count` 個ちょうど

## 例

```c
ap_arg_options files = ap_arg_options_default();
files.nargs = AP_NARGS_ONE_OR_MORE;
ap_add_argument(parser, "--files", files, &err);
```

```bash
prog --files a.txt b.txt c.txt
```

## positional 引数での考え方

positional では、後続の required positional に必要な最小数を残すように割り当てられます。

この仕様の詳細は [API Reference（日本語）](../api-spec.ja.md) も参照してください。
