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
  const char *shell = "bash";
  int arg_index = 2;

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

  print_completion_candidates(&result);
  ap_completion_result_free(&result);
  return 0;
}

static int maybe_handle_generator_flags(ap_parser *parser, ap_namespace *ns) {
  bool is_help = false;
  bool bash_completion = false;
  bool fish_completion = false;
  bool manpage = false;

  ap_ns_get_bool(ns, "help", &is_help);
  ap_ns_get_bool(ns, "generate_bash_completion", &bash_completion);
  ap_ns_get_bool(ns, "generate_fish_completion", &fish_completion);
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
      "demo app that can generate bash/fish completions and a man page.");
  const char *mode_choices[] = {"fast", "slow"};
  static const char *const exec_candidates[] = {"git", "grep", "ls", "sed",
                                                NULL};

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options bash = ap_arg_options_default();
    ap_arg_options fish = ap_arg_options_default();
    ap_arg_options manpage = ap_arg_options_default();
    ap_arg_options mode = ap_arg_options_default();
    ap_arg_options input = ap_arg_options_default();
    ap_arg_options exec = ap_arg_options_default();

    bash.type = AP_TYPE_BOOL;
    bash.action = AP_ACTION_STORE_TRUE;
    bash.help = "print a bash completion script";

    fish.type = AP_TYPE_BOOL;
    fish.action = AP_ACTION_STORE_TRUE;
    fish.help = "print a fish completion script";

    manpage.type = AP_TYPE_BOOL;
    manpage.action = AP_ACTION_STORE_TRUE;
    manpage.help = "print a roff man page";

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

    if (ap_add_argument(parser, "--generate-bash-completion", bash, &err) != 0 ||
        ap_add_argument(parser, "--generate-fish-completion", fish, &err) != 0 ||
        ap_add_argument(parser, "--generate-manpage", manpage, &err) != 0 ||
        ap_add_argument(parser, "--mode", mode, &err) != 0 ||
        ap_add_argument(parser, "--exec", exec, &err) != 0 ||
        ap_add_argument(parser, "input", input, &err) != 0) {
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
    const char *exec = NULL;

    ap_ns_get_string(ns, "mode", &mode);
    ap_ns_get_string(ns, "input", &input);
    ap_ns_get_string(ns, "exec", &exec);

    printf("mode=%s\n", mode ? mode : "(null)");
    printf("input=%s\n", input ? input : "(null)");
    printf("exec=%s\n", exec ? exec : "(null)");
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
