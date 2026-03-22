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

static bool has_flag(int argc, char **argv, const char *flag) {
  int i = 0;

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      return true;
    }
  }

  return false;
}

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new(
      "example_manpage",
      "demo app with subcommands and generator flags for documentation.");
  ap_parser *config = NULL;
  ap_parser *set = NULL;

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options bash = ap_arg_options_default();
    ap_arg_options fish = ap_arg_options_default();
    ap_arg_options manpage = ap_arg_options_default();

    bash.type = AP_TYPE_BOOL;
    bash.action = AP_ACTION_STORE_TRUE;
    bash.help = "print a bash completion script";

    fish.type = AP_TYPE_BOOL;
    fish.action = AP_ACTION_STORE_TRUE;
    fish.help = "print a fish completion script";

    manpage.type = AP_TYPE_BOOL;
    manpage.action = AP_ACTION_STORE_TRUE;
    manpage.help = "print a roff man page";

    if (ap_add_argument(parser, "--generate-bash-completion", bash, &err) != 0 ||
        ap_add_argument(parser, "--generate-fish-completion", fish, &err) != 0 ||
        ap_add_argument(parser, "--generate-manpage", manpage, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  config = ap_add_subcommand(parser, "config", "configuration commands", &err);
  if (!config) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  set = ap_add_subcommand(config, "set", "set a configuration value", &err);
  if (!set) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  {
    ap_arg_options section = ap_arg_options_default();
    ap_arg_options value = ap_arg_options_default();
    const char *section_choices[] = {"core", "ui", "network"};

    section.help = "configuration section";
    section.choices.items = section_choices;
    section.choices.count = 3;

    value.required = true;
    value.help = "value to store";

    if (ap_add_argument(set, "--section", section, &err) != 0 ||
        ap_add_argument(set, "key", ap_arg_options_default(), &err) != 0 ||
        ap_add_argument(set, "value", value, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  if (has_flag(argc, argv, "--generate-bash-completion")) {
    int rc = print_generated_text(ap_format_bash_completion(parser));
    ap_parser_free(parser);
    return rc;
  }

  if (has_flag(argc, argv, "--generate-fish-completion")) {
    int rc = print_generated_text(ap_format_fish_completion(parser));
    ap_parser_free(parser);
    return rc;
  }

  if (has_flag(argc, argv, "--generate-manpage")) {
    int rc = print_generated_text(ap_format_manpage(parser));
    ap_parser_free(parser);
    return rc;
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
    const char *subcommand = NULL;
    const char *subcommand_path = NULL;
    const char *section = NULL;
    const char *key = NULL;
    const char *value = NULL;

    ap_ns_get_string(ns, "subcommand", &subcommand);
    ap_ns_get_string(ns, "subcommand_path", &subcommand_path);
    ap_ns_get_string(ns, "section", &section);
    ap_ns_get_string(ns, "key", &key);
    ap_ns_get_string(ns, "value", &value);

    printf("subcommand=%s\n", subcommand ? subcommand : "(null)");
    printf("subcommand_path=%s\n", subcommand_path ? subcommand_path : "(null)");
    printf("section=%s\n", section ? section : "(null)");
    printf("key=%s\n", key ? key : "(null)");
    printf("value=%s\n", value ? value : "(null)");
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
