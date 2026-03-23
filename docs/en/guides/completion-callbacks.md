# Completion callbacks

This guide explains the default-on completion flow exposed by `include/argparse-c.h`: `ap_parser_options`, `ap_parser_new_with_options(...)`, `ap_parser_set_completion(...)`, `ap_try_handle_completion(...)`, `ap_complete(...)`, `ap_completion_callback`, `ap_completion_request`, `ap_completion_result`, `ap_completion_result_init(...)`, `ap_completion_result_add(...)`, `ap_completion_result_free(...)`, and the completion-related fields on `ap_arg_options`. The end-to-end minimal sample is `sample/example_completion.c`.

Completion is now treated as part of the standard application path. New parsers enable a hidden completion entrypoint by default.

- default hidden entrypoint: `__complete`
- default behavior: enabled
- opt out: `ap_parser_set_completion(parser, false, NULL, &err)`
- custom hidden entrypoint: `ap_parser_set_completion(parser, true, "__ap_complete", &err)` or `ap_parser_new_with_options(...)`
- reserved-name rule: while completion is enabled, a subcommand with the same name as the hidden entrypoint is rejected by `ap_add_subcommand(...)`

## Minimal implementation

The recommended wiring is now:

1. Create the parser.
2. Optionally customize or disable completion at parser scope.
3. Register completable arguments through `ap_arg_options`.
4. Call `ap_try_handle_completion(...)` before `ap_parse_args(...)`.
5. Print completion candidates only when `out_handled` is true.
6. Generate shell scripts with `ap_format_bash_completion(...)` or `ap_format_fish_completion(...)`.

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

## Parser-level completion settings

Use the default settings unless you need an explicit opt-out or a custom hidden command name.

```c
ap_parser_options parser_options = ap_parser_options_default();
parser_options.completion_entrypoint = "__ap_complete";
ap_parser *parser = ap_parser_new_with_options("demo", "desc", parser_options);
```

Or update an existing parser:

```c
ap_parser_set_completion(parser, false, NULL, &err);
ap_parser_set_completion(parser, true, "__ap_complete", &err);
```

When completion is enabled, the entrypoint name is reserved for hidden completion transport. Trying to add a subcommand with that exact name fails explicitly. If your CLI really needs that visible subcommand, disable completion first or move the hidden entrypoint to another reserved name.

## Argument metadata still drives candidate generation

`ap_try_handle_completion(...)` only handles transport and delegation. Candidate generation still comes from normal argument metadata:

- `choices` or `completion_kind = AP_COMPLETION_KIND_CHOICES`
- `completion_kind = AP_COMPLETION_KIND_FILE`
- `completion_kind = AP_COMPLETION_KIND_DIRECTORY`
- `completion_kind = AP_COMPLETION_KIND_COMMAND`
- `completion_callback` + `completion_user_data`

If you need to bypass the helper, you can still call `ap_complete(...)` directly.

## Shell installation

### Bash

```bash
./build/sample/example_completion --generate-bash-completion > ./example_completion.bash
mkdir -p ~/.local/share/bash-completion/completions
cp ./example_completion.bash ~/.local/share/bash-completion/completions/example_completion
source ~/.local/share/bash-completion/completions/example_completion
```

### Fish

```bash
mkdir -p ~/.config/fish/completions
./build/sample/example_completion --generate-fish-completion   > ~/.config/fish/completions/example_completion.fish
```

These scripts call the configured hidden entrypoint automatically, so end users do not need to know about `__complete` or a custom replacement name.

## Fallback policy

Treat “no candidates” as a normal result.

1. If a callback has no dynamic data, return `0` and add nothing.
2. If prefix filtering removes every candidate, return `0` and add nothing.
3. Keep static metadata such as `choices` or `completion_kind` even when the callback yields nothing.
4. Return `-1` only when completion processing itself fails.
