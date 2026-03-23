# Completion callbacks

This guide covers the **application-side completion entrypoint** used together with `ap_complete(...)` and `ap_completion_callback`.

If you only need to generate shell scripts, start with the generator APIs in `README.md`. If you want runtime-aware completions, such as filtering candidates from application state or a remote source, add a `__complete` entrypoint and completion callbacks. The reference implementation is `sample/example_completion.c`.

## Design overview

The public completion APIs live in `include/argparse-c.h`:

- `ap_complete(...)` runs the library-side completion resolver.
- `ap_completion_callback` lets one argument append dynamic candidates.
- `ap_completion_request` tells your callback which token is being completed.
- `ap_completion_result_init(...)`, `ap_completion_result_add(...)`, and `ap_completion_result_free(...)` manage the candidate list.
- `ap_arg_options.completion_kind`, `ap_arg_options.completion_hint`, `ap_arg_options.completion_callback`, and `ap_arg_options.completion_user_data` let each argument opt into completion behavior.

A practical design is:

1. Keep your normal CLI behavior unchanged.
2. Reserve `argv[1] == "__complete"` as an internal entrypoint.
3. Strip wrapper-only flags such as `--shell bash` before calling `ap_complete(...)`.
4. Print one candidate per line to stdout.
5. Exit with a non-zero code only when completion handling itself fails.

That layout matches `sample/example_completion.c`, so the sample can be used as a copy-paste baseline.

## Minimal parser setup

The smallest useful setup combines static metadata with one callback-backed argument.

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

This example deliberately mirrors the generator-oriented sample in `sample/example_completion.c`, so readers can compare the guide and a full program side by side.

## `__complete` entrypoint design

The library does not force a transport protocol between your shell script and your executable. The recommended convention is a private subcommand-like entrypoint:

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

Why this structure works well:

- `__complete` is easy to detect before `ap_parse_args(...)` runs.
- `--shell` stays outside your public CLI contract.
- `--` cleanly separates wrapper metadata from the original command line.
- The same binary can serve normal execution, generator flags, and completion requests.

## How shell integration calls the entrypoint

The generated bash/fish scripts produced by `ap_format_bash_completion(...)` and `ap_format_fish_completion(...)` are the stable integration point you should expose publicly. Those scripts can invoke your binary’s `__complete` entrypoint and pass through the current command line.

A simple manual invocation looks like this:

```bash
./build/sample/example_completion __complete --shell bash -- --mode f
./build/sample/example_completion __complete --shell bash -- --exec g
./build/sample/example_completion __complete --shell fish -- input.tx
```

In that contract:

- `__complete` selects completion mode.
- `--shell bash` or `--shell fish` tells `ap_complete(...)` which shell behavior to emulate.
- `--` marks the beginning of the original command line tokens.
- The remaining argv slice is passed to `ap_complete(...)` unchanged.

For end users, the usual workflow is still:

```bash
./build/sample/example_completion --generate-bash-completion > example_completion.bash
source ./example_completion.bash
```

Then the shell script performs the hidden `__complete` call for them.

## Fallback policy when no candidates can be returned

Treat “no completions available” as a normal outcome, not an error.

Recommended fallback rules:

1. **If your callback has no dynamic data**, return `0` and add nothing.
2. **If prefix filtering removes every candidate**, return `0` and add nothing.
3. **If the argument already has `choices` or `completion_kind` metadata**, keep those declarations even when the callback produces nothing.
4. **Only return `-1` when completion processing itself failed**, such as an allocation failure reported through `ap_completion_result_add(...)`.

This policy gives shells a clean fallback path:

- static `choices` can still appear when available;
- built-in kinds such as `AP_COMPLETION_KIND_FILE` or `AP_COMPLETION_KIND_COMMAND` can still hint at file/command completion;
- an empty stdout result simply means “no extra candidates from the app right now.”

In practice, that means your callback should prefer this pattern:

```c
if (!commands) {
  return 0;
}
```

instead of manufacturing placeholder values.

## Relationship to the sample

Use `sample/example_completion.c` as the canonical end-to-end example:

- generator flags: `--generate-bash-completion`, `--generate-fish-completion`, `--generate-manpage`
- hidden completion entrypoint: `__complete`
- static completion metadata: `choices`, `AP_COMPLETION_KIND_FILE`, `AP_COMPLETION_KIND_COMMAND`
- dynamic completion source: `exec_completion_callback(...)`

If you want the smallest adoption path, start by copying the sample’s `maybe_handle_completion_request(...)` function, then replace the static `exec_candidates` array with your own application data source.
