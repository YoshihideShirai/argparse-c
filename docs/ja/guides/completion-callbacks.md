# Completion callback ガイド

このガイドでは、`ap_complete(...)` と `ap_completion_callback` を使った **アプリ側 completion エントリポイント** の作り方を説明します。

shell script の生成だけが目的なら、まずは `README.md` にある generator API の説明を参照してください。実行時の状態に応じて候補を変えたい場合は、`__complete` エントリポイントと completion callback を追加します。実装全体の対応サンプルは `sample/example_completion.c` です。

## 設計の全体像

completion 関連の公開 API は `include/argparse-c.h` にあります。

- `ap_complete(...)`: ライブラリ側の completion 解決を実行する
- `ap_completion_callback`: 引数ごとに動的候補を追加する callback 型
- `ap_completion_request`: callback 側で現在のトークンや shell 情報を読むための入力
- `ap_completion_result_init(...)` / `ap_completion_result_add(...)` / `ap_completion_result_free(...)`: 候補配列の初期化・追加・解放
- `ap_arg_options.completion_kind` / `completion_hint` / `completion_callback` / `completion_user_data`: 各引数の completion 挙動を宣言する設定

実運用では、次の方針にすると扱いやすいです。

1. 通常の CLI 動作はそのまま維持する
2. `argv[1] == "__complete"` を内部用エントリポイントとして予約する
3. `--shell bash` のような wrapper 専用引数を取り除いてから `ap_complete(...)` を呼ぶ
4. 候補は 1 行 1 件で stdout へ出す
5. completion 処理そのものが失敗したときだけ非 0 で終了する

この構成は `sample/example_completion.c` と揃っているため、サンプルをそのまま出発点にできます。

## 最小構成の parser 設定

最小でも、静的な metadata と callback 付き引数を 1 つ組み合わせると実用的です。

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

この例は `sample/example_completion.c` と意図的に対応させてあり、ガイドと完全なサンプルを見比べながら実装できます。

## `__complete` エントリポイントの設計

ライブラリは、shell script と実行ファイルの間で使う transport protocol を固定しません。実装しやすい推奨パターンは、private な subcommand 風エントリポイントを置くことです。

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

- `__complete` を `ap_parse_args(...)` より前に簡単に判定できる
- `--shell` を公開 CLI 仕様の外側に置ける
- `--` で wrapper 用情報と元のコマンドラインを明確に分離できる
- 通常実行・generator flags・completion request を 1 つのバイナリで兼用できる

## shell 側からの呼び出し方

`ap_format_bash_completion(...)` と `ap_format_fish_completion(...)` が生成する script が、公開側の安定した integration point です。これらの script から、内部的に実行ファイルの `__complete` エントリポイントを呼び出し、現在のコマンドラインを渡します。

手動確認用の呼び出し例は次の通りです。

```bash
./build/sample/example_completion __complete --shell bash -- --mode f
./build/sample/example_completion __complete --shell bash -- --exec g
./build/sample/example_completion __complete --shell fish -- input.tx
```

この呼び出し規約では、

- `__complete` が completion mode を選ぶ
- `--shell bash` / `--shell fish` が `ap_complete(...)` に shell 種別を伝える
- `--` が元のコマンドライン開始位置を示す
- その後ろの argv をそのまま `ap_complete(...)` へ渡す

エンドユーザー向けの一般的な手順は、引き続き次の generator API ベースです。

```bash
./build/sample/example_completion --generate-bash-completion > example_completion.bash
source ./example_completion.bash
```

この script が裏側で `__complete` 呼び出しを行います。

## 候補が返せない場合の fallback 方針

「候補がない」はエラーではなく、正常系として扱うのが基本です。

推奨する fallback 方針は次の通りです。

1. **動的データがない場合** は `0` を返し、何も追加しない
2. **prefix 絞り込みで候補が 0 件になった場合** も `0` を返し、何も追加しない
3. **引数に `choices` や `completion_kind` がある場合** は、callback が 0 件でもその宣言を残す
4. **`ap_completion_result_add(...)` の失敗など completion 処理自体が壊れた場合だけ** `-1` を返す

この方針にすると、shell 側は自然に fallback できます。

- 静的な `choices` があればそれを使える
- `AP_COMPLETION_KIND_FILE` や `AP_COMPLETION_KIND_COMMAND` のような組み込み種別も活かせる
- stdout が空なら「今はアプリから追加候補がない」と解釈できる

実装上は、ダミー候補を返すよりも次の形を優先してください。

```c
if (!commands) {
  return 0;
}
```

## サンプルとの対応関係

`sample/example_completion.c` を、completion callback まわりの canonical な end-to-end サンプルとして参照してください。

- generator flags: `--generate-bash-completion`, `--generate-fish-completion`, `--generate-manpage`
- hidden completion entrypoint: `__complete`
- 静的 completion metadata: `choices`, `AP_COMPLETION_KIND_FILE`, `AP_COMPLETION_KIND_COMMAND`
- 動的候補の供給元: `exec_completion_callback(...)`

最短で導入するなら、まず sample の `maybe_handle_completion_request(...)` を取り込み、そのあと `exec_candidates` を自分のアプリのデータ取得処理に置き換えるのが簡単です。
