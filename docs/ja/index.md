# argparse-c

Python の `argparse` に近い書き味で、C99 の CLI を組み立てるためのドキュメント入口です。

`argparse-c` は、単純な flag parsing だけでなく、completion、manpage 生成、subcommands、`nargs`、known-args parsing までを 1 つの parser 定義から扱いたい場面を想定しています。

## このページから始める理由

このサイトでは次をすぐ辿れます。

- source / release asset からのインストール手順
- 最小 CLI サンプルと、その次に読むべきガイド
- completion や manpage を追加する実践的な導線
- 最後に API 仕様へ戻るための入口

## おすすめの読み順

1. **[Getting Started](getting-started.md)** — インストール、最初の sample 実行、最小 parser フロー
2. **Guides** — 必要になった機能ごとに読む
   - [基本の使い方](guides/basic-usage.md)
   - [オプションと型](guides/options-and-types.md)
   - [nargs](guides/nargs.md)
   - [Subcommands](guides/subcommands.md)
   - [Completion callback](guides/completion-callbacks.md)
3. **Reference**
   - [API仕様（日本語）](../api-spec.ja.md)
   - [FAQ](faq.md)

## すぐ作りやすい CLI の例

- 必須 option と positional を持つ単機能 CLI
- ネストした subcommand を持つ CLI
- `ap_parse_known_args(...)` で未知引数を転送する wrapper CLI
- bash / fish completion script と manpage を同じ定義から出力する CLI

## このリポジトリのサンプル

- [`sample/example1.c`](../repository/sample/example1.c.md) — 最小の parse / validate / 値取得
- [`sample/example_completion.c`](../repository/sample/example_completion.c.md) — completion / manpage generator の入口
- [`sample/example_subcommands.c`](../repository/sample/example_subcommands.c.md) — nested subcommands と `subcommand_path`

## English docs も使う

- [English index](../en/index.md)
- [English Getting Started](../en/getting-started.md)
