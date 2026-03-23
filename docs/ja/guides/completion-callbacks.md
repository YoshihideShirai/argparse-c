# Completion callback ガイド

このガイドでは、`include/argparse-c.h` で公開されている completion 関連 API、すなわち `ap_complete(...)`、`ap_completion_callback`、`ap_completion_request`、`ap_completion_result`、`ap_completion_result_init(...)`、`ap_completion_result_add(...)`、`ap_completion_result_free(...)`、および `ap_arg_options` の completion 用フィールドを使って、実行時状態に応じた shell completion を有効化する方法を説明します。エンドツーエンドの参照実装は `sample/example_completion.c` です。

静的な shell script を出力するだけなら `ap_format_bash_completion(...)` や `ap_format_fish_completion(...)` から始めれば十分です。アプリの状態に応じて候補を変えたい場合は、隠し `__complete` エントリポイントを追加し、必要な引数に `ap_arg_options.completion_callback` を設定します。

## completion を有効化する API はどれか

利用者が「どの API を使えば completion を有効化できるのか」を追いやすいように、最短経路を API 名そのままで並べると次の順番です。

1. `ap_parser_new(...)` で parser を作る。
2. `ap_arg_options` で completion 対応したい引数を設定する。
   - `completion_kind` で `AP_COMPLETION_KIND_FILE` や `AP_COMPLETION_KIND_COMMAND` のような組み込み挙動を宣言する。
   - `completion_hint` で短い補足ヒントを付ける。
   - `completion_callback` で動的候補を返す callback を登録する。
   - `completion_user_data` で callback に渡すアプリ側データを持たせる。
   - 固定列挙なら `choices` もそのまま使える。
3. `ap_add_argument(...)` でその引数を登録する。
4. `main(...)` に内部用 `__complete` エントリポイントを置く。
5. `ap_completion_result_init(...)` で結果を初期化し、`ap_complete(...)` を呼び、`ap_completion_result.items[i].value` を 1 行ずつ出力し、最後に `ap_completion_result_free(...)` で解放する。
6. `ap_format_bash_completion(...)` または `ap_format_fish_completion(...)` で shell 統合用 script を公開する。

この並びは `include/argparse-c.h` の公開名と一致しているので、ガイドとヘッダを往復しながら確認できます。

## `sample/example_completion.c` ベースの最小実装

サンプルでは、静的 `choices` を持つ引数、ファイル補完する positional 引数、callback 付き option を 1 つずつ使っています。これはアプリ実装の最小テンプレートとしても扱いやすい構成です。

```c
#include <stdio.h>
#include <string.h>

#include "argparse-c.h"

static int exec_completion_callback(const ap_completion_request *request,
                                    ap_completion_result *result,
                                    void *user_data, ap_error *err) {
  const char *const *commands = (const char *const *)user_data;
  const char *prefix = request && request->current_token ? request->current_token : "";
  int i;

  if (!commands) {
    return 0;
  }
  for (i = 0; commands[i] != NULL; i++) {
    if (strncmp(commands[i], prefix, strlen(prefix)) == 0 &&
        ap_completion_result_add(result, commands[i], "dynamic command", err) != 0) {
      return -1;
    }
  }
  return 0;
}

static ap_parser *build_parser(ap_error *err) {
  static const char *mode_choices[] = {"fast", "slow"};
  static const char *const exec_candidates[] = {"git", "grep", "ls", "sed", NULL};
  ap_parser *parser = ap_parser_new("demo", "completion callback demo");
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();

  if (!parser) {
    return NULL;
  }

  mode.help = "execution mode";
  mode.choices.items = mode_choices;
  mode.choices.count = 2;

  input.help = "input file";
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  input.completion_hint = "path to an input file";

  exec.help = "program to launch";
  exec.completion_kind = AP_COMPLETION_KIND_COMMAND;
  exec.completion_hint = "command name";
  exec.completion_callback = exec_completion_callback;
  exec.completion_user_data = (void *)exec_candidates;

  if (ap_add_argument(parser, "--mode", mode, err) != 0 ||
      ap_add_argument(parser, "--exec", exec, err) != 0 ||
      ap_add_argument(parser, "input", input, err) != 0) {
    ap_parser_free(parser);
    return NULL;
  }

  return parser;
}
```

このコードは `sample/example_completion.c` の構成と揃えてあるため、ガイドとサンプルを見比べながら導入できます。

## `__complete` エントリポイント設計

ライブラリは、生成された shell script と実行ファイルの間で使う transport protocol を固定しません。アプリ側で実装しやすい慣例は、`argv[1] == "__complete"` を内部用エントリポイントとして予約することです。

