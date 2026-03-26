#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  char **unknown = NULL;
  int unknown_count = 0;
  int i = 0;
  /* CHANGE_ME_PROG_NAME: rename for your CLI binary. */
  ap_parser *parser = ap_parser_new("template_known_args", "known-args template");

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

  printf("forward unknown tokens:\n");
  for (i = 0; i < unknown_count; ++i) {
    printf("  argv[%d] = %s\n", i, unknown[i]);
  }

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
