#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "argparse-c.h"
}

namespace {

struct TestCase {
  const char *name;
  void (*run)();
};

std::vector<TestCase> &registry() {
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar {
  Registrar(const char *name, void (*run)()) { registry().push_back({name, run}); }
};

[[noreturn]] void fail(const char *expr, const char *file, int line,
                      const std::string &detail = "") {
  std::ostringstream oss;
  oss << file << ':' << line << ": assertion failed: " << expr;
  if (!detail.empty()) {
    oss << " (" << detail << ')';
  }
  throw std::runtime_error(oss.str());
}

#define TEST(name)                                                              \
  static void name();                                                           \
  static Registrar name##_registrar(#name, &name);                              \
  static void name()

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      fail(#condition, __FILE__, __LINE__);                                     \
    }                                                                           \
  } while (0)

#define CHECK_TRUE(condition) CHECK(condition)

#define LONGS_EQUAL(expected, actual)                                           \
  do {                                                                          \
    auto expected_value = (expected);                                           \
    auto actual_value = (actual);                                               \
    if (expected_value != actual_value) {                                       \
      std::ostringstream detail;                                                \
      detail << "expected " << expected_value << ", got " << actual_value;     \
      fail(#actual, __FILE__, __LINE__, detail.str());                          \
    }                                                                           \
  } while (0)

#define STRCMP_EQUAL(expected, actual)                                          \
  do {                                                                          \
    const char *expected_value = (expected);                                    \
    const char *actual_value = (actual);                                        \
    if (((expected_value) == NULL) != ((actual_value) == NULL) ||               \
        ((expected_value) != NULL && strcmp(expected_value, actual_value) != 0)) { \
      std::ostringstream detail;                                                \
      detail << "expected '" << (expected_value ? expected_value : "(null)")    \
             << "', got '" << (actual_value ? actual_value : "(null)") << "'"; \
      fail(#actual, __FILE__, __LINE__, detail.str());                          \
    }                                                                           \
  } while (0)

static ap_parser *new_base_parser(void) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  if (!p) {
    return NULL;
  }

  ap_arg_options text_opts = ap_arg_options_default();
  text_opts.required = true;
  text_opts.help = "text";
  if (ap_add_argument(p, "-t, --text", text_opts, &err) != 0) {
    ap_parser_free(p);
    return NULL;
  }

  ap_arg_options int_opts = ap_arg_options_default();
  int_opts.type = AP_TYPE_INT32;
  if (ap_add_argument(p, "-n, --num", int_opts, &err) != 0) {
    ap_parser_free(p);
    return NULL;
  }

  ap_arg_options pos_opts = ap_arg_options_default();
  pos_opts.help = "input";
  if (ap_add_argument(p, "input", pos_opts, &err) != 0) {
    ap_parser_free(p);
    return NULL;
  }

  return p;
}

TEST(SuccessParse) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog", (char *)"-t", (char *)"hello", (char *)"-n",
                  (char *)"12", (char *)"file.txt", NULL};
  const char *text = NULL;
  const char *input = NULL;
  int32_t num = 0;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_args(p, 6, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(12, num);
  STRCMP_EQUAL("file.txt", input);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(UnknownOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog", (char *)"--bogus", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNKNOWN_OPTION, err.code);
  STRCMP_EQUAL("--bogus", err.argument);
  STRCMP_EQUAL("unknown option '--bogus'", err.message);

  ap_parser_free(p);
}

TEST(InvalidInt) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog", (char *)"-t", (char *)"x", (char *)"-n",
                  (char *)"abc", (char *)"file", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(-1, ap_parse_args(p, 6, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_INT32, err.code);
  STRCMP_EQUAL("-n", err.argument);
  STRCMP_EQUAL("argument '-n' must be a valid int32: 'abc'", err.message);

  ap_parser_free(p);
}

TEST(ChoicesValidation) {
  static const char *choices[] = {"fast", "slow"};
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  char *argv_bad[] = {(char *)"prog", (char *)"--mode", (char *)"medium", NULL};

  CHECK(p != NULL);
  mode.help = "mode";
  mode.choices.items = choices;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_bad, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_CHOICE, err.code);
  STRCMP_EQUAL("--mode", err.argument);
  STRCMP_EQUAL("invalid choice 'medium' for option '--mode'", err.message);

  ap_parser_free(p);
}

TEST(NargsOneOrMore) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options files = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--files", (char *)"a.txt", (char *)"b.txt", NULL};

