#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse-c.h"

static int print_generated_text(const char *text) {
  if (!text) {
    fprintf(stderr, "failed to generate output\n");
    return 1;
  }

  printf("%s", text);
  free((void *)text);
  return 0;
}

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
        ap_completion_result_add(result, commands[i], "dynamic command", err) !=
            0) {
      return -1;
    }
  }
  return 0;
}

static int print_completion_candidates(const ap_completion_result *result) {
  int i;

  if (!result) {
    return 1;
  }
  for (i = 0; i < result->count; i++) {
    printf("%s\n", result->items[i].value);
  }
  return 0;
}

static int maybe_handle_completion_request(ap_parser *parser, int argc,
                                           char **argv) {
  ap_completion_result result;
  ap_error err = {0};
  int handled = 0;
  int rc;

  rc = ap_try_handle_completion(parser, argc, argv, "bash", &handled, &result,
                                &err);
  if (rc != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_completion_result_free(&result);
    return 1;
  }
  if (!handled) {
    return -1;
  }

  rc = print_completion_candidates(&result);
  ap_completion_result_free(&result);
  return rc;
}

static int maybe_handle_generator_flags(ap_parser *parser, ap_namespace *ns) {
  bool is_help = false;
  bool bash_completion = false;
  bool fish_completion = false;
  bool zsh_completion = false;
  bool manpage = false;

  ap_ns_get_bool(ns, "help", &is_help);
  ap_ns_get_bool(ns, "generate_bash_completion", &bash_completion);
  ap_ns_get_bool(ns, "generate_fish_completion", &fish_completion);
  ap_ns_get_bool(ns, "generate_zsh_completion", &zsh_completion);
  ap_ns_get_bool(ns, "generate_manpage", &manpage);

  if (is_help) {
    return print_generated_text(ap_format_help(parser));
  }
  if (bash_completion) {
    return print_generated_text(ap_format_bash_completion(parser));
  }
  if (fish_completion) {
    return print_generated_text(ap_format_fish_completion(parser));
  }
  if (zsh_completion) {
    return print_generated_text(ap_format_zsh_completion(parser));
  }
  if (manpage) {
    return print_generated_text(ap_format_manpage(parser));
  }

  return -1;
}

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new(
      "example_completion",
      "demo app that can generate bash/fish/zsh completions and a man page.");
  const char *mode_choices[] = {"fast", "slow"};
  const char *format_choices[] = {"json", "yaml", "toml"};
  const char *profile_choices[] = {"local", "staging", "prod"};
  static const char *const exec_candidates[] = {"git", "grep", "ls", "sed",
                                                NULL};
  static const char *const task_candidates[] = {"build", "bench", "bundle",
                                                "test", NULL};

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options bash = ap_arg_options_default();
    ap_arg_options fish = ap_arg_options_default();
    ap_arg_options zsh = ap_arg_options_default();
    ap_arg_options manpage = ap_arg_options_default();
    ap_arg_options mode = ap_arg_options_default();
    ap_arg_options input = ap_arg_options_default();
    ap_arg_options format = ap_arg_options_default();
    ap_arg_options task = ap_arg_options_default();
    ap_arg_options exec = ap_arg_options_default();
    ap_arg_options profile = ap_arg_options_default();
    ap_arg_options config_dir = ap_arg_options_default();

    bash.type = AP_TYPE_BOOL;
    bash.action = AP_ACTION_STORE_TRUE;
    bash.help = "print a bash completion script";

    fish.type = AP_TYPE_BOOL;
    fish.action = AP_ACTION_STORE_TRUE;
    fish.help = "print a fish completion script";

    zsh.type = AP_TYPE_BOOL;
    zsh.action = AP_ACTION_STORE_TRUE;
    zsh.help = "print a zsh completion script";

    manpage.type = AP_TYPE_BOOL;
    manpage.action = AP_ACTION_STORE_TRUE;
    manpage.help = "print a roff man page";

    mode.help = "execution mode";
    mode.choices.items = mode_choices;
    mode.choices.count = 2;

    input.help = "input file";
    input.completion_kind = AP_COMPLETION_KIND_FILE;
    input.completion_hint = "path to an input file";

    format.help = "output format";
    format.choices.items = format_choices;
    format.choices.count = 3;

    task.help = "task name";
    task.completion_kind = AP_COMPLETION_KIND_COMMAND;
    task.completion_hint = "dynamic task";
    task.completion_callback = exec_completion_callback;
    task.completion_user_data = (void *)task_candidates;

    exec.help = "program to launch";
    exec.completion_kind = AP_COMPLETION_KIND_COMMAND;
    exec.completion_hint = "command name";
    exec.completion_callback = exec_completion_callback;
    exec.completion_user_data = (void *)exec_candidates;

    profile.help = "deployment profile";
    profile.choices.items = profile_choices;
    profile.choices.count = 3;

    config_dir.help = "config directory";
    config_dir.completion_kind = AP_COMPLETION_KIND_DIRECTORY;


    if (ap_add_argument(parser, "--generate-bash-completion", bash, &err) != 0 ||
        ap_add_argument(parser, "--generate-fish-completion", fish, &err) != 0 ||
        ap_add_argument(parser, "--generate-zsh-completion", zsh, &err) != 0 ||
        ap_add_argument(parser, "--generate-manpage", manpage, &err) != 0 ||
        ap_add_argument(parser, "--mode", mode, &err) != 0 ||
        ap_add_argument(parser, "--profile", profile, &err) != 0 ||
        ap_add_argument(parser, "--config-dir", config_dir, &err) != 0 ||
        ap_add_argument(parser, "--exec", exec, &err) != 0 ||
        ap_add_argument(parser, "input", input, &err) != 0 ||
        ap_add_argument(parser, "format", format, &err) != 0 ||
        ap_add_argument(parser, "task", task, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  {
    int completion_rc = maybe_handle_completion_request(parser, argc, argv);
    if (completion_rc >= 0) {
      ap_parser_free(parser);
      return completion_rc;
    }
  }

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *error_text = ap_format_error(parser, &err);
    if (error_text) {
      fprintf(stderr, "%s", error_text);
      free(error_text);
    } else {
      fprintf(stderr, "error: %s\n", err.message);
    }
    ap_parser_free(parser);
    return 1;
  }

  {
    int generated = maybe_handle_generator_flags(parser, ns);
    if (generated >= 0) {
      ap_namespace_free(ns);
      ap_parser_free(parser);
      return generated;
    }
  }

  {
    const char *mode = NULL;
    const char *input = NULL;
    const char *format = NULL;
    const char *task = NULL;
    const char *exec = NULL;
    const char *profile = NULL;
    const char *config_dir = NULL;

    ap_ns_get_string(ns, "mode", &mode);
    ap_ns_get_string(ns, "input", &input);
    ap_ns_get_string(ns, "format", &format);
    ap_ns_get_string(ns, "task", &task);
    ap_ns_get_string(ns, "exec", &exec);
    ap_ns_get_string(ns, "profile", &profile);
    ap_ns_get_string(ns, "config_dir", &config_dir);

    printf("mode=%s\n", mode ? mode : "(null)");
    printf("input=%s\n", input ? input : "(null)");
    printf("format=%s\n", format ? format : "(null)");
    printf("task=%s\n", task ? task : "(null)");
    printf("exec=%s\n", exec ? exec : "(null)");
    printf("profile=%s\n", profile ? profile : "(null)");
    printf("config_dir=%s\n", config_dir ? config_dir : "(null)");
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
