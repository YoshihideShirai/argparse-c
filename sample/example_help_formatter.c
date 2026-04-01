#include <stdio.h>
#include <stdlib.h>

#include "argparse-c.h"

static int print_text(const char *label, char *text) {
  if (!text) {
    fprintf(stderr, "failed to format help for %s\n", label);
    return 1;
  }

  printf("=== %s ===\n", label);
  printf("%s\n", text);
  free(text);
  return 0;
}

int main(void) {
  ap_error err = {0};
  ap_parser_options options = ap_parser_options_default();
  ap_parser *parser = NULL;
  ap_arg_options mode = ap_arg_options_default();
  char *help = NULL;

  options.help_formatter_mode = AP_HELP_FORMATTER_STANDARD;
  parser = ap_parser_new_with_options("example_help_formatter",
                                       "switchable help formatter demo",
                                       options);
  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  mode.help = "execution mode";
  mode.default_value = "fast";
  if (ap_add_argument(parser, "--mode", mode, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  help = ap_format_help(parser);
  if (print_text("standard", help) != 0) {
    ap_parser_free(parser);
    return 1;
  }

  ap_parser_free(parser);
  options.help_formatter_mode = AP_HELP_FORMATTER_SHOW_DEFAULTS;
  parser = ap_parser_new_with_options("example_help_formatter",
                                       "switchable help formatter demo",
                                       options);
  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }
  if (ap_add_argument(parser, "--mode", mode, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  help = ap_format_help(parser);
  if (print_text("show_defaults", help) != 0) {
    ap_parser_free(parser);
    return 1;
  }

  ap_parser_free(parser);
  options.help_formatter_mode = AP_HELP_FORMATTER_RAW_TEXT;
  parser = ap_parser_new_with_options("example_help_formatter",
                                       "switchable help formatter demo",
                                       options);
  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }
  if (ap_add_argument(parser, "--mode", mode, &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_parser_free(parser);
    return 1;
  }

  help = ap_format_help(parser);
  if (print_text("raw_text", help) != 0) {
    ap_parser_free(parser);
    return 1;
  }

  ap_parser_free(parser);
  return 0;
}
