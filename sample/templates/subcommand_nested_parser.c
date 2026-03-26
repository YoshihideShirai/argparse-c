#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  /* CHANGE_ME_PROG_NAME: rename for your CLI binary. */
  ap_parser *root = ap_parser_new("template_subcommands", "subcommand template");
  ap_parser *parent = NULL;
  ap_parser *child = NULL;

  if (!root) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  /* CHANGE_ME_PARENT_COMMAND / CHANGE_ME_CHILD_COMMAND */
  parent = ap_add_subcommand(root, "config", "top-level command", &err);
  child = ap_add_subcommand(parent, "set", "nested command", &err);
  if (!parent || !child) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(root);
    return 1;
  }

  if (ap_add_argument(child, "key", ap_arg_options_default(), &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(root);
    return 1;
  }

  if (ap_parse_args(root, argc, argv, &ns, &err) != 0) {
    char *formatted = ap_format_error(root, &err);
    if (formatted) {
      fprintf(stderr, "%s", formatted);
      free(formatted);
    }
    ap_parser_free(root);
    return 1;
  }

  {
    const char *subcommand_path = NULL;
    ap_ns_get_string(ns, "subcommand_path", &subcommand_path);
    printf("subcommand_path=%s\n", subcommand_path ? subcommand_path : "(null)");
  }

  ap_namespace_free(ns);
  ap_parser_free(root);
  return 0;
}
