#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

int main(int argc, char **argv) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = ap_parser_new("example1", "example1 command.");
  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  ap_arg_options text_opts = ap_arg_options_default();
  text_opts.required = true;
  text_opts.help = "text field.";
  if (ap_add_argument(parser, "-t, --text", text_opts, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  ap_arg_options integer_opts = ap_arg_options_default();
  integer_opts.type = AP_TYPE_INT32;
  integer_opts.help = "integer field.";
  if (ap_add_argument(parser, "-i, --integer", integer_opts, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  ap_arg_options arg1_opts = ap_arg_options_default();
  arg1_opts.required = true;
  arg1_opts.help = "arg1 field.";
  if (ap_add_argument(parser, "arg1", arg1_opts, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  ap_arg_options arg2_opts = ap_arg_options_default();
  arg2_opts.nargs = AP_NARGS_OPTIONAL;
  arg2_opts.help = "arg2 field.";
  if (ap_add_argument(parser, "arg2", arg2_opts, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    fprintf(stderr, "error: %s\n", err.message);
    char *usage = ap_format_usage(parser);
    if (usage) {
      fprintf(stderr, "%s", usage);
      free(usage);
    }
    ap_parser_free(parser);
    return 1;
  }

  {
    bool is_help = false;
    if (ap_ns_get_bool(ns, "help", &is_help) && is_help) {
      char *help = ap_format_help(parser);
      if (help) {
        printf("%s", help);
        free(help);
      }
      ap_namespace_free(ns);
      ap_parser_free(parser);
      return 0;
    }
  }

  {
    const char *text = NULL;
    const char *arg1 = NULL;
    const char *arg2 = NULL;
    int32_t integer = 0;

    ap_ns_get_string(ns, "text", &text);
    ap_ns_get_int32(ns, "integer", &integer);
    ap_ns_get_string(ns, "arg1", &arg1);
    ap_ns_get_string(ns, "arg2", &arg2);

    printf("text=%s\n", text ? text : "(null)");
    printf("integer=%d\n", integer);
    printf("arg1=%s\n", arg1 ? arg1 : "(null)");
    printf("arg2=%s\n", arg2 ? arg2 : "(null)");
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