  CHECK(p != NULL);
  files.nargs = AP_NARGS_ONE_OR_MORE;
  LONGS_EQUAL(0, ap_add_argument(p, "--files", files, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 4, argv, &ns, &err));
  LONGS_EQUAL(2, ap_ns_get_count(ns, "files"));
  STRCMP_EQUAL("a.txt", ap_ns_get_string_at(ns, "files", 0));
  STRCMP_EQUAL("b.txt", ap_ns_get_string_at(ns, "files", 1));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(DefaultValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options name = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};
  const char *actual = NULL;

  CHECK(p != NULL);
  name.default_value = "guest";
  LONGS_EQUAL(0, ap_add_argument(p, "--name", name, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 1, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "name", &actual));
  STRCMP_EQUAL("guest", actual);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(AutoDestPrefersLongFlagAndNormalizesHyphen) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  char *argv[] = {(char *)"prog", (char *)"--dry-run", (char *)"yes", NULL};
  const char *value = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "-d, --dry-run", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "dry_run", &value));
  STRCMP_EQUAL("yes", value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(AutoDestNormalizesPositionalHyphen) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  char *argv[] = {(char *)"prog", (char *)"report.txt", NULL};
  const char *value = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "input-file", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "input_file", &value));
  STRCMP_EQUAL("report.txt", value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ExplicitDestIsNotNormalized) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options opts = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--dry-run", (char *)"yes", NULL};
  const char *value = NULL;

  CHECK(p != NULL);
  opts.dest = "custom-key";
  LONGS_EQUAL(0, ap_add_argument(p, "--dry-run", opts, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "custom-key", &value));
  STRCMP_EQUAL("yes", value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(HelpGeneration) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options arg = ap_arg_options_default();
  char *help;

  CHECK(p != NULL);
  arg.help = "value";
  LONGS_EQUAL(0, ap_add_argument(p, "--value", arg, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "usage: prog") != NULL);
  CHECK(strstr(help, "--value") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(InlineOptionValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog", (char *)"--text=hello", (char *)"-n=33",
                  (char *)"file.txt", NULL};
  const char *text = NULL;
  int32_t num = 0;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_args(p, 4, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_int32(ns, "num", &num));
  STRCMP_EQUAL("hello", text);
  LONGS_EQUAL(33, num);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(RequiredOptionIgnoresDefaultWhenMissing) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options name = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  name.required = true;
  name.default_value = "guest";
  LONGS_EQUAL(0, ap_add_argument(p, "--name", name, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("--name", err.argument);
  STRCMP_EQUAL("option '--name' is required", err.message);

  ap_parser_free(p);
}

TEST(DefaultValueMustMatchChoices) {
  static const char *choices[] = {"fast", "slow"};
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  mode.choices.items = choices;
  mode.choices.count = 2;
  mode.default_value = "medium";
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_CHOICE, err.code);

  ap_parser_free(p);
}

TEST(StoreTrueAndStoreFalse) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options quiet = ap_arg_options_default();
  bool is_verbose = false;
  bool is_quiet = false;
  char *argv[] = {(char *)"prog", (char *)"--verbose", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  quiet.type = AP_TYPE_BOOL;
  quiet.action = AP_ACTION_STORE_FALSE;
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--quiet", quiet, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "verbose", &is_verbose));
  CHECK(ap_ns_get_bool(ns, "quiet", &is_quiet));
  CHECK_TRUE(is_verbose);
  CHECK_TRUE(is_quiet);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(StoreTrueRejectsInlineValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--verbose=yes", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);

  ap_parser_free(p);
}

TEST(HelpShowsChoicesDefaultAndRequired) {
  static const char *choices[] = {"fast", "slow"};
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options files = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  mode.required = true;
  mode.default_value = "fast";
  mode.choices.items = choices;
  mode.choices.count = 2;
  mode.help = "run mode";
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  files.nargs = AP_NARGS_ONE_OR_MORE;
  files.help = "input files";
  LONGS_EQUAL(0, ap_add_argument(p, "--files", files, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "--mode MODE") != NULL);
  CHECK(strstr(help, "choices: fast, slow") != NULL);
  CHECK(strstr(help, "default: fast") != NULL);
  CHECK(strstr(help, "required: true") != NULL);
  CHECK(strstr(help, "--files FILES [FILES ...]") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatErrorIncludesMessageAndUsage) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog", (char *)"--bogus", NULL};
  char *text = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNKNOWN_OPTION, err.code);

  text = ap_format_error(p, &err);
  CHECK(text != NULL);
  CHECK(strstr(text, "error: unknown option '--bogus'") != NULL);
  CHECK(strstr(text, "usage: prog") != NULL);
  CHECK(strstr(text, "-t TEXT") != NULL);

  free(text);
  ap_parser_free(p);
}

TEST(ParseKnownArgsCollectsUnknownOptions) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  char *argv[] = {(char *)"prog", (char *)"--bogus", (char *)"-t",
                  (char *)"hello", (char *)"file.txt", (char *)"--x=1", NULL};
  const char *text = NULL;
  const char *input = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count,
                                     &err));
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
  LONGS_EQUAL(-1, ap_parse_known_args(p, 3, argv, &ns, &unknown, &unknown_count,
                                      &err));
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
  char *argv[] = {(char *)"prog", (char *)"--unknown", (char *)"uval",
                  (char *)"-t", (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count,
                                     &err));
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
  char *argv[] = {(char *)"prog", (char *)"-t", (char *)"hello",
                  (char *)"file.txt", (char *)"extra1", (char *)"extra2",
                  NULL};
  const char *input = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count,
                                     &err));
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
  char *argv[] = {(char *)"prog", (char *)"-t", (char *)"hello",
                  (char *)"file.txt", (char *)"--", (char *)"--passthrough",
                  (char *)"value", NULL};
  const char *input = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 7, argv, &ns, &unknown, &unknown_count,
                                     &err));
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
  char *argv[] = {(char *)"prog", (char *)"--unknown", (char *)"--text",
                  (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 5, argv, &ns, &unknown, &unknown_count,
                                     &err));
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
  char *argv[] = {(char *)"prog", (char *)"--unknown=value", (char *)"-t",
                  (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 5, argv, &ns, &unknown, &unknown_count,
                                     &err));
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

