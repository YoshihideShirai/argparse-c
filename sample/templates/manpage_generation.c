#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new("template_manpage", "manpage template");

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options generate_manpage = ap_arg_options_default();
    generate_manpage.type = AP_TYPE_BOOL;
    generate_manpage.action = AP_ACTION_STORE_TRUE;
    /* CHANGE_ME_FLAG: rename if your app uses another generator flag. */
    generate_manpage.help = "print a roff man page";
    if (ap_add_argument(parser, "--generate-manpage", generate_manpage, &err) !=
        0) {
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
    }
    ap_parser_free(parser);
    return 1;
  }

  {
    bool requested = false;
    ap_ns_get_bool(ns, "generate_manpage", &requested);
    if (requested) {
      char *manpage = ap_format_manpage(parser);
      if (!manpage) {
        fprintf(stderr, "failed to generate manpage\n");
        ap_namespace_free(ns);
        ap_parser_free(parser);
        return 1;
      }
      printf("%s", manpage);
      free(manpage);
    }
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
