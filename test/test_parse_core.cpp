#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#ifdef __cplusplus
extern "C" {
#endif
#include "../lib/ap_internal.h"
#ifdef __cplusplus
}
#endif
#include "test_framework.h"

TEST(SuccessParse) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"-n",
                  (char *)"12",
                  (char *)"file.txt",
                  NULL};
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
  char *argv[] = {(char *)"prog", (char *)"-t",   (char *)"x", (char *)"-n",
                  (char *)"abc",  (char *)"file", NULL};

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
  char *argv[] = {(char *)"prog", (char *)"--files", (char *)"a.txt",
                  (char *)"b.txt", NULL};

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
  LONGS_EQUAL(
      0, ap_add_argument(p, "-d, --dry-run", ap_arg_options_default(), &err));

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
  LONGS_EQUAL(0,
              ap_add_argument(p, "input-file", ap_arg_options_default(), &err));

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

TEST(HelpBypassesValidationRequiredArgumentAndParsesSuccessfully) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options required_name = ap_arg_options_default();
  bool help = false;
  char *argv[] = {(char *)"prog", (char *)"--help", NULL};

  CHECK(p != NULL);
  required_name.required = true;
  LONGS_EQUAL(0, ap_add_argument(p, "--name", required_name, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "help", &help));
  CHECK_TRUE(help);
  LONGS_EQUAL(0, ap_ns_get_count(ns, "name"));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(HelpBypassesValidationChoiceAndNargsErrors) {
  static const char *choices[] = {"fast", "slow"};
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options files = ap_arg_options_default();
  bool help = false;
  char *argv[] = {(char *)"prog", (char *)"--help", NULL};

  CHECK(p != NULL);
  mode.choices.items = choices;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  files.nargs = AP_NARGS_ONE_OR_MORE;
  LONGS_EQUAL(0, ap_add_argument(p, "--files", files, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "help", &help));
  CHECK_TRUE(help);
  LONGS_EQUAL(0, ap_ns_get_count(ns, "mode"));
  LONGS_EQUAL(0, ap_ns_get_count(ns, "files"));

  ap_namespace_free(ns);
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

TEST(NextPositionalDefResolvesOptionalZeroOrMoreOneOrMoreAndFixedLayouts) {
  ap_error err = {};
  ap_parser *optional_then_required = ap_parser_new("prog", "desc");
  ap_parser *many_then_required = ap_parser_new("prog", "desc");
  ap_parser *fixed_then_tail = ap_parser_new("prog", "desc");
  ap_arg_options optional = ap_arg_options_default();
  ap_arg_options many = ap_arg_options_default();
  ap_arg_options plus = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  const ap_arg_def *def = NULL;

  CHECK(optional_then_required != NULL);
  CHECK(many_then_required != NULL);
  CHECK(fixed_then_tail != NULL);

  optional.nargs = AP_NARGS_OPTIONAL;
  LONGS_EQUAL(0,
              ap_add_argument(optional_then_required, "maybe", optional, &err));
  LONGS_EQUAL(0, ap_add_argument(optional_then_required, "target",
                                 ap_arg_options_default(), &err));
  def = ap_next_positional_def(optional_then_required, 0);
  CHECK(def != NULL);
  STRCMP_EQUAL("target", def->dest);

  many.nargs = AP_NARGS_ZERO_OR_MORE;
  many.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(many_then_required, "extras", many, &err));
  LONGS_EQUAL(0, ap_add_argument(many_then_required, "target",
                                 ap_arg_options_default(), &err));
  def = ap_next_positional_def(many_then_required, 0);
  CHECK(def != NULL);
  STRCMP_EQUAL("target", def->dest);

  plus.nargs = AP_NARGS_ONE_OR_MORE;
  LONGS_EQUAL(0, ap_add_argument(many_then_required, "items", plus, &err));
  def = ap_next_positional_def(many_then_required, 1);
  CHECK(def != NULL);
  STRCMP_EQUAL("items", def->dest);

  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(fixed_then_tail, "coords", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(fixed_then_tail, "target",
                                 ap_arg_options_default(), &err));
  def = ap_next_positional_def(fixed_then_tail, 0);
  CHECK(def != NULL);
  STRCMP_EQUAL("coords", def->dest);
  def = ap_next_positional_def(fixed_then_tail, 1);
  CHECK(def != NULL);
  STRCMP_EQUAL("coords", def->dest);
  def = ap_next_positional_def(fixed_then_tail, 2);
  CHECK(def != NULL);
  STRCMP_EQUAL("target", def->dest);
  CHECK(ap_next_positional_def(fixed_then_tail, -1) == NULL);
  CHECK(ap_next_positional_def(NULL, 0) == NULL);

  ap_parser_free(optional_then_required);
  ap_parser_free(many_then_required);
  ap_parser_free(fixed_then_tail);
}