TEST(ParseSubcommandArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *run = NULL;
  ap_arg_options config = ap_arg_options_default();
  ap_arg_options force = ap_arg_options_default();
  const char *subcommand = NULL;
  const char *config_value = NULL;
  bool force_enabled = false;
  char *argv[] = {(char *)"prog", (char *)"run", (char *)"--config",
                  (char *)"prod.json", (char *)"--force", NULL};

  CHECK(p != NULL);
  run = ap_add_subcommand(p, "run", "run the job", &err);
  CHECK(run != NULL);
  force.type = AP_TYPE_BOOL;
  force.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(run, "--config", config, &err));
  LONGS_EQUAL(0, ap_add_argument(run, "--force", force, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 5, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "subcommand", &subcommand));
  CHECK(ap_ns_get_string(ns, "config", &config_value));
  CHECK(ap_ns_get_bool(ns, "force", &force_enabled));
  STRCMP_EQUAL("run", subcommand);
  STRCMP_EQUAL("prod.json", config_value);
  CHECK_TRUE(force_enabled);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(HelpListsSubcommands) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *run = NULL;
  char *help;

  CHECK(p != NULL);
  run = ap_add_subcommand(p, "run", "run the job", &err);
  CHECK(run != NULL);

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "subcommands:") != NULL);
  CHECK(strstr(help, "run") != NULL);
  CHECK(strstr(help, "run the job") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(ParseNestedSubcommandArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *config = NULL;
  ap_parser *set = NULL;
  ap_arg_options global = ap_arg_options_default();
  ap_arg_options value = ap_arg_options_default();
  const char *subcommand = NULL;
  const char *parent_subcommand = NULL;
  const char *subcommand_path = NULL;
  const char *value_text = NULL;
  const char *key = NULL;
  bool is_global = false;
  char *argv[] = {(char *)"prog", (char *)"config", (char *)"set",
                  (char *)"--global", (char *)"--value", (char *)"blue",
                  (char *)"theme", NULL};

  CHECK(p != NULL);
  config = ap_add_subcommand(p, "config", "config commands", &err);
  CHECK(config != NULL);
  set = ap_add_subcommand(config, "set", "set a value", &err);
  CHECK(set != NULL);

  global.type = AP_TYPE_BOOL;
  global.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(set, "--global", global, &err));
  LONGS_EQUAL(0, ap_add_argument(set, "--value", value, &err));
  LONGS_EQUAL(0, ap_add_argument(set, "key", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 7, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "subcommand", &subcommand));
  CHECK(ap_ns_get_bool(ns, "global", &is_global));
  CHECK(ap_ns_get_string(ns, "value", &value_text));
  CHECK(ap_ns_get_string(ns, "key", &key));
  CHECK(!ap_ns_get_string(ns, "config", &parent_subcommand));
  CHECK(!ap_ns_get_string(ns, "subcommand_path", &subcommand_path));
  STRCMP_EQUAL("set", subcommand);
  CHECK_TRUE(is_global);
  STRCMP_EQUAL("blue", value_text);
  STRCMP_EQUAL("theme", key);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(NestedSubcommandHelpUsesFullCommandPath) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *config = NULL;
  ap_parser *set = NULL;
  char *help = NULL;

  CHECK(p != NULL);
  config = ap_add_subcommand(p, "config", "config commands", &err);
  CHECK(config != NULL);
  set = ap_add_subcommand(config, "set", "set a value", &err);
  CHECK(set != NULL);

  help = ap_format_help(set);
  CHECK(help != NULL);
  CHECK(strstr(help, "usage: prog config set") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(MissingSubcommandFails) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  CHECK(ap_add_subcommand(p, "run", "run the job", &err) != NULL);

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);

  ap_parser_free(p);
}



