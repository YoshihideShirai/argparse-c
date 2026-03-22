# FAQ

## Does `argparse-c` exit on parse errors?

No. Parse APIs return `0` on success and `-1` on failure, and the details are written into `ap_error`.

## How do I print help?

`-h/--help` is added automatically. If you want the formatted help text, call `ap_format_help(...)`.

## How is `dest` chosen?

- optional arguments prefer the first long flag
- if there is no long flag, the first short flag is used
- positionals use the declared name
- during auto-generation, `-` is normalized to `_`

## Where is the full API reference?

- Japanese: [API Reference（日本語）](../api-spec.ja.md)
- English: [API Reference (English)](../api-spec.en.md)