TEST(InternalValidationCatchesDeferredNargsCasesAndAcceptsMatchingChoices) {
  ap_error err = {};
  ap_parser *one = ap_parser_new("prog", "desc");
  ap_parser *plus = ap_parser_new("prog", "desc");
  ap_parser *fixed = ap_parser_new("prog", "desc");
  ap_parser *choices = ap_parser_new("prog", "desc");
  ap_parsed_arg *parsed = NULL;
  ap_arg_options files = ap_arg_options_default();
  ap_arg_options coords = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  const char *valid_modes[] = {"fast", "slow"};

  CHECK(one != NULL);
  CHECK(plus != NULL);
  CHECK(fixed != NULL);
  CHECK(choices != NULL);

  LONGS_EQUAL(0,
              ap_add_argument(one, "--name", ap_arg_options_default(), &err));
  parsed = (ap_parsed_arg *)calloc((size_t)one->defs_count, sizeof(*parsed));
  CHECK(parsed != NULL);
  parsed[1].seen = true;
  LONGS_EQUAL(-1, ap_validate_args(one, parsed, &err));
  LONGS_EQUAL(AP_ERR_MISSING_VALUE, err.code);
  STRCMP_EQUAL("--name", err.argument);
  STRCMP_EQUAL("option '--name' requires a value", err.message);
  free(parsed);

  files.nargs = AP_NARGS_ONE_OR_MORE;
  LONGS_EQUAL(0, ap_add_argument(plus, "--files", files, &err));
  parsed = (ap_parsed_arg *)calloc((size_t)plus->defs_count, sizeof(*parsed));
  CHECK(parsed != NULL);
  parsed[1].seen = true;
  LONGS_EQUAL(-1, ap_validate_args(plus, parsed, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("--files", err.argument);
  STRCMP_EQUAL("option '--files' requires at least one value", err.message);
  free(parsed);

  coords.nargs = AP_NARGS_FIXED;
  coords.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(fixed, "coords", coords, &err));
  parsed = (ap_parsed_arg *)calloc((size_t)fixed->defs_count, sizeof(*parsed));
  CHECK(parsed != NULL);
  CHECK(ap_strvec_push(&parsed[1].values, ap_strdup("10")) == 0);
  LONGS_EQUAL(-1, ap_validate_args(fixed, parsed, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("coords", err.argument);
  STRCMP_EQUAL("argument 'coords' requires exactly 2 values", err.message);
  ap_strvec_free(&parsed[1].values);
  free(parsed);

  mode.choices.items = valid_modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(choices, "--mode", mode, &err));
  parsed =
      (ap_parsed_arg *)calloc((size_t)choices->defs_count, sizeof(*parsed));
  CHECK(parsed != NULL);
  parsed[1].seen = true;
  CHECK(ap_strvec_push(&parsed[1].values, ap_strdup("fast")) == 0);
  LONGS_EQUAL(0, ap_validate_args(choices, parsed, &err));
  ap_strvec_free(&parsed[1].values);
  free(parsed);

  ap_parser_free(one);
  ap_parser_free(plus);
  ap_parser_free(fixed);
  ap_parser_free(choices);
}

TEST(InternalNamespaceBuildCoversRequiredMissingDoubleEmptyAndBoolFallback) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *required = ap_parser_new("prog", "desc");
  ap_parser *optional_double = ap_parser_new("prog", "desc");
  ap_parser *fallback_bool = ap_parser_new("prog", "desc");
  ap_parsed_arg *parsed = NULL;
  ap_arg_options required_name = ap_arg_options_default();
  ap_arg_options threshold = ap_arg_options_default();
  ap_arg_options mystery = ap_arg_options_default();

  CHECK(required != NULL);
  CHECK(optional_double != NULL);
  CHECK(fallback_bool != NULL);

  required_name.required = true;
  LONGS_EQUAL(0, ap_add_argument(required, "--name", required_name, &err));
  parsed =
      (ap_parsed_arg *)calloc((size_t)required->defs_count, sizeof(*parsed));
  CHECK(parsed != NULL);
  LONGS_EQUAL(-1, ap_build_namespace(required, parsed, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("--name", err.argument);
  STRCMP_EQUAL("option '--name' is required", err.message);
  CHECK(ns == NULL);
  free(parsed);

  threshold.type = AP_TYPE_DOUBLE;
  LONGS_EQUAL(0,
              ap_add_argument(optional_double, "--threshold", threshold, &err));
  parsed = (ap_parsed_arg *)calloc((size_t)optional_double->defs_count,
                                   sizeof(*parsed));
  CHECK(parsed != NULL);
  LONGS_EQUAL(0, ap_build_namespace(optional_double, parsed, &ns, &err));
  CHECK(ns != NULL);
  LONGS_EQUAL(AP_NS_VALUE_DOUBLE, ns->entries[1].type);
  LONGS_EQUAL(0, ns->entries[1].count);
  CHECK(ns->entries[1].as.doubles == NULL);
  ap_namespace_free(ns);
  ns = NULL;
  free(parsed);

  LONGS_EQUAL(0, ap_add_argument(fallback_bool, "--mystery", mystery, &err));
  fallback_bool->defs[1].opts.type = (ap_type)99;
  parsed = (ap_parsed_arg *)calloc((size_t)fallback_bool->defs_count,
                                   sizeof(*parsed));
  CHECK(parsed != NULL);
  parsed[1].seen = true;
  CHECK(ap_strvec_push(&parsed[1].values, ap_strdup("present")) == 0);
  LONGS_EQUAL(0, ap_build_namespace(fallback_bool, parsed, &ns, &err));
  CHECK(ns != NULL);
  LONGS_EQUAL(AP_NS_VALUE_BOOL, ns->entries[1].type);
  LONGS_EQUAL(1, ns->entries[1].count);
  CHECK_TRUE(ns->entries[1].as.boolean);
  ap_namespace_free(ns);
  ap_strvec_free(&parsed[1].values);
  free(parsed);

  ap_parser_free(required);
  ap_parser_free(optional_double);
  ap_parser_free(fallback_bool);
}

TEST(HelpBypassesValidationInBuildNamespaceAndKeepsNamespaceState) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parsed_arg *parsed = NULL;
  ap_arg_options required_name = ap_arg_options_default();
  bool help = false;

  CHECK(p != NULL);
  required_name.required = true;
  LONGS_EQUAL(0, ap_add_argument(p, "--name", required_name, &err));

  parsed = (ap_parsed_arg *)calloc((size_t)p->defs_count, sizeof(*parsed));
  CHECK(parsed != NULL);

  parsed[0].seen = true; /* builtin --help */
  LONGS_EQUAL(0, ap_build_namespace(p, parsed, &ns, &err));
  CHECK(ns != NULL);
  CHECK(ap_ns_get_bool(ns, "help", &help));
  CHECK_TRUE(help);
  LONGS_EQUAL(0, ap_ns_get_count(ns, "name"));

  ap_namespace_free(ns);
  free(parsed);
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
  ap_arg_options flag = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--flag=yes", NULL};

  CHECK(p != NULL);
  flag.type = AP_TYPE_BOOL;
  flag.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "--flag", flag, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("--flag", err.argument);
  STRCMP_EQUAL("option '--flag' does not take a value", err.message);

  ap_parser_free(p);
}

TEST(CountRejectsInlineValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options flag = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--flag=yes", NULL};

  CHECK(p != NULL);
  flag.type = AP_TYPE_INT32;
  flag.action = AP_ACTION_COUNT;
  LONGS_EQUAL(0, ap_add_argument(p, "--flag", flag, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("--flag", err.argument);
  STRCMP_EQUAL("option '--flag' does not take a value", err.message);

  ap_parser_free(p);
}

TEST(StoreConstRejectsInlineValue) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options flag = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--flag=yes", NULL};

  CHECK(p != NULL);
  flag.action = AP_ACTION_STORE_CONST;
  flag.const_value = "const";
  LONGS_EQUAL(0, ap_add_argument(p, "--flag", flag, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("--flag", err.argument);
  STRCMP_EQUAL("option '--flag' does not take a value", err.message);

  ap_parser_free(p);
}

TEST(ShortOptionClusterRejectsValueOptionWithInvalidNargs) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options flag_a = ap_arg_options_default();
  ap_arg_options flag_b = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"-ab", NULL};

  CHECK(p != NULL);
  flag_a.type = AP_TYPE_BOOL;
  flag_a.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "-a", flag_a, &err));
  flag_b.nargs = AP_NARGS_ONE;
  LONGS_EQUAL(0, ap_add_argument(p, "-b", flag_b, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("option '-b' cannot be used in a short option cluster",
               err.message);

  ap_parser_free(p);
}

TEST(ParseKnownArgsFallsBackFromShortClusterToUnknownTokenBoundary) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options flag_a = ap_arg_options_default();
  ap_arg_options flag_b = ap_arg_options_default();
  char **unknown = NULL;
  int unknown_count = 0;
  const char *value_b = NULL;
  char *argv[] = {(char *)"prog", (char *)"-ab", (char *)"-b", (char *)"x",
                  NULL};

  CHECK(p != NULL);
  flag_a.type = AP_TYPE_BOOL;
  flag_a.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "-a", flag_a, &err));
  flag_b.nargs = AP_NARGS_ONE;
  LONGS_EQUAL(0, ap_add_argument(p, "-b", flag_b, &err));

  LONGS_EQUAL(
      0, ap_parse_known_args(p, 4, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(1, unknown_count);
  STRCMP_EQUAL("-ab", unknown[0]);
  CHECK(ap_ns_get_string(ns, "b", &value_b));
  STRCMP_EQUAL("x", value_b);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
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

TEST(ParseArgsDistributesLongMixedPositionalOptionalSequence) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options tail = ap_arg_options_default();
  const char *maybe_value = NULL;
  const char *tail_value = NULL;
  char *argv[] = {
      (char *)"prog", (char *)"a", (char *)"b",        (char *)"--maybe",
      (char *)"seen", (char *)"c", (char *)"tail.txt", NULL};

  CHECK(p != NULL);
  maybe.nargs = AP_NARGS_OPTIONAL;
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "tail", tail, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 7, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "maybe", &maybe_value));
  STRCMP_EQUAL("seen", maybe_value);
  LONGS_EQUAL(3, ap_ns_get_count(ns, "extras"));
  STRCMP_EQUAL("a", ap_ns_get_string_at(ns, "extras", 0));
  STRCMP_EQUAL("b", ap_ns_get_string_at(ns, "extras", 1));
  STRCMP_EQUAL("c", ap_ns_get_string_at(ns, "extras", 2));
  CHECK(ap_ns_get_string(ns, "tail", &tail_value));
  STRCMP_EQUAL("tail.txt", tail_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsReportsUnexpectedPositionalAfterDoubleDashBoundary) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options target = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--", (char *)"target.txt",
                  (char *)"extra.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 4, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("extra.txt", err.argument);
  STRCMP_EQUAL("unexpected positional argument 'extra.txt'", err.message);

  ap_parser_free(p);
}