TEST(CountRequiresInt32TypeDefinition) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();

  CHECK(p != NULL);
  verbose.type = AP_TYPE_STRING;
  verbose.action = AP_ACTION_COUNT;

  LONGS_EQUAL(-1, ap_add_argument(p, "--verbose", verbose, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--verbose", err.argument);
  STRCMP_EQUAL("count action requires int32 type", err.message);

  ap_parser_free(p);
}

TEST(InvalidNargsDefinitionIsRejected) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options value = ap_arg_options_default();

  CHECK(p != NULL);
  value.nargs = (ap_nargs)99;

  LONGS_EQUAL(-1, ap_add_argument(p, "--value", value, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--value", err.argument);
  STRCMP_EQUAL("invalid nargs definition", err.message);

  ap_parser_free(p);
}

TEST(DuplicateDestIsRejected) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options first = ap_arg_options_default();
  ap_arg_options second = ap_arg_options_default();

  CHECK(p != NULL);
  first.dest = "same";
  second.dest = "same";
  LONGS_EQUAL(0, ap_add_argument(p, "--first", first, &err));

  LONGS_EQUAL(-1, ap_add_argument(p, "--second", second, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("same", err.argument);
  STRCMP_EQUAL("dest 'same' already exists", err.message);

  ap_parser_free(p);
}

TEST(OptionalArgumentDeclarationRejectsNonFlagToken) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  CHECK(p != NULL);
  LONGS_EQUAL(-1, ap_add_argument(p, "-v, verbose", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("verbose", err.argument);
  STRCMP_EQUAL("optional argument flags must start with '-'", err.message);

  ap_parser_free(p);
}

TEST(PositionalDeclarationRejectsMultipleTokens) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  CHECK(p != NULL);
  LONGS_EQUAL(-1, ap_add_argument(p, "input, output", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("input, output", err.argument);
  STRCMP_EQUAL("positional argument must be a single non-flag token", err.message);

  ap_parser_free(p);
}

TEST(DuplicateSubcommandIsRejected) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  CHECK(p != NULL);
  CHECK(ap_add_subcommand(p, "run", "first", &err) != NULL);
  CHECK(ap_add_subcommand(p, "run", "second", &err) == NULL);
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("run", err.argument);
  STRCMP_EQUAL("subcommand 'run' already exists", err.message);

  ap_parser_free(p);
}

TEST(InvalidSubcommandNameIsRejected) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  CHECK(p != NULL);
  CHECK(ap_add_subcommand(p, "bad name", "desc", &err) == NULL);
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("bad name", err.argument);
  STRCMP_EQUAL("subcommand name must be a single non-flag token", err.message);

  ap_parser_free(p);
}

TEST(GroupAddArgumentRequiresGroup) {
  ap_error err = {};

  LONGS_EQUAL(-1, ap_group_add_argument(NULL, "--json", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("group is required", err.message);
}

TEST(MissingValueWhenNextTokenIsKnownOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--name", (char *)"--verbose", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "--name", ap_arg_options_default(), &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_VALUE, err.code);
  STRCMP_EQUAL("--name", err.argument);
  STRCMP_EQUAL("option '--name' requires a value", err.message);

  ap_parser_free(p);
}

TEST(OneOrMoreOptionRequiresAtLeastOneValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options files = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--files", NULL};

  CHECK(p != NULL);
  files.nargs = AP_NARGS_ONE_OR_MORE;
  LONGS_EQUAL(0, ap_add_argument(p, "--files", files, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("--files", err.argument);
  STRCMP_EQUAL("option '--files' requires at least one value", err.message);

  ap_parser_free(p);
}

TEST(FixedOptionRejectsMissingValues) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options range = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--range", (char *)"10", NULL};

  CHECK(p != NULL);
  range.nargs = AP_NARGS_FIXED;
  range.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--range", range, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("--range", err.argument);
  STRCMP_EQUAL("option '--range' requires exactly 2 values", err.message);

  ap_parser_free(p);
}

TEST(PositionalOneOrMoreRequiresValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options files = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  files.nargs = AP_NARGS_ONE_OR_MORE;
  files.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "files", files, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("files", err.argument);
  STRCMP_EQUAL("argument 'files' requires at least one value", err.message);

  ap_parser_free(p);
}

TEST(PositionalFixedRequiresExactValues) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options coords = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"10", NULL};

  CHECK(p != NULL);
  coords.nargs = AP_NARGS_FIXED;
  coords.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "coords", coords, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("coords", err.argument);
  STRCMP_EQUAL("argument 'coords' requires exactly 2 values", err.message);

  ap_parser_free(p);
}

TEST(UnexpectedPositionalArgumentFails) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  char *argv[] = {(char *)"prog", (char *)"input.txt", NULL};

  CHECK(p != NULL);

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("input.txt", err.argument);
  STRCMP_EQUAL("unexpected positional argument 'input.txt'", err.message);

  ap_parser_free(p);
}

TEST(DuplicateLongOptionFails) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  char *argv[] = {(char *)"prog", (char *)"--name", (char *)"a", (char *)"--name", (char *)"b", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "--name", ap_arg_options_default(), &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 5, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_DUPLICATE_OPTION, err.code);
  STRCMP_EQUAL("--name", err.argument);
  STRCMP_EQUAL("duplicate option '--name'", err.message);

  ap_parser_free(p);
}

TEST(UnknownShortOptionInClusterFails) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"-vz", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNKNOWN_OPTION, err.code);
  STRCMP_EQUAL("-z", err.argument);
  STRCMP_EQUAL("unknown option '-z'", err.message);

  ap_parser_free(p);
}

