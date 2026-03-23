# Completion callbacks

This guide explains how to enable runtime-aware shell completion with the public APIs declared in `include/argparse-c.h`: `ap_complete(...)`, `ap_completion_callback`, `ap_completion_request`, `ap_completion_result`, `ap_completion_result_init(...)`, `ap_completion_result_add(...)`, `ap_completion_result_free(...)`, and the completion-related fields on `ap_arg_options`. The end-to-end reference implementation is `sample/example_completion.c`.

If you only need static shell scripts, start with `ap_format_bash_completion(...)` or `ap_format_fish_completion(...)`. If you want completions that depend on application state, add a hidden `__complete` entrypoint and wire one or more arguments with `ap_arg_options.completion_callback`.

## Which APIs enable completion?

When users ask “which API turns completion on?”, the answer is usually this sequence:

1. Create your parser with `ap_parser_new(...)`.
2. Configure each completable argument through `ap_arg_options`.
   - `completion_kind` advertises built-in behavior such as `AP_COMPLETION_KIND_FILE` or `AP_COMPLETION_KIND_COMMAND`.
   - `completion_hint` provides a short human-readable hint.
   - `completion_callback` registers a dynamic candidate producer.
   - `completion_user_data` passes application-owned state into that callback.
   - `choices` remains useful for static enumerations.
3. Register the argument with `ap_add_argument(...)`.
4. Add an internal `__complete` entrypoint in `main(...)`.
5. Call `ap_completion_result_init(...)`, then `ap_complete(...)`, print `ap_completion_result.items[i].value`, and finally call `ap_completion_result_free(...)`.
6. Expose shell integration with `ap_format_bash_completion(...)` or `ap_format_fish_completion(...)`.

Those names match the public header exactly, so users can jump directly between this guide and `include/argparse-c.h`.

## Minimal implementation based on `sample/example_completion.c`

The sample uses one static `choices` argument, one file-completing positional, and one callback-backed option. That is also a good minimal template for application code.

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

This mirrors `sample/example_completion.c`, so the guide and the sample can be read side by side.

## `__complete` entrypoint design

The library does not prescribe a transport protocol between generated shell code and your executable. A practical application-side convention is a private entrypoint such as `argv[1] == "__complete"`.

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

Why this layout works well:

- `__complete` is easy to detect before `ap_parse_args(...)` runs.
- Wrapper-only flags such as `--shell` remain outside your public CLI contract.
- `--` cleanly separates wrapper metadata from the original command line.
- The same binary can handle normal execution, generated completion scripts, and live completion requests.
- The implementation maps directly to `sample/example_completion.c`.

## How shell integration reaches `__complete`

The public integration surface for end users is still the generated shell script:

- `ap_format_bash_completion(...)`
- `ap_format_fish_completion(...)`

Those APIs return shell code that can call your binary’s hidden `__complete` entrypoint and forward the current command line. In other words, users install a generated script, but your executable still owns candidate generation through `ap_complete(...)`.

Manual smoke tests look like this:

```bash
./build/sample/example_completion __complete --shell bash -- --mode f
./build/sample/example_completion __complete --shell bash -- --exec g
./build/sample/example_completion __complete --shell fish -- input.tx
```

In that contract:

- `__complete` selects completion mode.
- `--shell bash` or `--shell fish` tells `ap_complete(...)` which shell behavior to emulate.
- `--` marks the start of the original argv slice.
- The remaining tokens are passed to `ap_complete(...)` unchanged.

For actual installation, keep the user-facing path simple:

```bash
./build/sample/example_completion --generate-bash-completion > example_completion.bash
source ./example_completion.bash
```

The generated script hides the `__complete` transport details from end users.

## Fallback policy when candidate generation fails or yields nothing

Treat “no candidates” as a normal result, and reserve hard failures for broken completion processing.

Recommended fallback policy:

1. If your callback has no dynamic data, return `0` and add nothing.
2. If prefix filtering removes every candidate, return `0` and add nothing.
3. If `choices` or `completion_kind` is already declared on the argument, keep that metadata in place even when the callback contributes nothing.
4. Return `-1` only when completion processing itself failed, for example when `ap_completion_result_add(...)` reports an allocation problem.
5. Let empty stdout mean “the application has no extra candidates right now,” so the shell or static metadata can fall back naturally.

That usually means this is the right shape:

```c
if (!commands) {
  return 0;
}
```

instead of fabricating placeholder values.

## Relationship to the sample

Use `sample/example_completion.c` as the canonical end-to-end example of the full completion flow:

- parser setup with `ap_arg_options.completion_kind`, `ap_arg_options.completion_hint`, `ap_arg_options.completion_callback`, and `ap_arg_options.completion_user_data`
- hidden `__complete` entrypoint via `maybe_handle_completion_request(...)`
- library-side resolution through `ap_complete(...)`
- candidate list management through `ap_completion_result_init(...)`, `ap_completion_result_add(...)`, and `ap_completion_result_free(...)`
- generated shell integration through `--generate-bash-completion` and `--generate-fish-completion`

If you want the shortest adoption path, copy the sample’s `maybe_handle_completion_request(...)`, keep the same `ap_complete(...)` call pattern, and replace `exec_candidates` with your own data source.