TEST(ParseArgsPreservesAppendCountAndPositionalOrderAcrossVeryLongSequence) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options tag = ap_arg_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options files = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  int32_t verbose_count = 0;
  const char *target_value = NULL;
  char *argv[] = {(char *)"prog",     (char *)"--tag",
                  (char *)"alpha",    (char *)"-v",
                  (char *)"file-01",  (char *)"--tag",
                  (char *)"beta",     (char *)"file-02",
                  (char *)"-v",       (char *)"file-03",
                  (char *)"--tag",    (char *)"gamma",
                  (char *)"file-04",  (char *)"-v",
                  (char *)"file-05",  (char *)"file-06",
                  (char *)"--tag",    (char *)"delta",
                  (char *)"file-07",  (char *)"-v",
                  (char *)"dest.out", NULL};

  CHECK(p != NULL);
  tag.action = AP_ACTION_APPEND;
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  files.nargs = AP_NARGS_ZERO_OR_MORE;
  files.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "--tag", tag, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "files", files, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 21, argv, &ns, &err));
  LONGS_EQUAL(4, ap_ns_get_count(ns, "tag"));
  STRCMP_EQUAL("alpha", ap_ns_get_string_at(ns, "tag", 0));
  STRCMP_EQUAL("beta", ap_ns_get_string_at(ns, "tag", 1));
  STRCMP_EQUAL("gamma", ap_ns_get_string_at(ns, "tag", 2));
  STRCMP_EQUAL("delta", ap_ns_get_string_at(ns, "tag", 3));
  CHECK(ap_ns_get_int32(ns, "verbose", &verbose_count));
  LONGS_EQUAL(4, verbose_count);
  LONGS_EQUAL(7, ap_ns_get_count(ns, "files"));
  STRCMP_EQUAL("file-01", ap_ns_get_string_at(ns, "files", 0));
  STRCMP_EQUAL("file-02", ap_ns_get_string_at(ns, "files", 1));
  STRCMP_EQUAL("file-03", ap_ns_get_string_at(ns, "files", 2));
  STRCMP_EQUAL("file-04", ap_ns_get_string_at(ns, "files", 3));
  STRCMP_EQUAL("file-05", ap_ns_get_string_at(ns, "files", 4));
  STRCMP_EQUAL("file-06", ap_ns_get_string_at(ns, "files", 5));
  STRCMP_EQUAL("file-07", ap_ns_get_string_at(ns, "files", 6));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest.out", target_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsKeepsRequiredPositionalsStableInLongMixedSequence) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  const char *maybe_value = NULL;
  const char *target_value = NULL;
  char *argv[] = {(char *)"prog",
                  (char *)"extra-01",
                  (char *)"extra-02",
                  (char *)"extra-03",
                  (char *)"extra-04",
                  (char *)"--maybe",
                  (char *)"maybe.txt",
                  (char *)"left.bin",
                  (char *)"right.bin",
                  (char *)"dest.out",
                  NULL};

  CHECK(p != NULL);
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  maybe.nargs = AP_NARGS_OPTIONAL;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 10, argv, &ns, &err));
  LONGS_EQUAL(4, ap_ns_get_count(ns, "extras"));
  STRCMP_EQUAL("extra-01", ap_ns_get_string_at(ns, "extras", 0));
  STRCMP_EQUAL("extra-02", ap_ns_get_string_at(ns, "extras", 1));
  STRCMP_EQUAL("extra-03", ap_ns_get_string_at(ns, "extras", 2));
  STRCMP_EQUAL("extra-04", ap_ns_get_string_at(ns, "extras", 3));
  CHECK(ap_ns_get_string(ns, "maybe", &maybe_value));
  STRCMP_EQUAL("maybe.txt", maybe_value);
  LONGS_EQUAL(2, ap_ns_get_count(ns, "pair"));
  STRCMP_EQUAL("left.bin", ap_ns_get_string_at(ns, "pair", 0));
  STRCMP_EQUAL("right.bin", ap_ns_get_string_at(ns, "pair", 1));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest.out", target_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsAllowsOnlyExpectedPositionalsAfterDoubleDashBoundary) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  const char *target_value = NULL;
  char *argv_ok[] = {(char *)"prog", (char *)"--", (char *)"target.txt", NULL};
  char *argv_extra[] = {(char *)"prog",       (char *)"--",
                        (char *)"target.txt", (char *)"extra.txt",
                        (char *)"extra2.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "target", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv_ok, &ns, &err));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("target.txt", target_value);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(-1, ap_parse_args(p, 5, argv_extra, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("extra.txt", err.argument);
  STRCMP_EQUAL("unexpected positional argument 'extra.txt'", err.message);

  ap_parser_free(p);
}