TEST(ParseKnownArgsTreatsUnknownClusterAsUnknownToken) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *text = NULL;
  const char *input = NULL;
  char *argv[] = {(char *)"prog", (char *)"-xz", (char *)"-t", (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parse_known_args(p, 5, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(1, unknown_count);
  STRCMP_EQUAL("-xz", unknown[0]);
  CHECK(ap_ns_get_string(ns, "text", &text));
  CHECK(ap_ns_get_string(ns, "input", &input));
  STRCMP_EQUAL("hello", text);
  STRCMP_EQUAL("file.txt", input);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(FormatHelpCoversOptionalAndPositionalNargsVariants) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options item = ap_arg_options_default();
  ap_arg_options paths = ap_arg_options_default();
  ap_arg_options coords = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  maybe.nargs = AP_NARGS_OPTIONAL;
  maybe.metavar = "VALUE";
  maybe.help = "optional value";
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.metavar = "EXTRA";
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  pair.metavar = "PAIR";
  item.nargs = AP_NARGS_OPTIONAL;
  item.metavar = "ITEM";
  paths.nargs = AP_NARGS_ZERO_OR_MORE;
  paths.action = AP_ACTION_APPEND;
  paths.metavar = "PATH";
  coords.nargs = AP_NARGS_FIXED;
  coords.nargs_count = 2;
  coords.metavar = "COORD";

  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "item", item, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "paths", paths, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "coords", coords, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "[--maybe [VALUE]]") != NULL);
  CHECK(strstr(help, "[--extras [EXTRA ...]]") != NULL);
  CHECK(strstr(help, "[--pair PAIR PAIR]") != NULL);
  CHECK(strstr(help, "[ITEM]") != NULL);
  CHECK(strstr(help, "[PATH ...]") != NULL);
  CHECK(strstr(help, "COORD COORD") != NULL);
  CHECK(strstr(help, "optional value") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatUsageShowsRequiredBoolAndConstActions) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options enable = ap_arg_options_default();
  ap_arg_options color = ap_arg_options_default();
  char *usage = NULL;

  CHECK(p != NULL);
  enable.type = AP_TYPE_BOOL;
  enable.action = AP_ACTION_STORE_TRUE;
  enable.required = true;
  color.action = AP_ACTION_STORE_CONST;
  color.const_value = "always";
  color.required = true;

  LONGS_EQUAL(0, ap_add_argument(p, "--enable", enable, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--color", color, &err));

  usage = ap_format_usage(p);
  CHECK(usage != NULL);
  CHECK(strstr(usage, " --enable") != NULL);
  CHECK(strstr(usage, " --color") != NULL);
  CHECK(strstr(usage, "[--color]") == NULL);

  free(usage);
  ap_parser_free(p);
}

TEST(HelpOptionBypassesRequiredArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options required = ap_arg_options_default();
  bool help = false;
  char *argv[] = {(char *)"prog", (char *)"--help", NULL};

  CHECK(p != NULL);
  required.required = true;
  LONGS_EQUAL(0, ap_add_argument(p, "--name", required, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "help", &help));
  CHECK_TRUE(help);
  LONGS_EQUAL(0, ap_ns_get_count(ns, "name"));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(HelpOptionBypassesRequiredSubcommand) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  bool help = false;
  char *argv[] = {(char *)"prog", (char *)"--help", NULL};

  CHECK(p != NULL);
  CHECK(ap_add_subcommand(p, "run", "run the job", &err) != NULL);

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "help", &help));
  CHECK_TRUE(help);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ShortOptionClusterRejectsDuplicateBoolFlag) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"-vv", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_DUPLICATE_OPTION, err.code);
  STRCMP_EQUAL("-v", err.argument);
  STRCMP_EQUAL("duplicate option '-v'", err.message);

  ap_parser_free(p);
}

