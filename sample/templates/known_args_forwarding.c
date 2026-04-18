#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  char **unknown = NULL;
  int unknown_count = 0;
  int i = 0;
  const char *subcommand = NULL;
  const char *subcommand_path = NULL;
  /* CHANGE_ME_PROG_NAME: rename for your CLI binary. */
  ap_parser *parser = ap_parser_new("template_known_args", "known-args template");
  ap_parser *run = NULL;
  ap_error sub_err = {0};

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options config = ap_arg_options_default();
    /* CHANGE_ME_DEST: change destination name used by ap_ns_get_string. */
    config.dest = "config_path";
    config.help = "configuration path";
    if (ap_add_argument(parser, "--config", config, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  /* Optional: demonstrate subcommand-aware forwarding. */
  run = ap_add_subcommand(parser, "run", "execute downstream command", &sub_err);
  if (!run) {
    fprintf(stderr, "%s\n", sub_err.message);
    ap_parser_free(parser);
    return 1;
  }

  {
    ap_arg_options verbose = ap_arg_options_default();
    verbose.type = AP_TYPE_BOOL;
    verbose.action = AP_ACTION_STORE_TRUE;
    verbose.help = "wrapper-side verbose switch";
    if (ap_add_argument(parser, "--verbose", verbose, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  if (ap_parse_known_args(parser, argc, argv, &ns, &unknown, &unknown_count,
                          &err) != 0) {
    char *formatted = ap_format_error(parser, &err);
    if (formatted) {
      fprintf(stderr, "%s", formatted);
      free(formatted);
    }
    ap_parser_free(parser);
    return 1;
  }

  {
    const char *config_path = NULL;
    ap_ns_get_string(ns, "config_path", &config_path);
    printf("config=%s\n", config_path ? config_path : "(null)");
  }
  ap_ns_get_string(ns, "subcommand", &subcommand);
  ap_ns_get_string(ns, "subcommand_path", &subcommand_path);
  printf("subcommand=%s\n", subcommand ? subcommand : "(none)");
  printf("subcommand_path=%s\n", subcommand_path ? subcommand_path : "(none)");

  puts("tip: use `--` before downstream flags to prevent accidental wrapper-side parsing.");

  printf("forward unknown tokens:\n");
  for (i = 0; i < unknown_count; ++i) {
    printf("  argv[%d] = %s\n", i, unknown[i]);
  }

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
