# 基础指南

## API 参考（计划翻译）

完整 API 文档当前提供英文与日文版本（本地化版本计划中）：
- [API Reference (English)](../../api-spec.en.md)
- [API仕様（日本語）](../../api-spec.ja.md)

[首页](../index.md) | [Getting Started](../getting-started.md) | [FAQ](../faq.md)

如果你刚开始使用 `argparse-c`，最容易记住的是这四步流程：

1. 使用 `ap_parser_new(...)` 创建 parser
2. 使用 `ap_add_argument(...)` 定义参数
3. 使用 `ap_parse_args(...)` 解析命令行
4. 使用 `ap_ns_get_*` 读取结果

## 可选参数

```c
ap_arg_options text = ap_arg_options_default();
text.required = true;
text.help = "text value";
ap_add_argument(parser, "-t, --text", text, &err);
```

- 同时支持 `-t` 与 `--text`
- 当 `required = true` 时为必填

## 位置参数

```c
ap_arg_options input = ap_arg_options_default();
input.help = "input file";
ap_add_argument(parser, "input", input, &err);
```

- 位置参数声明时不带 flag
- 在 usage/help 中以位置参数条目展示

## 错误处理

`argparse-c` 不会在库内部调用 `exit()`。失败时请检查 `ap_error`，并可用 `ap_format_error(...)` 生成面向用户的错误文本。

```c
if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
  char *message = ap_format_error(parser, &err);
  fprintf(stderr, "%s", message ? message : err.message);
  free(message);
}
```

## 帮助输出

`-h/--help` 会自动添加。

```c
char *help = ap_format_help(parser);
printf("%s", help);
free(help);
```