TEST(StoreTrueRequiresBoolTypeDefinition) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();

  CHECK(p != NULL);
  verbose.type = AP_TYPE_STRING;
  verbose.action = AP_ACTION_STORE_TRUE;

  LONGS_EQUAL(-1, ap_add_argument(p, "--verbose", verbose, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--verbose", err.argument);
  STRCMP_EQUAL("store_true/store_false requires bool type", err.message);

  ap_parser_free(p);
}

TEST(StoreConstRequiresConstValueDefinition) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();

  CHECK(p != NULL);
  mode.action = AP_ACTION_STORE_CONST;

  LONGS_EQUAL(-1, ap_add_argument(p, "--mode", mode, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--mode", err.argument);
  STRCMP_EQUAL("store_const requires const_value", err.message);

  ap_parser_free(p);
}

TEST(FixedNargsRequiresPositiveCountDefinition) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options range = ap_arg_options_default();

  CHECK(p != NULL);
  range.nargs = AP_NARGS_FIXED;
  range.nargs_count = 0;

  LONGS_EQUAL(-1, ap_add_argument(p, "--range", range, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--range", err.argument);
  STRCMP_EQUAL("fixed nargs requires nargs_count > 0", err.message);

  ap_parser_free(p);
}

TEST(CountActionAccumulatesShortFlags) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  int32_t count = 0;
  char *argv[] = {(char *)"prog", (char *)"-vvv", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_int32(ns, "verbose", &count));
  LONGS_EQUAL(3, count);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(AppendActionCollectsRepeatedOptionValues) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options include = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--include", (char *)"a",
                  (char *)"--include", (char *)"b", NULL};

  CHECK(p != NULL);
  include.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "--include", include, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 5, argv, &ns, &err));
  LONGS_EQUAL(2, ap_ns_get_count(ns, "include"));
  STRCMP_EQUAL("a", ap_ns_get_string_at(ns, "include", 0));
  STRCMP_EQUAL("b", ap_ns_get_string_at(ns, "include", 1));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(StoreConstUsesConstValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options color = ap_arg_options_default();
  const char *value = NULL;
  char *argv[] = {(char *)"prog", (char *)"--always", NULL};

  CHECK(p != NULL);
  color.action = AP_ACTION_STORE_CONST;
  color.const_value = "always";
  LONGS_EQUAL(0, ap_add_argument(p, "--always", color, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "always", &value));
  STRCMP_EQUAL("always", value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(FixedNargsForOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options range = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--range", (char *)"10",
                  (char *)"20", NULL};
  int32_t start = 0;
  int32_t end = 0;

  CHECK(p != NULL);
  range.type = AP_TYPE_INT32;
  range.nargs = AP_NARGS_FIXED;
  range.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--range", range, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 4, argv, &ns, &err));
  LONGS_EQUAL(2, ap_ns_get_count(ns, "range"));
  CHECK(ap_ns_get_int32_at(ns, "range", 0, &start));
  CHECK(ap_ns_get_int32_at(ns, "range", 1, &end));
  LONGS_EQUAL(10, start);
  LONGS_EQUAL(20, end);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(MutuallyExclusiveGroupRejectsConflicts) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_mutually_exclusive_group *group = NULL;
  ap_arg_options json = ap_arg_options_default();
  ap_arg_options yaml = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--json", (char *)"--yaml", NULL};

  CHECK(p != NULL);
  json.type = AP_TYPE_BOOL;
  json.action = AP_ACTION_STORE_TRUE;
  yaml.type = AP_TYPE_BOOL;
  yaml.action = AP_ACTION_STORE_TRUE;
  group = ap_add_mutually_exclusive_group(p, false, &err);
  CHECK(group != NULL);
  LONGS_EQUAL(0, ap_group_add_argument(group, "--json", json, &err));
  LONGS_EQUAL(0, ap_group_add_argument(group, "--yaml", yaml, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv, &ns, &err));

  ap_parser_free(p);
}

TEST(RequiredMutuallyExclusiveGroupNeedsOneOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_mutually_exclusive_group *group = NULL;
  ap_arg_options json = ap_arg_options_default();
  ap_arg_options yaml = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  json.type = AP_TYPE_BOOL;
  json.action = AP_ACTION_STORE_TRUE;
  yaml.type = AP_TYPE_BOOL;
  yaml.action = AP_ACTION_STORE_TRUE;
  group = ap_add_mutually_exclusive_group(p, true, &err);
  CHECK(group != NULL);
  LONGS_EQUAL(0, ap_group_add_argument(group, "--json", json, &err));
  LONGS_EQUAL(0, ap_group_add_argument(group, "--yaml", yaml, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("one argument from a mutually exclusive group is required", err.message);

  ap_parser_free(p);
}

TEST(MissingRequiredPositionalUsesConsistentArgumentNameAndMessage) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options opts = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  opts.required = true;
  LONGS_EQUAL(0, ap_add_argument(p, "input-file", opts, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("input-file", err.argument);
  STRCMP_EQUAL("argument 'input-file' is required", err.message);

  ap_parser_free(p);
}

} // namespace

int main() {
  int failures = 0;

  for (const TestCase &test : registry()) {
    try {
      test.run();
      std::fprintf(stdout, "[PASS] %s\n", test.name);
    } catch (const std::exception &ex) {
      std::fprintf(stderr, "[FAIL] %s: %s\n", test.name, ex.what());
      ++failures;
    } catch (...) {
      std::fprintf(stderr, "[FAIL] %s: unknown exception\n", test.name);
      ++failures;
    }
  }

  if (failures == 0) {
    std::fprintf(stdout, "OK (%zu tests)\n", registry().size());
    return 0;
  }

  std::fprintf(stderr, "%d test(s) failed\n", failures);
  return 1;
}