```c
static int maybe_handle_completion_request(ap_parser *parser, int argc,
                                           char **argv) {
  ap_completion_result result;
  ap_error err = {0};
  const char *shell = "bash";
  int arg_index = 2;
  int i;

  if (argc <= 1 || strcmp(argv[1], "__complete") != 0) {
    return -1;
  }

  ap_completion_result_init(&result);
  if (arg_index < argc && strcmp(argv[arg_index], "--shell") == 0 &&
      arg_index + 1 < argc) {
    shell = argv[arg_index + 1];
    arg_index += 2;
  }
  if (arg_index < argc && strcmp(argv[arg_index], "--") == 0) {
    arg_index++;
  }

  if (ap_complete(parser, argc - arg_index, argv + arg_index, shell, &result,
                  &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_completion_result_free(&result);
    return 1;
  }

  for (i = 0; i < result.count; i++) {
    printf("%s\n", result.items[i].value);
  }
  ap_completion_result_free(&result);
  return 0;
}
```

この構成が扱いやすい理由は次の通りです。

- `__complete` を `ap_parse_args(...)` の前に簡単に判定できる。
- `--shell` のような wrapper 専用引数を公開 CLI 契約の外側に置ける。
- `--` によって wrapper 情報と元のコマンドラインを明確に分離できる。
- 通常実行、completion script 生成、実行時 completion 要求を 1 つのバイナリにまとめられる。
- 実装形が `sample/example_completion.c` と直接対応する。

## shell 統合が `__complete` に到達する流れ

エンドユーザーに公開する安定した統合ポイントは、引き続き生成された shell script です。

- `ap_format_bash_completion(...)`
- `ap_format_fish_completion(...)`

これらの API が返す script が、内部的にバイナリの `__complete` エントリポイントを呼び出し、現在のコマンドラインを転送します。つまり、利用者が見るのは generator API ですが、候補生成の本体はアプリ側の `ap_complete(...)` と callback にあります。

手動確認用の smoke test は次の通りです。

```bash
./build/sample/example_completion __complete --shell bash -- --mode f
./build/sample/example_completion __complete --shell bash -- --exec g
./build/sample/example_completion __complete --shell fish -- input.tx
```

この呼び出し規約では、

- `__complete` が completion mode を選ぶ。
- `--shell bash` / `--shell fish` が `ap_complete(...)` に shell 種別を伝える。
- `--` が元の argv スライス開始位置を示す。
- 以降のトークンを `ap_complete(...)` へそのまま渡す。

実際の導入手順は、利用者から見れば次の generator API ベースで十分です。

```bash
./build/sample/example_completion --generate-bash-completion > example_completion.bash
source ./example_completion.bash
```

生成された script が `__complete` の transport detail を隠してくれます。

## 候補生成に失敗した、または候補が空だった場合の fallback 方針

「候補がない」は正常系として扱い、completion 処理そのものが壊れたときだけ失敗扱いにするのが基本です。

推奨する fallback 方針は次の通りです。

1. 動的データが無い場合は `0` を返し、何も追加しない。
2. prefix 絞り込みで 0 件になった場合も `0` を返し、何も追加しない。
3. `choices` や `completion_kind` が宣言済みなら、callback が 0 件でもその静的 metadata を残す。
4. `ap_completion_result_add(...)` の失敗のように completion 処理自体が壊れた場合だけ `-1` を返す。
5. stdout が空なら「今はアプリから追加候補が無い」と解釈できるようにして、shell や静的 metadata の fallback を妨げない。

実装では、ダミー値を返すより次の形を優先してください。

```c
if (!commands) {
  return 0;
}
```

## サンプルとの対応関係

`sample/example_completion.c` は completion フロー全体を追うための canonical なサンプルです。

- `ap_arg_options.completion_kind`、`ap_arg_options.completion_hint`、`ap_arg_options.completion_callback`、`ap_arg_options.completion_user_data` を使った parser 設定
- `maybe_handle_completion_request(...)` による隠し `__complete` エントリポイント
- `ap_complete(...)` によるライブラリ側の候補解決
- `ap_completion_result_init(...)`、`ap_completion_result_add(...)`、`ap_completion_result_free(...)` による結果管理
- `--generate-bash-completion`、`--generate-fish-completion` による shell 統合

最短で導入するなら、sample の `maybe_handle_completion_request(...)` を取り込み、`ap_complete(...)` の呼び方をそのまま維持した上で、`exec_candidates` だけを自分のデータ取得処理に置き換えるのが簡単です。
