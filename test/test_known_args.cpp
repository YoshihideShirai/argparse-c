#include <string>
#include <vector>
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

TEST(ParseKnownArgsCollectsConsecutiveUnknownOptionsInOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {
      (char *)"prog", (char *)"--u1",  (char *)"--u2",     (char *)"--u3",
      (char *)"-t",   (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 7, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(3, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("--u2", unknown[1]);
  STRCMP_EQUAL("--u3", unknown[2]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsAlternatingUnknownOptionAndValueInOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",  (char *)"--u1",     (char *)"v1",
                  (char *)"--u2",  (char *)"v2",       (char *)"-t",
                  (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 8, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(4, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("v1", unknown[1]);
  STRCMP_EQUAL("--u2", unknown[2]);
  STRCMP_EQUAL("v2", unknown[3]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsLongTokenSequenceAfterDoubleDashInOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"file.txt",
                  (char *)"--",
                  (char *)"--u1",
                  (char *)"v1",
                  (char *)"--u2",
                  (char *)"v2",
                  (char *)"--u3",
                  (char *)"v3",
                  (char *)"tail",
                  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 12, argv, &ns, &unknown, &unknown_count, &err));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("file.txt", input);
  LONGS_EQUAL(7, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("v1", unknown[1]);
  STRCMP_EQUAL("--u2", unknown[2]);
  STRCMP_EQUAL("v2", unknown[3]);
  STRCMP_EQUAL("--u3", unknown[4]);
  STRCMP_EQUAL("v3", unknown[5]);
  STRCMP_EQUAL("tail", unknown[6]);

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

TEST(ParseKnownArgsCollectsAlternatingUnknownOptionValueAndKnownOptions) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;
  char *argv[] = {(char *)"prog", (char *)"--u1",  (char *)"v1",
                  (char *)"-t",   (char *)"hello", (char *)"--u2",
                  (char *)"v2",   (char *)"-n",    (char *)"7",
                  (char *)"--u3", (char *)"v3",    (char *)"file.txt",
                  (char *)"--u4", (char *)"v4",    NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 14, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(8, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("v1", unknown[1]);
  STRCMP_EQUAL("--u2", unknown[2]);
  STRCMP_EQUAL("v2", unknown[3]);
  STRCMP_EQUAL("--u3", unknown[4]);
  STRCMP_EQUAL("v3", unknown[5]);
  STRCMP_EQUAL("--u4", unknown[6]);
  STRCMP_EQUAL("v4", unknown[7]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(7, num);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsLongMixedUnknownSequencesAfterKnownParsingStops) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",
                  (char *)"--u1",
                  (char *)"--u2",
                  (char *)"--u3=value",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"file.txt",
                  (char *)"--",
                  (char *)"--tail-1",
                  (char *)"tail-value",
                  (char *)"--tail-2=value",
                  (char *)"tail-plain",
                  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 12, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(7, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("--u2", unknown[1]);
  STRCMP_EQUAL("--u3=value", unknown[2]);
  STRCMP_EQUAL("--tail-1", unknown[3]);
  STRCMP_EQUAL("tail-value", unknown[4]);
  STRCMP_EQUAL("--tail-2=value", unknown[5]);
  STRCMP_EQUAL("tail-plain", unknown[6]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsLongRunsOfUnknownOptionsInOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {
      (char *)"prog", (char *)"--u1",  (char *)"--u2",     (char *)"--u3",
      (char *)"-t",   (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 7, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(3, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("--u2", unknown[1]);
  STRCMP_EQUAL("--u3", unknown[2]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsUnknownOptionValuePairsWithoutStealingKnownTokens) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",  (char *)"--u1",     (char *)"v1",
                  (char *)"--u2",  (char *)"v2",       (char *)"-t",
                  (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 8, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(4, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("v1", unknown[1]);
  STRCMP_EQUAL("--u2", unknown[2]);
  STRCMP_EQUAL("v2", unknown[3]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsAllTokensAfterDoubleDashInOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",     (char *)"-t",    (char *)"hello",
                  (char *)"file.txt", (char *)"--",    (char *)"-x",
                  (char *)"--long",   (char *)"plain", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 8, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(3, unknown_count);
  STRCMP_EQUAL("-x", unknown[0]);
  STRCMP_EQUAL("--long", unknown[1]);
  STRCMP_EQUAL("plain", unknown[2]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(
    ParseKnownArgsCollectsAlternatingUnknownValueKnownSegmentsAcrossLongSequence) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;
  char *argv[] = {(char *)"prog",
                  (char *)"--u1",
                  (char *)"v1",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"--u2",
                  (char *)"v2",
                  (char *)"-n",
                  (char *)"11",
                  (char *)"--u3",
                  (char *)"v3",
                  (char *)"--u4",
                  (char *)"v4",
                  (char *)"file.txt",
                  (char *)"--u5",
                  (char *)"v5",
                  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 16, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(10, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("v1", unknown[1]);
  STRCMP_EQUAL("--u2", unknown[2]);
  STRCMP_EQUAL("v2", unknown[3]);
  STRCMP_EQUAL("--u3", unknown[4]);
  STRCMP_EQUAL("v3", unknown[5]);
  STRCMP_EQUAL("--u4", unknown[6]);
  STRCMP_EQUAL("v4", unknown[7]);
  STRCMP_EQUAL("--u5", unknown[8]);
  STRCMP_EQUAL("v5", unknown[9]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(11, num);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsLongUnknownRunsAndDoubleDashTailInOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog",
                  (char *)"--u1",
                  (char *)"--u2",
                  (char *)"--u3=value",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"file.txt",
                  (char *)"--",
                  (char *)"--tail-01",
                  (char *)"tail-01",
                  (char *)"--tail-02",
                  (char *)"tail-02",
                  (char *)"--tail-03=value",
                  (char *)"tail-plain",
                  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 14, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(9, unknown_count);
  STRCMP_EQUAL("--u1", unknown[0]);
  STRCMP_EQUAL("--u2", unknown[1]);
  STRCMP_EQUAL("--u3=value", unknown[2]);
  STRCMP_EQUAL("--tail-01", unknown[3]);
  STRCMP_EQUAL("tail-01", unknown[4]);
  STRCMP_EQUAL("--tail-02", unknown[5]);
  STRCMP_EQUAL("tail-02", unknown[6]);
  STRCMP_EQUAL("--tail-03=value", unknown[7]);
  STRCMP_EQUAL("tail-plain", unknown[8]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsLongDoubleDashTailInStableOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  std::vector<std::string> tail_tokens;
  std::vector<char *> argv;

  CHECK(p != NULL);
  argv.push_back((char *)"prog");
  argv.push_back((char *)"-t");
  argv.push_back((char *)"hello");
  argv.push_back((char *)"file.txt");
  argv.push_back((char *)"--");
  tail_tokens.reserve(24);
  for (int i = 0; i < 24; ++i) {
    if ((i % 2) == 0) {
      tail_tokens.push_back("--unknown-" + std::to_string(i));
    } else {
      tail_tokens.push_back("value-" + std::to_string(i));
    }
    argv.push_back((char *)tail_tokens.back().c_str());
  }

  LONGS_EQUAL(0, ap_parse_known_args(p, (int)argv.size(), argv.data(), &ns,
                                     &unknown, &unknown_count, &err));
  LONGS_EQUAL(24, unknown_count);
  for (int i = 0; i < 24; ++i) {
    STRCMP_EQUAL(tail_tokens[(size_t)i].c_str(), unknown[i]);
  }
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsAlternatingUnknownPairsDoNotStealKnownValuesInLongRun) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;
  std::vector<std::string> unknown_tokens;
  std::vector<char *> argv;
  const int unknown_pairs = 12;

  CHECK(p != NULL);
  argv.push_back((char *)"prog");
  unknown_tokens.reserve((size_t)unknown_pairs * 2);
  for (int i = 0; i < unknown_pairs; ++i) {
    unknown_tokens.push_back("--ux-" + std::to_string(i));
    unknown_tokens.push_back("uv-" + std::to_string(i));
    argv.push_back((char *)unknown_tokens[unknown_tokens.size() - 2].c_str());
    argv.push_back((char *)unknown_tokens.back().c_str());
    if (i == 3) {
      argv.push_back((char *)"-t");
      argv.push_back((char *)"hello");
    }
    if (i == 7) {
      argv.push_back((char *)"-n");
      argv.push_back((char *)"99");
    }
  }
  argv.push_back((char *)"file.txt");

  LONGS_EQUAL(0, ap_parse_known_args(p, (int)argv.size(), argv.data(), &ns,
                                     &unknown, &unknown_count, &err));
  LONGS_EQUAL(unknown_pairs * 2, unknown_count);
  for (int i = 0; i < unknown_count; ++i) {
    STRCMP_EQUAL(unknown_tokens[(size_t)i].c_str(), unknown[i]);
  }
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(99, num);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(
    ParseKnownArgsHandlesVeryLongAndManyUnknownTokensWithoutStealingNamespace) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  std::vector<std::string> storage;
  std::vector<char *> argv;
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;
  const size_t long_size = 8192;
  const int extra_tokens = 128;

  CHECK(p != NULL);
  storage.reserve((size_t)extra_tokens + 6);
  argv.push_back((char *)"prog");
  argv.push_back((char *)"--very-long");
  storage.push_back(std::string(long_size, 'x'));
  argv.push_back((char *)storage.back().c_str());
  argv.push_back((char *)"-t");
  argv.push_back((char *)"hello");
  argv.push_back((char *)"-n");
  argv.push_back((char *)"123");
  argv.push_back((char *)"file.txt");

  for (int i = 0; i < extra_tokens; ++i) {
    storage.push_back("--u-" + std::to_string(i));
    argv.push_back((char *)storage.back().c_str());
  }

  LONGS_EQUAL(0, ap_parse_known_args(p, (int)argv.size(), argv.data(), &ns,
                                     &unknown, &unknown_count, &err));
  LONGS_EQUAL(AP_ERR_NONE, err.code);
  LONGS_EQUAL(extra_tokens + 2, unknown_count);
  STRCMP_EQUAL("--very-long", unknown[0]);
  STRCMP_EQUAL(storage[0].c_str(), unknown[1]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(123, num);
  STRCMP_EQUAL("file.txt", input);
  CHECK(!ap_ns_get_string(ns, "very_long", &text));

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsAlternatingUnknownOptionValueDoesNotAbsorbKnownTokens) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  std::vector<std::string> unknown_storage;
  std::vector<char *> argv;
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;
  const int pairs = 20;

  CHECK(p != NULL);
  unknown_storage.reserve((size_t)pairs * 2);
  argv.push_back((char *)"prog");
  for (int i = 0; i < pairs; ++i) {
    unknown_storage.push_back("--z-" + std::to_string(i));
    unknown_storage.push_back("zv-" + std::to_string(i));
    argv.push_back((char *)unknown_storage[unknown_storage.size() - 2].c_str());
    argv.push_back((char *)unknown_storage.back().c_str());
    if (i == 5) {
      argv.push_back((char *)"-t");
      argv.push_back((char *)"hello");
    }
    if (i == 11) {
      argv.push_back((char *)"-n");
      argv.push_back((char *)"77");
    }
  }
  argv.push_back((char *)"file.txt");

  LONGS_EQUAL(0, ap_parse_known_args(p, (int)argv.size(), argv.data(), &ns,
                                     &unknown, &unknown_count, &err));
  LONGS_EQUAL(AP_ERR_NONE, err.code);
  LONGS_EQUAL(pairs * 2, unknown_count);
  for (int i = 0; i < unknown_count; ++i) {
    STRCMP_EQUAL(unknown_storage[(size_t)i].c_str(), unknown[i]);
  }
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(77, num);
  STRCMP_EQUAL("file.txt", input);
  CHECK(!ap_ns_get_string(ns, "z_0", &text));

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}
