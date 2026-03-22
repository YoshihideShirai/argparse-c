#include "test_framework.h"

TEST(ParseKnownArgsCollectsUnknownOptions) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  char *argv[] = {
      (char *)"prog",     (char *)"--bogus", (char *)"-t", (char *)"hello",
      (char *)"file.txt", (char *)"--x=1",   NULL};
  const char *text = NULL;
  const char *input = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count, &err));
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);
  LONGS_EQUAL(2, unknown_count);
  STRCMP_EQUAL("--bogus", unknown[0]);
  STRCMP_EQUAL("--x=1", unknown[1]);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsStillValidatesRequired) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  char *argv[] = {(char *)"prog", (char *)"--bogus", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      -1, ap_parse_known_args(p, 3, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("-t", err.argument);
  STRCMP_EQUAL("option '-t' is required", err.message);
  CHECK(unknown == NULL);
  LONGS_EQUAL(0, unknown_count);

  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsUnknownOptionValueToken) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  char *argv[] = {(char *)"prog",
                  (char *)"--unknown",
                  (char *)"uval",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"file.txt",
                  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(2, unknown_count);
  STRCMP_EQUAL("--unknown", unknown[0]);
  STRCMP_EQUAL("uval", unknown[1]);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsExtraPositionals) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  char *argv[] = {(char *)"prog",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"file.txt",
                  (char *)"extra1",
                  (char *)"extra2",
                  NULL};
  const char *input = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count, &err));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("file.txt", input);
  LONGS_EQUAL(2, unknown_count);
  STRCMP_EQUAL("extra1", unknown[0]);
  STRCMP_EQUAL("extra2", unknown[1]);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsAfterDoubleDash) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  char *argv[] = {(char *)"prog",     (char *)"-t", (char *)"hello",
                  (char *)"file.txt", (char *)"--", (char *)"--passthrough",
                  (char *)"value",    NULL};
  const char *input = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 7, argv, &ns, &unknown, &unknown_count, &err));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("file.txt", input);
  LONGS_EQUAL(2, unknown_count);
  STRCMP_EQUAL("--passthrough", unknown[0]);
  STRCMP_EQUAL("value", unknown[1]);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(OptionalNargsDoesNotConsumeFollowingKnownOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  bool is_verbose = false;
  char *argv[] = {(char *)"prog", (char *)"--maybe", (char *)"--verbose", NULL};

  CHECK(p != NULL);
  maybe.nargs = AP_NARGS_OPTIONAL;
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  LONGS_EQUAL(0, ap_ns_get_count(ns, "maybe"));
  CHECK(ap_ns_get_bool(ns, "verbose", &is_verbose));
  CHECK_TRUE(is_verbose);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(PositionalZeroOrMoreLeavesTokensForRequiredPositional) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options files = ap_arg_options_default();
  const char *target = NULL;
  char *argv[] = {(char *)"prog", (char *)"a.txt", (char *)"b.txt",
                  (char *)"dest.out", NULL};

  CHECK(p != NULL);
  files.nargs = AP_NARGS_ZERO_OR_MORE;
  files.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "files", files, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 4, argv, &ns, &err));
  LONGS_EQUAL(2, ap_ns_get_count(ns, "files"));
  STRCMP_EQUAL("a.txt", ap_ns_get_string_at(ns, "files", 0));
  STRCMP_EQUAL("b.txt", ap_ns_get_string_at(ns, "files", 1));
  CHECK(ap_ns_get_string(ns, "target", &target));
  STRCMP_EQUAL("dest.out", target);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsUnknownOptionDoesNotConsumeKnownOptionToken) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",  (char *)"--unknown", (char *)"--text",
                  (char *)"hello", (char *)"file.txt",  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 5, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(1, unknown_count);
  STRCMP_EQUAL("--unknown", unknown[0]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsUnknownInlineOptionRemainsSingleUnknownToken) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",  (char *)"--unknown=value", (char *)"-t",
                  (char *)"hello", (char *)"file.txt",        NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 5, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(1, unknown_count);
  STRCMP_EQUAL("--unknown=value", unknown[0]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ShortOptionClusterForBoolFlags) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options quiet = ap_arg_options_default();
  bool is_verbose = false;
  bool is_quiet = false;
  char *argv[] = {(char *)"prog", (char *)"-vq", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  quiet.type = AP_TYPE_BOOL;
  quiet.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "-q, --quiet", quiet, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "verbose", &is_verbose));
  CHECK(ap_ns_get_bool(ns, "quiet", &is_quiet));
  CHECK_TRUE(is_verbose);
  CHECK_TRUE(is_quiet);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ShortOptionClusterRejectsValueOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options number = ap_arg_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"-nv", (char *)"12", NULL};

  CHECK(p != NULL);
  number.type = AP_TYPE_INT32;
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "-n, --num", number, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);

  ap_parser_free(p);
}
