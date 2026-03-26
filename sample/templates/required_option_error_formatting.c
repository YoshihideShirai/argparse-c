#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  /* CHANGE_ME_PROG_NAME: rename for your CLI binary. */
  ap_parser *parser =
      ap_parser_new("template_required_option", "required option template");

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options required = ap_arg_options_default();
    required.required = true;
    /* CHANGE_ME_OPTION_HELP: describe what this required option means. */
    required.help = "input text";

    if (ap_add_argument(parser, "-t, --text", required, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *formatted = ap_format_error(parser, &err);
    if (formatted) {
      fprintf(stderr, "%s", formatted);
      free(formatted);
    } else {
      fprintf(stderr, "error: %s\n", err.message);
    }
    ap_parser_free(parser);
    return 1;
  }

  {
    const char *text = NULL;
    ap_ns_get_string(ns, "text", &text);
    printf("text=%s\n", text ? text : "(null)");
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