TEST(
    ParseArgsDoubleDashTreatsDashPrefixedTokensAsPositionalsInMixedNargsLayout) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  const char *target_value = NULL;
  char *argv[] = {(char *)"prog",
                  (char *)"--",
                  (char *)"--as-data",
                  (char *)"left",
                  (char *)"right",
                  (char *)"dest.out",
                  NULL};

  CHECK(p != NULL);
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 6, argv, &ns, &err));
  LONGS_EQUAL(1, ap_ns_get_count(ns, "extras"));
  STRCMP_EQUAL("--as-data", ap_ns_get_string_at(ns, "extras", 0));
  LONGS_EQUAL(2, ap_ns_get_count(ns, "pair"));
  STRCMP_EQUAL("left", ap_ns_get_string_at(ns, "pair", 0));
  STRCMP_EQUAL("right", ap_ns_get_string_at(ns, "pair", 1));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest.out", target_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsHandlesLongMixedSequenceWithOptionalPositionalAndFixedTail) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options prefix = ap_arg_options_default();
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  const char *maybe_value = NULL;
  const char *target_value = NULL;
  char *argv[] = {(char *)"prog",     (char *)"lead-01",   (char *)"lead-02",
                  (char *)"lead-03",  (char *)"lead-04",   (char *)"lead-05",
                  (char *)"--maybe",  (char *)"maybe.txt", (char *)"left-01",
                  (char *)"right-01", (char *)"dest.out",  NULL};

  CHECK(p != NULL);
  prefix.nargs = AP_NARGS_ZERO_OR_MORE;
  prefix.action = AP_ACTION_APPEND;
  maybe.nargs = AP_NARGS_OPTIONAL;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "prefix", prefix, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 11, argv, &ns, &err));
  LONGS_EQUAL(5, ap_ns_get_count(ns, "prefix"));
  STRCMP_EQUAL("lead-01", ap_ns_get_string_at(ns, "prefix", 0));
  STRCMP_EQUAL("lead-02", ap_ns_get_string_at(ns, "prefix", 1));
  STRCMP_EQUAL("lead-03", ap_ns_get_string_at(ns, "prefix", 2));
  STRCMP_EQUAL("lead-04", ap_ns_get_string_at(ns, "prefix", 3));
  STRCMP_EQUAL("lead-05", ap_ns_get_string_at(ns, "prefix", 4));
  CHECK(ap_ns_get_string(ns, "maybe", &maybe_value));
  STRCMP_EQUAL("maybe.txt", maybe_value);
  LONGS_EQUAL(2, ap_ns_get_count(ns, "pair"));
  STRCMP_EQUAL("left-01", ap_ns_get_string_at(ns, "pair", 0));
  STRCMP_EQUAL("right-01", ap_ns_get_string_at(ns, "pair", 1));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest.out", target_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsReportsUnexpectedPositionalAtFirstOverflowAfterDoubleDash) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options first = ap_arg_options_default();
  ap_arg_options second = ap_arg_options_default();
  char *argv[] = {(char *)"prog",
                  (char *)"--",
                  (char *)"first.txt",
                  (char *)"second.txt",
                  (char *)"overflow-01",
                  (char *)"overflow-02",
                  NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "first", first, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "second", second, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 6, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("overflow-01", err.argument);
  STRCMP_EQUAL("unexpected positional argument 'overflow-01'", err.message);

  ap_parser_free(p);
}

