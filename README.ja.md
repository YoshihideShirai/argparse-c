# argparse-c

`argparse-c` は、Python の `argparse` に近い書き心地で C99 の CLI を構築できるライブラリです。

> Python `argparse` の使い勝手を C99 に持ち込み、completion・manpage 生成・subcommand・known-args parsing まで対応できます。

[![README (English)](https://img.shields.io/badge/README-English-0A66C2?style=for-the-badge)](./README.md)
[![GitHub%20Pages (日本語)](https://img.shields.io/badge/GitHub%20Pages-日本語-0A66C2?style=for-the-badge)](https://yoshihideshirai.github.io/argparse-c/ja/)

## ここから始める

- **README / English**: [README.md](./README.md)
- **日本語ドキュメント**: [docs/ja/index.md](./docs/ja/index.md)
- **GitHub Pages / 日本語トップ**: <https://yoshihideshirai.github.io/argparse-c/ja/>
- **Getting Started / 日本語**: [docs/ja/getting-started.md](./docs/ja/getting-started.md)

インストール手順、completion の設定、パッケージング、API の詳細は GitHub Pages または `docs/ja/` 配下にまとめています。この README では **ライブラリの概要**、**主な利点**、**最初に読むサンプル**、**日本語ドキュメントへの導線** に絞って説明します。

## 概要

`argparse-c` は、低レベルな引数走査や検証処理を手書きせずに、C99 で実用的なコマンドラインインターフェースを定義したい場面を想定したライブラリです。parser を作成し、option や positional を追加し、`argv` を parse して、namespace から値を取り出します。

主な機能は英語 README と粒度を揃えて管理しています。

- Python `argparse` に着想を得た parser 定義
- option、positional、default、required、`choices`
- `nargs`、append/count/store-const action、mutually exclusive group
- ネストした subcommand
- 同じ parser 定義から bash / fish / zsh の shell completion と manpage を生成
- `ap_parse_known_args(...)` による未知引数の転送
- parse 失敗時も `exit()` を強制せず、アプリ側でエラー処理を制御

## 主な利点

### 1. 慣れたスタイルで CLI を書ける

Python の `argparse` を使ったことがあれば、parser を作る、引数を追加する、parse する、値を読む、という流れをほぼ同じ感覚で扱えます。

### 2. 1 つの定義で実用機能までカバーできる

1 つの parser 定義から、次の機能を一貫して扱えます。

- ヘルプ表示
- shell completion の入口
- manpage 生成
- subcommand ツリー
- wrapper CLI 向けの known / unknown 引数分離

### 3. エラー処理をアプリ側で決められる

`argparse-c` は parse 失敗時に一律で `exit()` しません。構造化されたエラーを受け取り、表示文言や終了コード、回復処理をアプリ側で制御できます。

## 最小サンプル

```c
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new("demo", "demo parser");

  ap_arg_options text = ap_arg_options_default();
  text.required = true;
  text.help = "input text";
  ap_add_argument(parser, "-t, --text", text, &err);

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *message = ap_format_error(parser, &err);
    fprintf(stderr, "%s", message ? message : err.message);
    free(message);
    ap_parser_free(parser);
    return 1;
  }

  {
    const char *value = NULL;
    if (ap_ns_get_string(ns, "text", &value)) {
      printf("text=%s\n", value);
    }
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
```

セットアップ、ビルド手順、次に覚える API は以下を参照してください。

- **日本語ドキュメント**: [docs/ja/index.md](./docs/ja/index.md)
- **Getting Started / 日本語**: [docs/ja/getting-started.md](./docs/ja/getting-started.md)
- **GitHub Pages / 日本語トップ**: <https://yoshihideshirai.github.io/argparse-c/ja/>

## 日本語ドキュメントへのリンク

- [日本語ドキュメント入口](./docs/ja/index.md)
- [Getting Started](./docs/ja/getting-started.md)
- [ガイド一覧](./docs/ja/guides/)
- [API 仕様](./docs/api-spec.ja.md)

## GitHub Pages 日本語トップへのリンク

- <https://yoshihideshirai.github.io/argparse-c/ja/>

## 更新方針

- `README.md` と `README.ja.md` では、概要・主な利点・機能一覧の粒度を大きくずらさずに保ちます。
- README は訴求と導線を担い、詳細な使い方は `docs/ja/getting-started.md` や各 guide に寄せます。
- 詳細説明を追加したくなった場合は、まず docs 側へ追記し、README からはそのページへ案内します。

## セキュリティ検証の現状と適用範囲

- sanitizer と境界ケースを含むテストを CI で継続実行し、coverage を公開しています。
  - CI（tests + sanitizers + coverage）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/ci.yml>
  - Pages（coverage 公開ジョブ）: <https://github.com/yoshihideshirai/argparse-c/actions/workflows/pages.yml>
- セキュリティテスト手順: <https://yoshihideshirai.github.io/argparse-c/ja/security-testing/>
- 公開時点の既知状態: この README の公開時点で、未修正の重大な脆弱性は把握していません。

### 非保証範囲

- 実際に本ライブラリを利用するアプリケーション側の入出力検証は、利用側アプリケーションの責務です。
- OS・ツールチェーン・依存ライブラリなど、実行環境の差異による挙動差やリスクは、本ライブラリ単体では完全には保証できません。

### 更新ルール

- リリースごとに本節の表現を見直し、最新の CI 実行結果とセキュリティテスト手順に合わせて更新します。
