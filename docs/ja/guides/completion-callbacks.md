# Completion callback ガイド

このガイドでは、`include/argparse-c.h` の default-on completion 導線を説明します。対象 API は `ap_parser_options`、`ap_parser_new_with_options(...)`、`ap_parser_set_completion(...)`、`ap_try_handle_completion(...)`、`ap_complete(...)`、`ap_completion_callback`、`ap_completion_request`、`ap_completion_result`、`ap_completion_result_init(...)`、`ap_completion_result_add(...)`、`ap_completion_result_free(...)`、および `ap_arg_options` の completion 関連フィールドです。最小の標準実装は `sample/example_completion.c` にあります。

completion は追加機能ではなく標準導線として扱います。新しい parser では hidden completion entrypoint が既定で有効です。

- 既定の hidden entrypoint: `__complete`
- 既定値: 有効
- 無効化: `ap_parser_set_completion(parser, false, NULL, &err)`
- hidden 名の変更: `ap_parser_set_completion(parser, true, "__ap_complete", &err)` または `ap_parser_new_with_options(...)`
- 予約名ルール: completion が有効な間は、hidden entrypoint と同名の subcommand を `ap_add_subcommand(...)` で追加できません

## 最小実装

推奨フローは次の通りです。

1. parser を作る。
2. 必要なら parser 単位で completion 設定を変更する。
3. `ap_arg_options` で completable な引数を登録する。
4. `ap_parse_args(...)` の前に `ap_try_handle_completion(...)` を呼ぶ。
5. `handled` が真のときだけ候補を出力する。
6. shell script は `ap_format_bash_completion(...)` / `ap_format_fish_completion(...)` / `ap_format_zsh_completion(...)` で生成する。

```c
static int maybe_handle_completion_request(ap_parser *parser, int argc,
                                           char **argv) {
  ap_completion_result result;
  ap_error err = {0};
  int handled = 0;
  int rc = ap_try_handle_completion(parser, argc, argv, "bash", &handled,
                                    &result, &err);

  if (rc != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_completion_result_free(&result);
    return 1;
  }
  if (!handled) {
    return -1;
  }

  for (int i = 0; i < result.count; i++) {
    printf("%s\n", result.items[i].value);
  }
  ap_completion_result_free(&result);
  return 0;
}
```

## Parser 単位の completion 設定

通常は既定値のままで十分です。hidden 名を変えたい場合だけ `ap_parser_options` か setter を使います。

```c
ap_parser_options parser_options = ap_parser_options_default();
parser_options.completion_entrypoint = "__ap_complete";
ap_parser *parser = ap_parser_new_with_options("demo", "desc", parser_options);
```

既存 parser を更新する場合:

```c
ap_parser_set_completion(parser, false, NULL, &err);
ap_parser_set_completion(parser, true, "__ap_complete", &err);

if (!ap_parser_completion_enabled(parser)) {
  fprintf(stderr, "completion は有効のままにしてください\n");
  return 1;
}
```

completion が有効なときは、その hidden entrypoint 名は予約済みです。同名の visible subcommand が必要なら、先に completion を無効化するか hidden 名を別名へ変更してください。

## 候補生成は従来どおり引数 metadata が担う

`ap_try_handle_completion(...)` は transport と委譲だけを担当します。候補そのものは引き続き次の metadata から生成されます。

- `choices` または `completion_kind = AP_COMPLETION_KIND_CHOICES`
- `completion_kind = AP_COMPLETION_KIND_FILE`
- `completion_kind = AP_COMPLETION_KIND_DIRECTORY`
- `completion_kind = AP_COMPLETION_KIND_COMMAND`
- `completion_callback` + `completion_user_data`

同じ metadata は、現在入力中の positional 引数にも適用されます。`ap_complete(...)` または `ap_try_handle_completion(...)` が positional を解決した場合、`ap_completion_request.active_option` は `NULL` になり、`ap_completion_request.dest` にはその positional の destination 名が入ります。

```c
static const char *const task_names[] = {"build", "bench", "bundle", NULL};

static int task_completion(const ap_completion_request *request,
                           ap_completion_result *result, void *user_data,
                           ap_error *err) {
  const char *prefix =
      request && request->current_token ? request->current_token : "";
  const char *const *items = (const char *const *)user_data;

  if (!request || request->active_option != NULL ||
      strcmp(request->dest, "task") != 0) {
    return 0;
  }
  for (int i = 0; items[i] != NULL; i++) {
    if (strncmp(items[i], prefix, strlen(prefix)) == 0 &&
        ap_completion_result_add(result, items[i], "task", err) != 0) {
      return -1;
    }
  }
  return 0;
}

ap_arg_options input = ap_arg_options_default();
input.completion_kind = AP_COMPLETION_KIND_FILE;
ap_add_argument(parser, "input", input, &err);

static const char *const format_choices[] = {"json", "yaml", "toml"};
ap_arg_options format = ap_arg_options_default();
format.choices.items = format_choices;
format.choices.count = 3;
ap_add_argument(parser, "format", format, &err);

ap_arg_options task = ap_arg_options_default();
task.completion_callback = task_completion;
task.completion_user_data = (void *)task_names;
ap_add_argument(parser, "task", task, &err);
```

helper を使わずに `ap_complete(...)` を直接呼ぶことも可能です。

## shell への導入手順

### bash

```bash
./build/sample/example_completion --generate-bash-completion > ./example_completion.bash
mkdir -p ~/.local/share/bash-completion/completions
cp ./example_completion.bash ~/.local/share/bash-completion/completions/example_completion
source ~/.local/share/bash-completion/completions/example_completion
```

### fish

```bash
mkdir -p ~/.config/fish/completions
./build/sample/example_completion --generate-fish-completion \
  > ~/.config/fish/completions/example_completion.fish
```

### zsh

```bash
mkdir -p ~/.zsh/completions
./build/sample/example_completion --generate-zsh-completion \
  > ~/.zsh/completions/_example_completion
fpath=(~/.zsh/completions $fpath)
autoload -Uz compinit && compinit
```

生成された script が configured hidden entrypoint を内部的に呼び出すため、エンドユーザーは `__complete` やカスタム名を意識する必要はありません。

## fallback 方針

「候補が 0 件」は正常系として扱います。

1. 動的データが無ければ `0` を返して何も追加しない。
2. prefix 絞り込みで 0 件になっても `0` を返す。
3. callback が 0 件でも `choices` や `completion_kind` は維持する。
4. completion 処理自体が壊れた場合だけ `-1` を返す。