TEST(ParseArgsLongAlternatingOptionalAndPositionalHitsUnexpectedBoundary) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options opt = ap_arg_options_default();
  ap_arg_options extras = ap_arg_options_default();
  const char *input_value = NULL;
  int32_t verbose_count = 0;
  std::vector<std::string> unknown_like_tokens;
  std::vector<char *> argv_ok;
  const int alternating_pairs = 12;

  CHECK(p != NULL);
  opt.type = AP_TYPE_INT32;
  opt.action = AP_ACTION_COUNT;
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", opt, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "input", ap_arg_options_default(), &err));

  argv_ok.push_back((char *)"prog");
  unknown_like_tokens.reserve((size_t)alternating_pairs);
  for (int i = 0; i < alternating_pairs; ++i) {
    argv_ok.push_back((char *)"-v");
    unknown_like_tokens.push_back("unknown-like-" + std::to_string(i));
    argv_ok.push_back((char *)unknown_like_tokens.back().c_str());
  }
  argv_ok.push_back((char *)"--");
  argv_ok.push_back((char *)"target.txt");

  LONGS_EQUAL(0,
              ap_parse_args(p, (int)argv_ok.size(), argv_ok.data(), &ns, &err));
  CHECK(ap_ns_get_int32(ns, "verbose", &verbose_count));
  LONGS_EQUAL(alternating_pairs, verbose_count);
  LONGS_EQUAL(alternating_pairs, ap_ns_get_count(ns, "extras"));
  for (int i = 0; i < alternating_pairs; ++i) {
    STRCMP_EQUAL(unknown_like_tokens[(size_t)i].c_str(),
                 ap_ns_get_string_at(ns, "extras", i));
  }
  CHECK(ap_ns_get_string(ns, "input", &input_value));
  STRCMP_EQUAL("target.txt", input_value);
  ap_namespace_free(ns);
  ns = NULL;
  ap_parser_free(p);

  p = ap_parser_new("prog", "desc");
  std::vector<char *> argv_strict_ok;
  std::vector<char *> argv_strict_fail;
  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", opt, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "input", ap_arg_options_default(), &err));

  argv_strict_ok.push_back((char *)"prog");
  argv_strict_ok.push_back((char *)"-v");
  argv_strict_ok.push_back((char *)unknown_like_tokens[0].c_str());
  LONGS_EQUAL(0, ap_parse_args(p, (int)argv_strict_ok.size(),
                               argv_strict_ok.data(), &ns, &err));
  ap_namespace_free(ns);
  ns = NULL;

  argv_strict_fail.push_back((char *)"prog");
  argv_strict_fail.push_back((char *)"-v");
  argv_strict_fail.push_back((char *)unknown_like_tokens[0].c_str());
  argv_strict_fail.push_back((char *)"-v");
  argv_strict_fail.push_back((char *)unknown_like_tokens[1].c_str());
  LONGS_EQUAL(-1, ap_parse_args(p, (int)argv_strict_fail.size(),
                                argv_strict_fail.data(), &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("unknown-like-1", err.argument);
  STRCMP_EQUAL("unexpected positional argument 'unknown-like-1'", err.message);

  ap_parser_free(p);
}

TEST(ParseArgsLongZeroOrMorePositionalReservesRequiredTailPositionals) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options extras = ap_arg_options_default();
  const char *mid_value = NULL;
  const char *output_value = NULL;
  std::vector<std::string> extra_values;
  std::vector<char *> argv;
  const int extra_count = 40;

  CHECK(p != NULL);
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "mid", ap_arg_options_default(), &err));
  LONGS_EQUAL(0, ap_add_argument(p, "output", ap_arg_options_default(), &err));

  argv.push_back((char *)"prog");
  extra_values.reserve((size_t)extra_count);
  for (int i = 0; i < extra_count; ++i) {
    extra_values.push_back("extra-" + std::to_string(i));
    argv.push_back((char *)extra_values.back().c_str());
  }
  argv.push_back((char *)"mid.bin");
  argv.push_back((char *)"dest.out");

  LONGS_EQUAL(0, ap_parse_args(p, (int)argv.size(), argv.data(), &ns, &err));
  LONGS_EQUAL(extra_count, ap_ns_get_count(ns, "extras"));
  for (int i = 0; i < extra_count; ++i) {
    STRCMP_EQUAL(extra_values[(size_t)i].c_str(),
                 ap_ns_get_string_at(ns, "extras", i));
  }
  CHECK(ap_ns_get_string(ns, "mid", &mid_value));
  STRCMP_EQUAL("mid.bin", mid_value);
  CHECK(ap_ns_get_string(ns, "output", &output_value));
  STRCMP_EQUAL("dest.out", output_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsTracksAppendCountAndPositionalsAcrossLongSequence) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options tag = ap_arg_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options files = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  int32_t verbose_count = 0;
  const char *target_value = NULL;
  char *argv[] = {(char *)"prog",
                  (char *)"--tag",
                  (char *)"alpha",
                  (char *)"-v",
                  (char *)"f1",
                  (char *)"--tag",
                  (char *)"beta",
                  (char *)"-v",
                  (char *)"f2",
                  (char *)"dest",
                  NULL};

  CHECK(p != NULL);
  tag.action = AP_ACTION_APPEND;
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  files.nargs = AP_NARGS_ZERO_OR_MORE;
  files.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "--tag", tag, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "files", files, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 10, argv, &ns, &err));
  LONGS_EQUAL(2, ap_ns_get_count(ns, "tag"));
  STRCMP_EQUAL("alpha", ap_ns_get_string_at(ns, "tag", 0));
  STRCMP_EQUAL("beta", ap_ns_get_string_at(ns, "tag", 1));
  CHECK(ap_ns_get_int32(ns, "verbose", &verbose_count));
  LONGS_EQUAL(2, verbose_count);
  LONGS_EQUAL(2, ap_ns_get_count(ns, "files"));
  STRCMP_EQUAL("f1", ap_ns_get_string_at(ns, "files", 0));
  STRCMP_EQUAL("f2", ap_ns_get_string_at(ns, "files", 1));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest", target_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsScalesRepeatedAppendAndCountAcrossLongInputVector) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options tag = ap_arg_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  int32_t verbose_count = 0;
  const int repeat = 40;
  std::vector<std::string> tag_values;
  std::vector<char *> argv;

  CHECK(p != NULL);
  tag.action = AP_ACTION_APPEND;
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  LONGS_EQUAL(0, ap_add_argument(p, "--tag", tag, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "input", input, &err));

  argv.push_back((char *)"prog");
  tag_values.reserve(repeat);
  for (int i = 0; i < repeat; ++i) {
    tag_values.push_back("tag-" + std::to_string(i));
    argv.push_back((char *)"--tag");
    argv.push_back((char *)tag_values.back().c_str());
    if ((i % 2) == 0) {
      argv.push_back((char *)"-v");
    }
  }
  argv.push_back((char *)"input.txt");

  LONGS_EQUAL(0, ap_parse_args(p, (int)argv.size(), argv.data(), &ns, &err));
  LONGS_EQUAL(repeat, ap_ns_get_count(ns, "tag"));
  for (int i = 0; i < repeat; ++i) {
    std::string expected = "tag-" + std::to_string(i);
    STRCMP_EQUAL(expected.c_str(), ap_ns_get_string_at(ns, "tag", i));
  }
  CHECK(ap_ns_get_int32(ns, "verbose", &verbose_count));
  LONGS_EQUAL(20, verbose_count);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsDistributesLongMixedNargsStarOptionalAndFixedPositionals) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  const int extra_count = 15;
  std::vector<std::string> extra_values;
  std::vector<char *> argv;
  const char *maybe_value = NULL;
  const char *target_value = NULL;

  CHECK(p != NULL);
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  maybe.nargs = AP_NARGS_OPTIONAL;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  argv.push_back((char *)"prog");
  extra_values.reserve(extra_count);
  for (int i = 0; i < extra_count; ++i) {
    extra_values.push_back("extra-" + std::to_string(i));
    argv.push_back((char *)extra_values.back().c_str());
  }
  argv.push_back((char *)"--maybe");
  argv.push_back((char *)"seen.txt");
  argv.push_back((char *)"left.bin");
  argv.push_back((char *)"right.bin");
  argv.push_back((char *)"dest.out");

  LONGS_EQUAL(0, ap_parse_args(p, (int)argv.size(), argv.data(), &ns, &err));
  LONGS_EQUAL(extra_count, ap_ns_get_count(ns, "extras"));
  for (int i = 0; i < extra_count; ++i) {
    std::string expected = "extra-" + std::to_string(i);
    STRCMP_EQUAL(expected.c_str(), ap_ns_get_string_at(ns, "extras", i));
  }
  CHECK(ap_ns_get_string(ns, "maybe", &maybe_value));
  STRCMP_EQUAL("seen.txt", maybe_value);
  LONGS_EQUAL(2, ap_ns_get_count(ns, "pair"));
  STRCMP_EQUAL("left.bin", ap_ns_get_string_at(ns, "pair", 0));
  STRCMP_EQUAL("right.bin", ap_ns_get_string_at(ns, "pair", 1));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest.out", target_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsLongMixedDistributionAndErrorBoundaries) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  ap_parser *q = NULL;
  ap_arg_options q_maybe = ap_arg_options_default();
  ap_arg_options q_pair = ap_arg_options_default();
  ap_arg_options q_target = ap_arg_options_default();
  const int extra_count = 12;
  std::vector<std::string> extra_values;
  std::vector<char *> argv_ok;
  char *argv_shortage[] = {(char *)"prog", (char *)"--maybe",
                           (char *)"seen.txt", (char *)"left.bin", NULL};
  char *argv_overflow[] = {(char *)"prog",      (char *)"--maybe",
                           (char *)"seen.txt",  (char *)"left.bin",
                           (char *)"right.bin", (char *)"dest.out",
                           (char *)"overflow",  NULL};
  char *argv_after_dd[] = {(char *)"prog",
                           (char *)"--",
                           (char *)"left.bin",
                           (char *)"right.bin",
                           (char *)"dest.out",
                           (char *)"--after-0",
                           NULL};
  const char *maybe_value = NULL;
  const char *target_value = NULL;

  CHECK(p != NULL);
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  extras.action = AP_ACTION_APPEND;
  maybe.nargs = AP_NARGS_OPTIONAL;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  argv_ok.push_back((char *)"prog");
  extra_values.reserve(extra_count);
  for (int i = 0; i < extra_count; ++i) {
    extra_values.push_back("extra-" + std::to_string(i));
    argv_ok.push_back((char *)extra_values.back().c_str());
  }
  argv_ok.push_back((char *)"--maybe");
  argv_ok.push_back((char *)"seen.txt");
  argv_ok.push_back((char *)"left.bin");
  argv_ok.push_back((char *)"right.bin");
  argv_ok.push_back((char *)"dest.out");

  LONGS_EQUAL(0,
              ap_parse_args(p, (int)argv_ok.size(), argv_ok.data(), &ns, &err));
  LONGS_EQUAL(extra_count, ap_ns_get_count(ns, "extras"));
  for (int i = 0; i < extra_count; ++i) {
    std::string expected = "extra-" + std::to_string(i);
    STRCMP_EQUAL(expected.c_str(), ap_ns_get_string_at(ns, "extras", i));
  }
  CHECK(ap_ns_get_string(ns, "maybe", &maybe_value));
  STRCMP_EQUAL("seen.txt", maybe_value);
  LONGS_EQUAL(2, ap_ns_get_count(ns, "pair"));
  STRCMP_EQUAL("left.bin", ap_ns_get_string_at(ns, "pair", 0));
  STRCMP_EQUAL("right.bin", ap_ns_get_string_at(ns, "pair", 1));
  CHECK(ap_ns_get_string(ns, "target", &target_value));
  STRCMP_EQUAL("dest.out", target_value);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(-1, ap_parse_args(p, 4, argv_shortage, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_NARGS, err.code);
  STRCMP_EQUAL("pair", err.argument);
  STRCMP_EQUAL("argument 'pair' requires exactly 2 values", err.message);

  q = ap_parser_new("prog", "desc");
  CHECK(q != NULL);
  q_maybe.nargs = AP_NARGS_OPTIONAL;
  q_pair.nargs = AP_NARGS_FIXED;
  q_pair.nargs_count = 2;
  LONGS_EQUAL(0, ap_add_argument(q, "--maybe", q_maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(q, "pair", q_pair, &err));
  LONGS_EQUAL(0, ap_add_argument(q, "target", q_target, &err));

  LONGS_EQUAL(-1, ap_parse_args(q, 7, argv_overflow, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("overflow", err.argument);
  STRCMP_EQUAL("unexpected positional argument 'overflow'", err.message);

  LONGS_EQUAL(-1, ap_parse_args(q, 6, argv_after_dd, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("--after-0", err.argument);
  STRCMP_EQUAL("unexpected positional argument '--after-0'", err.message);

  ap_parser_free(q);
  ap_parser_free(p);
}

TEST(ParseArgsReportsFirstOverflowTokenFromLongDoubleDashTail) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  std::vector<std::string> tail_tokens;
  std::vector<char *> argv;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "target", ap_arg_options_default(), &err));

  argv.push_back((char *)"prog");
  argv.push_back((char *)"--");
  argv.push_back((char *)"target.txt");
  tail_tokens.reserve(18);
  for (int i = 0; i < 18; ++i) {
    tail_tokens.push_back("--tail-" + std::to_string(i));
    argv.push_back((char *)tail_tokens.back().c_str());
  }

  LONGS_EQUAL(-1, ap_parse_args(p, (int)argv.size(), argv.data(), &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("--tail-0", err.argument);
  STRCMP_EQUAL("unexpected positional argument '--tail-0'", err.message);

  ap_parser_free(p);
}

TEST(ParseInt64AndDoubleValues) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options big = ap_arg_options_default();
  ap_arg_options ratio = ap_arg_options_default();
  int64_t big_value = 0;
  double ratio_value = 0.0;
  char *argv[] = {
      (char *)"prog",    (char *)"--big", (char *)"9223372036854775806",
      (char *)"--ratio", (char *)"3.25",  NULL};

  CHECK(p != NULL);
  big.type = AP_TYPE_INT64;
  ratio.type = AP_TYPE_DOUBLE;
  LONGS_EQUAL(0, ap_add_argument(p, "--big", big, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--ratio", ratio, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 5, argv, &ns, &err));
  CHECK(ap_ns_get_int64(ns, "big", &big_value));
  CHECK(ap_ns_get_double(ns, "ratio", &ratio_value));
  LONGS_EQUAL(9223372036854775806LL, big_value);
  CHECK(std::fabs(ratio_value - 3.25) < 1e-12);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(InvalidInt64RangeAndInvalidDouble) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options big = ap_arg_options_default();
  ap_arg_options ratio = ap_arg_options_default();
  char *argv_big[] = {(char *)"prog", (char *)"--big",
                      (char *)"9223372036854775808", NULL};
  char *argv_ratio[] = {(char *)"prog", (char *)"--ratio", (char *)"1.2.3",
                        NULL};

  CHECK(p != NULL);
  big.type = AP_TYPE_INT64;
  ratio.type = AP_TYPE_DOUBLE;
  LONGS_EQUAL(0, ap_add_argument(p, "--big", big, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--ratio", ratio, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_big, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_INT64, err.code);
  STRCMP_EQUAL("--big", err.argument);
  STRCMP_EQUAL("argument '--big' must be a valid int64: '9223372036854775808'",
               err.message);

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_ratio, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DOUBLE, err.code);
  STRCMP_EQUAL("--ratio", err.argument);
  STRCMP_EQUAL("argument '--ratio' must be a valid double: '1.2.3'",
               err.message);

  ap_parser_free(p);
}

TEST(DefaultChoicesAndAppendForInt64AndDouble) {
  static const char *level_choices[] = {"1.5", "2.5"};
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options limit = ap_arg_options_default();
  ap_arg_options level = ap_arg_options_default();
  ap_arg_options ids = ap_arg_options_default();
  int64_t limit_value = 0;
  double level_value = 0.0;
  int64_t id0 = 0;
  int64_t id1 = 0;
  char *argv[] = {(char *)"prog",  (char *)"--ids", (char *)"10",
                  (char *)"--ids", (char *)"20",    NULL};

  CHECK(p != NULL);
  limit.type = AP_TYPE_INT64;
  limit.default_value = "42";
  level.type = AP_TYPE_DOUBLE;
  level.default_value = "2.5";
  level.choices.items = level_choices;
  level.choices.count = 2;
  ids.type = AP_TYPE_INT64;
  ids.action = AP_ACTION_APPEND;

  LONGS_EQUAL(0, ap_add_argument(p, "--limit", limit, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--level", level, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--ids", ids, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 5, argv, &ns, &err));
  CHECK(ap_ns_get_int64(ns, "limit", &limit_value));
  CHECK(ap_ns_get_double(ns, "level", &level_value));
  CHECK(ap_ns_get_int64_at(ns, "ids", 0, &id0));
  CHECK(ap_ns_get_int64_at(ns, "ids", 1, &id1));
  LONGS_EQUAL(42LL, limit_value);
  CHECK(std::fabs(level_value - 2.5) < 1e-12);
  LONGS_EQUAL(2, ap_ns_get_count(ns, "ids"));
  LONGS_EQUAL(10LL, id0);
  LONGS_EQUAL(20LL, id1);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseUint64ValuesAndRejectNegativeInput) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options size = ap_arg_options_default();
  uint64_t size_value = 0;
  char *argv_ok[] = {(char *)"prog", (char *)"--size",
                     (char *)"18446744073709551615", NULL};
  char *argv_bad[] = {(char *)"prog", (char *)"--size", (char *)"-1", NULL};

  CHECK(p != NULL);
  size.type = AP_TYPE_UINT64;
  LONGS_EQUAL(0, ap_add_argument(p, "--size", size, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv_ok, &ns, &err));
  CHECK(ap_ns_get_uint64(ns, "size", &size_value));
  LONGS_EQUAL(18446744073709551615ULL, size_value);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_bad, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_UINT64, err.code);
  STRCMP_EQUAL("--size", err.argument);
  STRCMP_EQUAL("argument '--size' must be a valid uint64: '-1'", err.message);

  ap_parser_free(p);
}

TEST(ParseUint64RejectsOverflowInput) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options size = ap_arg_options_default();
  char *argv_overflow[] = {(char *)"prog", (char *)"--size",
                           (char *)"18446744073709551616", NULL};

  CHECK(p != NULL);
  size.type = AP_TYPE_UINT64;
  LONGS_EQUAL(0, ap_add_argument(p, "--size", size, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_overflow, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_UINT64, err.code);
  STRCMP_EQUAL("--size", err.argument);
  STRCMP_EQUAL(
      "argument '--size' must be a valid uint64: '18446744073709551616'",
      err.message);

  ap_parser_free(p);
}

TEST(NamespaceUint64GetterContractCoversSingleAndAppendValues) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options zero = ap_arg_options_default();
  ap_arg_options max = ap_arg_options_default();
  ap_arg_options mismatch = ap_arg_options_default();
  ap_arg_options values = ap_arg_options_default();
  uint64_t value_out = 0;
  uint64_t append0 = 0;
  uint64_t append1 = 0;
  int32_t mismatch_value = 0;
  char *argv[] = {
      (char *)"prog",
      (char *)"--zero",
      (char *)"0",
      (char *)"--max",
      (char *)"18446744073709551615",
      (char *)"--mismatch",
      (char *)"9",
      (char *)"--values",
      (char *)"0",
      (char *)"--values",
      (char *)"18446744073709551615",
      NULL,
  };
  char *argv_unset[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  zero.type = AP_TYPE_UINT64;
  max.type = AP_TYPE_UINT64;
  mismatch.type = AP_TYPE_INT32;
  values.type = AP_TYPE_UINT64;
  values.action = AP_ACTION_APPEND;

  LONGS_EQUAL(0, ap_add_argument(p, "--zero", zero, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--max", max, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--mismatch", mismatch, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--values", values, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 11, argv, &ns, &err));
  CHECK(ap_ns_get_uint64(ns, "zero", &value_out));
  LONGS_EQUAL(0ULL, value_out);
  CHECK(ap_ns_get_uint64(ns, "max", &value_out));
  LONGS_EQUAL(18446744073709551615ULL, value_out);
  CHECK(!ap_ns_get_uint64(ns, "mismatch", &value_out));
  CHECK(ap_ns_get_int32(ns, "mismatch", &mismatch_value));
  LONGS_EQUAL(9, mismatch_value);
  CHECK(ap_ns_get_uint64(ns, "values", &value_out));
  CHECK(ap_ns_get_uint64_at(ns, "values", 0, &append0));
  CHECK(ap_ns_get_uint64_at(ns, "values", 1, &append1));
  LONGS_EQUAL(append0, value_out);
  LONGS_EQUAL(0ULL, append0);
  LONGS_EQUAL(18446744073709551615ULL, append1);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(0, ap_parse_args(p, 1, argv_unset, &ns, &err));
  CHECK(!ap_ns_get_uint64(ns, "zero", &value_out));
  CHECK(!ap_ns_get_uint64(ns, "max", &value_out));
  CHECK(!ap_ns_get_uint64(ns, "values", &value_out));
  CHECK(!ap_ns_get_uint64_at(ns, "values", 0, &append0));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseInt32RangeBoundariesAndRejectsOverflowInput) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options value = ap_arg_options_default();
  int32_t parsed = 0;
  char *argv_min[] = {(char *)"prog", (char *)"--value", (char *)"-2147483648",
                      NULL};
  char *argv_max[] = {(char *)"prog", (char *)"--value", (char *)"2147483647",
                      NULL};
  char *argv_overflow[] = {(char *)"prog", (char *)"--value",
                           (char *)"2147483648", NULL};

  CHECK(p != NULL);
  value.type = AP_TYPE_INT32;
  LONGS_EQUAL(0, ap_add_argument(p, "--value", value, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv_min, &ns, &err));
  CHECK(ap_ns_get_int32(ns, "value", &parsed));
  LONGS_EQUAL(-2147483647 - 1, parsed);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(0, ap_parse_args(p, 3, argv_max, &ns, &err));
  CHECK(ap_ns_get_int32(ns, "value", &parsed));
  LONGS_EQUAL(2147483647, parsed);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_overflow, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_INT32, err.code);
  STRCMP_EQUAL("--value", err.argument);
  STRCMP_EQUAL("argument '--value' must be a valid int32: '2147483648'",
               err.message);

  ap_parser_free(p);
}

TEST(ParseDoubleRejectsOverflowInput) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options ratio = ap_arg_options_default();
  char *argv_overflow[] = {(char *)"prog", (char *)"--ratio", (char *)"1e309",
                           NULL};

  CHECK(p != NULL);
  ratio.type = AP_TYPE_DOUBLE;
  LONGS_EQUAL(0, ap_add_argument(p, "--ratio", ratio, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_overflow, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DOUBLE, err.code);
  STRCMP_EQUAL("--ratio", err.argument);
  STRCMP_EQUAL("argument '--ratio' must be a valid double: '1e309'",
               err.message);

  ap_parser_free(p);
}

TEST(InvalidNumericFormatsBurstAcrossTypesKeepsFailureScoped) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options i32 = ap_arg_options_default();
  ap_arg_options i64 = ap_arg_options_default();
  ap_arg_options u64 = ap_arg_options_default();
  ap_arg_options dbl = ap_arg_options_default();
  char *argv_i32[] = {(char *)"prog", (char *)"--i32", (char *)"12x", NULL};
  char *argv_i64[] = {(char *)"prog", (char *)"--i64", (char *)"--", NULL};
  char *argv_u64[] = {(char *)"prog", (char *)"--u64", (char *)"-10", NULL};
  char *argv_dbl[] = {(char *)"prog", (char *)"--dbl", (char *)"1.2.3", NULL};

  CHECK(p != NULL);
  i32.type = AP_TYPE_INT32;
  i64.type = AP_TYPE_INT64;
  u64.type = AP_TYPE_UINT64;
  dbl.type = AP_TYPE_DOUBLE;
  LONGS_EQUAL(0, ap_add_argument(p, "--i32", i32, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--i64", i64, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--u64", u64, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--dbl", dbl, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_i32, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_INT32, err.code);
  STRCMP_EQUAL("--i32", err.argument);
  CHECK(ns == NULL);

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_i64, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_INT64, err.code);
  STRCMP_EQUAL("--i64", err.argument);
  CHECK(ns == NULL);

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_u64, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_UINT64, err.code);
  STRCMP_EQUAL("--u64", err.argument);
  CHECK(ns == NULL);

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv_dbl, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DOUBLE, err.code);
  STRCMP_EQUAL("--dbl", err.argument);
  CHECK(ns == NULL);

  ap_parser_free(p);
}

TEST(DeepSubcommandDoubleDashBoundaryRejectsTailPositionals) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *l1 = NULL;
  ap_parser *l2 = NULL;
  ap_parser *l3 = NULL;
  ap_arg_options leaf_value = ap_arg_options_default();
  char *argv[] = {(char *)"prog",  (char *)"alpha",    (char *)"beta",
                  (char *)"gamma", (char *)"--leaf",   (char *)"ok",
                  (char *)"--",    (char *)"dangling", NULL};

  CHECK(p != NULL);
  l1 = ap_add_subcommand(p, "alpha", "level1", &err);
  CHECK(l1 != NULL);
  l2 = ap_add_subcommand(l1, "beta", "level2", &err);
  CHECK(l2 != NULL);
  l3 = ap_add_subcommand(l2, "gamma", "level3", &err);
  CHECK(l3 != NULL);
  LONGS_EQUAL(0, ap_add_argument(l3, "--leaf", leaf_value, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 8, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_UNEXPECTED_POSITIONAL, err.code);
  STRCMP_EQUAL("dangling", err.argument);
  CHECK(ns == NULL);

  ap_parser_free(p);
}

TEST(ParseIntermixedAliasParsesMixedOrder) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options count = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"a.txt", (char *)"--count",
                  (char *)"2",    (char *)"b.txt", NULL};
  const char *first = NULL;
  const char *second = NULL;
  int32_t parsed_count = 0;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "src", ap_arg_options_default(), &err));
  LONGS_EQUAL(0, ap_add_argument(p, "dst", ap_arg_options_default(), &err));
  count.type = AP_TYPE_INT32;
  LONGS_EQUAL(0, ap_add_argument(p, "--count", count, &err));

  LONGS_EQUAL(0, ap_parse_intermixed_args(p, 5, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "src", &first));
  CHECK(ap_ns_get_string(ns, "dst", &second));
  CHECK(ap_ns_get_int32(ns, "count", &parsed_count));
  STRCMP_EQUAL("a.txt", first);
  STRCMP_EQUAL("b.txt", second);
  LONGS_EQUAL(2, parsed_count);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(CustomPrefixCharsAndAbbrevResolveLongOption) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser_options options = ap_parser_options_default();
  ap_parser *p = NULL;
  ap_arg_options value = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"++val", (char *)"123", NULL};
  int32_t parsed = 0;

  options.prefix_chars = "+";
  options.allow_abbrev = true;
  p = ap_parser_new_with_options("prog", "desc", options);
  CHECK(p != NULL);

  value.type = AP_TYPE_INT32;
  value.dest = "value";
  LONGS_EQUAL(0, ap_add_argument(p, "++value", value, &err));
  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  CHECK(ap_ns_get_int32(ns, "value", &parsed));
  LONGS_EQUAL(123, parsed);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(FromfilePrefixExpandsArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser_options options = ap_parser_options_default();
  ap_parser *p = NULL;
  ap_arg_options num = ap_arg_options_default();
  const char *path = "/tmp/argparse_c_fromfile_args.txt";
  FILE *fp = fopen(path, "w");
  char *argv[] = {(char *)"prog", (char *)"@/tmp/argparse_c_fromfile_args.txt",
                  NULL};
  int32_t parsed = 0;

  CHECK(fp != NULL);
  fputs("--num 42", fp);
  fclose(fp);

  options.fromfile_prefix_chars = "@";
  p = ap_parser_new_with_options("prog", "desc", options);
  CHECK(p != NULL);
  num.type = AP_TYPE_INT32;
  LONGS_EQUAL(0, ap_add_argument(p, "--num", num, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_int32(ns, "num", &parsed));
  LONGS_EQUAL(42, parsed);

  ap_namespace_free(ns);
  ap_parser_free(p);
  remove(path);
}

TEST(FromfilePrefixSkipsCommentsAndPreservesInlineArgs) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser_options options = ap_parser_options_default();
  ap_parser *p = NULL;
  ap_arg_options num = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  const char *mode_choices[] = {"fast", "slow"};
  const char *path = "/tmp/argparse_c_fromfile_comments.txt";
  FILE *fp = fopen(path, "w");
  char *argv[] = {(char *)"prog", (char *)"--mode", (char *)"slow",
                  (char *)"@/tmp/argparse_c_fromfile_comments.txt", NULL};
  int32_t parsed = 0;
  const char *mode_value = NULL;

  CHECK(fp != NULL);
  fputs("  # whole-line comment\n\n--num\n7\ntrailing  # trailing comment\n",
        fp);
  fclose(fp);

  options.fromfile_prefix_chars = "@";
  p = ap_parser_new_with_options("prog", "desc", options);
  CHECK(p != NULL);
  num.type = AP_TYPE_INT32;
  mode.choices.items = mode_choices;
  mode.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "--num", num, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));
  LONGS_EQUAL(0,
              ap_add_argument(p, "trailing", ap_arg_options_default(), &err));

  LONGS_EQUAL(0, ap_parse_args(p, 4, argv, &ns, &err));
  CHECK(ap_ns_get_int32(ns, "num", &parsed));
  CHECK(ap_ns_get_string(ns, "mode", &mode_value));
  LONGS_EQUAL(7, parsed);
  STRCMP_EQUAL("slow", mode_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
  remove(path);
}

