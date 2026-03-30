# FAQ

## API 参考（计划翻译）

完整 API 文档当前提供英文与日文版本（本地化版本计划中）：
- [API Reference (English)](../api-spec.en.md)
- [API仕様（日本語）](../api-spec.ja.md)

[首页](index.md) | [Getting Started](getting-started.md) | [基础指南](guides/basic-usage.md)

## `argparse-c` 在解析出错时会直接退出进程吗？

不会。解析 API 成功返回 `0`，失败返回 `-1`，详细信息写入 `ap_error`。

## 如何打印帮助信息？

`-h/--help` 会自动添加。如果你需要格式化后的帮助文本，请调用 `ap_format_help(...)`。

## `dest` 是如何确定的？

- 可选参数优先使用第一个长选项
- 如果没有长选项，则使用第一个短选项
- 位置参数使用声明名
- 自动生成时会将 `-` 规范化为 `_`

## 完整 API 参考在哪里？

- Japanese: [API Reference（日本語）](../api-spec.ja.md)
- English: [API Reference (English)](../api-spec.en.md)
