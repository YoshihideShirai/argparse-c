#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse-c.h"

static ap_parser *build_parser(ap_error *err) {
  ap_parser *parser = ap_parser_new("example_test_runner",
                                    "minimal parser test runner");
  if (!parser) {
    return NULL;
  }

  ap_arg_options count_opts = ap_arg_options_default();
  count_opts.type = AP_TYPE_INT32;
  count_opts.required = true;
  count_opts.help = "count value";
  if (ap_add_argument(parser, "--count", count_opts, err) != 0) {
    ap_parser_free(parser);
    return NULL;
  }

  ap_arg_options name_opts = ap_arg_options_default();
  name_opts.required = true;
  name_opts.help = "target name";
  if (ap_add_argument(parser, "name", name_opts, err) != 0) {
    ap_parser_free(parser);
    return NULL;
  }

  return parser;
}

static int test_parse_success(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = build_parser(&err);
  if (!parser) {
    fprintf(stderr, "FAIL: setup failed in success test: %s\n", err.message);
    return 1;
  }

  char *argv[] = {"prog", "--count", "3", "alice"};
  int argc = (int)(sizeof(argv) / sizeof(argv[0]));

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *formatted = ap_format_error(parser, &err);
    if (formatted) {
      fprintf(stderr, "FAIL: success test parse error:\n%s", formatted);
      free(formatted);
    } else {
      fprintf(stderr, "FAIL: success test parse error: %s\n", err.message);
    }
    ap_parser_free(parser);
    return 1;
  }

  int32_t count = 0;
  const char *name = NULL;
  bool has_count = ap_ns_get_int32(ns, "count", &count);
  bool has_name = ap_ns_get_string(ns, "name", &name);
  if (!has_count || !has_name || count != 3 || !name || strcmp(name, "alice") != 0) {
    fprintf(stderr,
            "FAIL: success test values mismatch (has_count=%d has_name=%d count=%d name=%s)\n",
            has_count,
            has_name,
            count,
            name ? name : "(null)");
    ap_namespace_free(ns);
    ap_parser_free(parser);
    return 1;
  }

  printf("PASS: success case parsed --count=3 name=alice\n");
  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}

static int test_parse_invalid_int(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = build_parser(&err);
  if (!parser) {
    fprintf(stderr, "FAIL: setup failed in invalid-int test: %s\n", err.message);
    return 1;
  }

  char *argv[] = {"prog", "--count", "x", "alice"};
  int argc = (int)(sizeof(argv) / sizeof(argv[0]));

  if (ap_parse_args(parser, argc, argv, &ns, &err) == 0) {
    fprintf(stderr, "FAIL: invalid-int test unexpectedly succeeded\n");
    ap_namespace_free(ns);
    ap_parser_free(parser);
    return 1;
  }

  if (err.message[0] == '\0') {
    fprintf(stderr, "FAIL: invalid-int test missing ap_error message\n");
    ap_namespace_free(ns);
    ap_parser_free(parser);
    return 1;
  }

  {
    char *formatted = ap_format_error(parser, &err);
    if (!formatted) {
      fprintf(stderr, "FAIL: invalid-int test missing formatted error\n");
      ap_namespace_free(ns);
      ap_parser_free(parser);
      return 1;
    }
    printf("PASS: invalid-int case failed as expected\n");
    printf("PASS: invalid-int formatted error:\n%s", formatted);
    free(formatted);
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}

static int test_parse_missing_positional(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *parser = build_parser(&err);
  if (!parser) {
    fprintf(stderr,
            "FAIL: setup failed in missing-positional test: %s\n",
            err.message);
    return 1;
  }

  char *argv[] = {"prog", "--count", "3"};
  int argc = (int)(sizeof(argv) / sizeof(argv[0]));

  if (ap_parse_args(parser, argc, argv, &ns, &err) == 0) {
    fprintf(stderr, "FAIL: missing-positional test unexpectedly succeeded\n");
    ap_namespace_free(ns);
    ap_parser_free(parser);
    return 1;
  }

  if (err.message[0] == '\0') {
    fprintf(stderr, "FAIL: missing-positional test missing ap_error message\n");
    ap_namespace_free(ns);
    ap_parser_free(parser);
    return 1;
  }

  {
    char *formatted = ap_format_error(parser, &err);
    if (!formatted) {
      fprintf(stderr, "FAIL: missing-positional test missing formatted error\n");
      ap_namespace_free(ns);
      ap_parser_free(parser);
      return 1;
    }
    printf("PASS: missing-positional case failed as expected\n");
    printf("PASS: missing-positional formatted error:\n%s", formatted);
    free(formatted);
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}

int main(void) {
  int failed = 0;

  failed += test_parse_success();
  failed += test_parse_invalid_int();
  failed += test_parse_missing_positional();

  if (failed != 0) {
    fprintf(stderr, "FAIL: %d test(s) failed\n", failed);
    return 1;
  }

  printf("PASS: all tests passed\n");
  return 0;
}