TEST(FromfilePrefixMissingFileReturnsDefinitionError) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser_options options = ap_parser_options_default();
  ap_parser *p = NULL;
  ap_arg_options num = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"@/tmp/argparse_c_missing_args.txt",
                  NULL};

  options.fromfile_prefix_chars = "@";
  p = ap_parser_new_with_options("prog", "desc", options);
  CHECK(p != NULL);
  num.type = AP_TYPE_INT32;
  LONGS_EQUAL(0, ap_add_argument(p, "--num", num, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 2, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("/tmp/argparse_c_missing_args.txt", err.argument);
  CHECK(strstr(err.message, "failed to open args file") != NULL);

  ap_parser_free(p);
}

TEST(ParseKnownIntermixedArgsDelegatesToKnownArgsBehavior) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options count = ap_arg_options_default();
  char **unknown = NULL;
  int unknown_count = 0;
  int32_t parsed_count = 0;
  char *argv[] = {(char *)"prog",      (char *)"--count", (char *)"3",
                  (char *)"--mystery", (char *)"value",   NULL};

  CHECK(p != NULL);
  count.type = AP_TYPE_INT32;
  LONGS_EQUAL(0, ap_add_argument(p, "--count", count, &err));

  LONGS_EQUAL(0, ap_parse_known_intermixed_args(p, 5, argv, &ns, &unknown,
                                                &unknown_count, &err));
  CHECK(ap_ns_get_int32(ns, "count", &parsed_count));
  LONGS_EQUAL(3, parsed_count);
  LONGS_EQUAL(2, unknown_count);
  STRCMP_EQUAL("--mystery", unknown[0]);
  STRCMP_EQUAL("value", unknown[1]);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}
