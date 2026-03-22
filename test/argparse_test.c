#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse-c.h"

static int failures = 0;

#define ASSERT_TRUE(cond)                                                       \
  do {                                                                          \
    if (!(cond)) {                                                              \
      fprintf(stderr, "ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, \
              #cond);                                                           \
      failures++;                                                               \
      return;                                                                   \
    }                                                                           \
  } while (0)

#define ASSERT_EQ_INT(expected, actual)                                         \
  do {                                                                          \
    if ((expected) != (actual)) {                                               \
      fprintf(stderr,                                                           \
              "ASSERT_EQ_INT failed at %s:%d: expected=%d actual=%d\n",       \
              __FILE__, __LINE__, (expected), (actual));                       \
      failures++;                                                               \
      return;                                                                   \
    }                                                                           \
  } while (0)

#define ASSERT_EQ_STR(expected, actual)                                         \
  do {                                                                          \
    if (strcmp((expected), (actual)) != 0) {                                   \
      fprintf(stderr,                                                           \
              "ASSERT_EQ_STR failed at %s:%d: expected=%s actual=%s\n",       \
              __FILE__, __LINE__, (expected), (actual));                       \
      failures++;                                                               \
      return;                                                                   \
    }                                                                           \
  } while (0)

static ap_parser *new_base_parser(void) {
  ap_error err = {0};
  ap_parser *p = ap_parser_new("prog", "desc");

  ap_arg_options text_opts = ap_arg_options_default();
  text_opts.required = true;
  text_opts.help = "text";
  if (ap_add_argument(p, "-t, --text", text_opts, &err) != 0) {
    return NULL;
  }

  ap_arg_options int_opts = ap_arg_options_default();
  int_opts.type = AP_TYPE_INT32;
  if (ap_add_argument(p, "-n, --num", int_opts, &err) != 0) {
    return NULL;
  }

  ap_arg_options pos_opts = ap_arg_options_default();
  pos_opts.help = "input";
  if (ap_add_argument(p, "input", pos_opts, &err) != 0) {
    return NULL;
  }

  return p;
}

static void test_success_parse(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {"prog", "-t", "hello", "-n", "12", "file.txt", NULL};
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;

  ASSERT_TRUE(p != NULL);
  ASSERT_EQ_INT(0, ap_parse_args(p, 6, argv, &ns, &err));
  ASSERT_TRUE(ap_ns_get_string(ns, "text", &text));
  ASSERT_TRUE(ap_ns_get_int32(ns, "num", &num));
  ASSERT_TRUE(ap_ns_get_string(ns, "input", &input));
  ASSERT_EQ_STR("hello", text);
  ASSERT_EQ_INT(12, (int)num);
  ASSERT_EQ_STR("file.txt", input);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

static void test_unknown_option(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {"prog", "--bogus", NULL};

  ASSERT_TRUE(p != NULL);
  ASSERT_EQ_INT(-1, ap_parse_args(p, 2, argv, &ns, &err));
  ASSERT_EQ_INT(AP_ERR_UNKNOWN_OPTION, err.code);

  ap_parser_free(p);
}

static void test_invalid_int(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {"prog", "-t", "x", "-n", "abc", "file", NULL};

  ASSERT_TRUE(p != NULL);
  ASSERT_EQ_INT(-1, ap_parse_args(p, 6, argv, &ns, &err));
  ASSERT_EQ_INT(AP_ERR_INVALID_INT32, err.code);

  ap_parser_free(p);
}

static void test_choices(void) {
  static const char *choices[] = {"fast", "slow"};
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  char *argv_bad[] = {"prog", "--mode", "medium", NULL};

  ASSERT_TRUE(p != NULL);
  mode.help = "mode";
  mode.choices.items = choices;
  mode.choices.count = 2;
  ASSERT_EQ_INT(0, ap_add_argument(p, "--mode", mode, &err));

  ASSERT_EQ_INT(-1, ap_parse_args(p, 3, argv_bad, &ns, &err));
  ASSERT_EQ_INT(AP_ERR_INVALID_CHOICE, err.code);

  ap_parser_free(p);
}

static void test_nargs_variants(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options files = ap_arg_options_default();
  char *argv[] = {"prog", "--files", "a.txt", "b.txt", NULL};

  ASSERT_TRUE(p != NULL);
  files.nargs = AP_NARGS_ONE_OR_MORE;
  ASSERT_EQ_INT(0, ap_add_argument(p, "--files", files, &err));

  ASSERT_EQ_INT(0, ap_parse_args(p, 4, argv, &ns, &err));
  ASSERT_EQ_INT(2, ap_ns_get_count(ns, "files"));
  ASSERT_EQ_STR("a.txt", ap_ns_get_string_at(ns, "files", 0));
  ASSERT_EQ_STR("b.txt", ap_ns_get_string_at(ns, "files", 1));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

static void test_default_value(void) {
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options name = ap_arg_options_default();
  char *argv[] = {"prog", NULL};
  const char *actual = NULL;

  ASSERT_TRUE(p != NULL);
  name.default_value = "guest";
  ASSERT_EQ_INT(0, ap_add_argument(p, "--name", name, &err));

  ASSERT_EQ_INT(0, ap_parse_args(p, 1, argv, &ns, &err));
  ASSERT_TRUE(ap_ns_get_string(ns, "name", &actual));
  ASSERT_EQ_STR("guest", actual);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

static void test_help_generation(void) {
  ap_error err = {0};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options arg = ap_arg_options_default();
  char *help;

  ASSERT_TRUE(p != NULL);
  arg.help = "value";
  ASSERT_EQ_INT(0, ap_add_argument(p, "--value", arg, &err));
  help = ap_format_help(p);
  ASSERT_TRUE(help != NULL);
  ASSERT_TRUE(strstr(help, "usage: prog") != NULL);
  ASSERT_TRUE(strstr(help, "--value") != NULL);

  free(help);
  ap_parser_free(p);
}

int main(void) {
  test_success_parse();
  test_unknown_option();
  test_invalid_int();
  test_choices();
  test_nargs_variants();
  test_default_value();
  test_help_generation();

  if (failures > 0) {
    fprintf(stderr, "tests failed: %d\n", failures);
    return 1;
  }
  printf("all tests passed\n");
  return 0;
}
