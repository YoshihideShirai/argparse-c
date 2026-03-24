#include "argparse-c.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AP_FUZZ_MAX_TOKENS 64
#define AP_FUZZ_MAX_TOKEN_LEN 64

typedef struct {
  char **argv;
  int argc;
} ap_fuzz_argv;

static void ap_fuzz_free_argv(ap_fuzz_argv *parsed) {
  if (parsed == NULL || parsed->argv == NULL) {
    return;
  }

  for (int i = 0; i < parsed->argc; ++i) {
    free(parsed->argv[i]);
  }

  free(parsed->argv);
  parsed->argv = NULL;
  parsed->argc = 0;
}

static int ap_fuzz_push_token(ap_fuzz_argv *parsed, const char *start,
                              size_t len) {
  if (parsed->argc >= AP_FUZZ_MAX_TOKENS + 1) {
    return 0;
  }

  if (len == 0) {
    return 1;
  }

  if (len > AP_FUZZ_MAX_TOKEN_LEN) {
    len = AP_FUZZ_MAX_TOKEN_LEN;
  }

  char *token = (char *)malloc(len + 1);
  if (token == NULL) {
    return 0;
  }

  memcpy(token, start, len);
  token[len] = '\0';

  parsed->argv[parsed->argc++] = token;
  return 1;
}

static int ap_fuzz_split_tokens(const uint8_t *data, size_t size,
                                ap_fuzz_argv *out) {
  out->argc = 0;
  out->argv = (char **)calloc(AP_FUZZ_MAX_TOKENS + 1, sizeof(char *));
  if (out->argv == NULL) {
    return 0;
  }

  const char *prog = "argparse-fuzz";
  if (!ap_fuzz_push_token(out, prog, strlen(prog))) {
    ap_fuzz_free_argv(out);
    return 0;
  }

  size_t token_start = 0;
  int in_token = 0;

  for (size_t i = 0; i < size; ++i) {
    int delimiter = (data[i] == 0) || isspace((unsigned char)data[i]);

    if (delimiter) {
      if (in_token) {
        if (!ap_fuzz_push_token(out, (const char *)&data[token_start],
                                i - token_start)) {
          ap_fuzz_free_argv(out);
          return 0;
        }
      }
      in_token = 0;
      continue;
    }

    if (!in_token) {
      token_start = i;
      in_token = 1;
    }
  }

  if (in_token) {
    if (!ap_fuzz_push_token(out, (const char *)&data[token_start],
                            size - token_start)) {
      ap_fuzz_free_argv(out);
      return 0;
    }
  }

  return 1;
}

static ap_parser *ap_fuzz_make_parser(int variant, ap_error *err) {
  ap_arg_options options = ap_arg_options_default();
  ap_parser *parser = ap_parser_new("fuzz", "fuzz parser");
  if (parser == NULL) {
    return NULL;
  }

  switch (variant % 4) {
  case 0:
    options.type = AP_TYPE_STRING;
    options.help = "name option";
    (void)ap_add_argument(parser, "--name", options, err);

    options = ap_arg_options_default();
    options.action = AP_ACTION_COUNT;
    options.help = "verbosity";
    (void)ap_add_argument(parser, "-v", options, err);

    options = ap_arg_options_default();
    options.type = AP_TYPE_STRING;
    options.help = "input file";
    (void)ap_add_argument(parser, "input", options, err);
    break;

  case 1: {
    ap_parser *run_cmd = ap_add_subcommand(parser, "run", "run command", err);
    if (run_cmd != NULL) {
      options = ap_arg_options_default();
      options.action = AP_ACTION_STORE_TRUE;
      options.help = "force";
      (void)ap_add_argument(run_cmd, "--force", options, err);

      options = ap_arg_options_default();
      options.type = AP_TYPE_STRING;
      options.help = "target";
      (void)ap_add_argument(run_cmd, "target", options, err);
    }

    (void)ap_add_subcommand(parser, "status", "status command", err);
    break;
  }

  case 2: {
    ap_mutually_exclusive_group *group =
        ap_add_mutually_exclusive_group(parser, false, err);

    if (group != NULL) {
      options = ap_arg_options_default();
      options.action = AP_ACTION_STORE_TRUE;
      options.help = "json output";
      (void)ap_group_add_argument(group, "--json", options, err);

      options = ap_arg_options_default();
      options.action = AP_ACTION_STORE_TRUE;
      options.help = "yaml output";
      (void)ap_group_add_argument(group, "--yaml", options, err);
    }

    options = ap_arg_options_default();
    options.type = AP_TYPE_STRING;
    options.nargs = AP_NARGS_ZERO_OR_MORE;
    options.help = "paths";
    (void)ap_add_argument(parser, "paths", options, err);
    break;
  }

  default:
    options = ap_arg_options_default();
    options.type = AP_TYPE_INT64;
    options.help = "count";
    (void)ap_add_argument(parser, "--count", options, err);

    options = ap_arg_options_default();
    options.required = true;
    options.help = "mode";
    (void)ap_add_argument(parser, "--mode", options, err);

    options = ap_arg_options_default();
    options.type = AP_TYPE_STRING;
    options.nargs = AP_NARGS_OPTIONAL;
    options.help = "source";
    (void)ap_add_argument(parser, "source", options, err);
    break;
  }

  return parser;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (data == NULL || size == 0) {
    return 0;
  }

  ap_error err = {0};
  ap_parser *parser = ap_fuzz_make_parser((int)data[0], &err);
  if (parser == NULL) {
    return 0;
  }

  ap_fuzz_argv parsed = {0};
  if (!ap_fuzz_split_tokens(data + 1, size - 1, &parsed)) {
    ap_parser_free(parser);
    return 0;
  }

  switch (data[0] % 4) {
  case 0: {
    ap_namespace *ns = NULL;
    (void)ap_parse_args(parser, parsed.argc, parsed.argv, &ns, &err);
    ap_namespace_free(ns);
    break;
  }

  case 1: {
    ap_namespace *ns = NULL;
    char **unknown = NULL;
    int unknown_count = 0;

    (void)ap_parse_known_args(parser, parsed.argc, parsed.argv, &ns, &unknown,
                              &unknown_count, &err);
    ap_namespace_free(ns);
    ap_free_tokens(unknown, unknown_count);
    break;
  }

  case 2: {
    char *help = ap_format_help(parser);
    free(help);
    break;
  }

  default: {
    char *manpage = ap_format_manpage(parser);
    free(manpage);
    break;
  }
  }

  ap_fuzz_free_argv(&parsed);
  ap_parser_free(parser);
  return 0;
}
