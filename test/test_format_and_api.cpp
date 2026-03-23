#include "test_framework.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>

namespace {

std::string shell_quote(const std::string &value) {
  std::string quoted = "'";

  for (char ch : value) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted += ch;
    }
  }

  quoted += '\'';
  return quoted;
}

std::string make_temp_dir() {
  std::array<char, 64> pattern = {0};
  std::snprintf(pattern.data(), pattern.size(), "/tmp/argparse-c-XXXXXX");
  char *path = mkdtemp(pattern.data());

  if (!path) {
    std::ostringstream detail;
    detail << "mkdtemp failed: " << strerror(errno);
    throw std::runtime_error(detail.str());
  }

  return std::string(path);
}

int run_command(const std::string &command) {
  int rc = std::system(command.c_str());

  if (rc == -1) {
    std::ostringstream detail;
    detail << "system failed for command: " << command;
    throw std::runtime_error(detail.str());
  }

  return rc;
}

int dynamic_exec_completion(const ap_completion_request *request,
                            ap_completion_result *result, void *user_data,
                            ap_error *err) {
  const char *const *items = static_cast<const char *const *>(user_data);
  const char *prefix =
      request && request->current_token ? request->current_token : "";

  for (int i = 0; items && items[i] != nullptr; i++) {
    if (std::strncmp(items[i], prefix, std::strlen(prefix)) == 0 &&
        ap_completion_result_add(result, items[i], "dynamic", err) != 0) {
      return -1;
    }
  }
  return 0;
}

} // namespace

TEST(FormatManpageIncludesOptionsAndNestedSubcommands) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "top level parser");
  ap_parser *config = NULL;
  ap_parser *set = NULL;
  ap_arg_options mode = ap_arg_options_default();
  const char *choices[] = {"fast", "slow"};
  char *manpage = NULL;

  CHECK(p != NULL);
  mode.help = "select execution mode";
  mode.metavar = "MODE";
  mode.default_value = "fast";
  mode.required = true;
  mode.choices.items = choices;
  mode.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "-m, --mode", mode, &err));
  config = ap_add_subcommand(p, "config", "manage configuration", &err);
  CHECK(config != NULL);
  set = ap_add_subcommand(config, "set", "set a value", &err);
  CHECK(set != NULL);

  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, ".SH NAME") != NULL);
  CHECK(strstr(manpage, ".SH SYNOPSIS") != NULL);
  CHECK(strstr(manpage, ".SH OPTIONS") != NULL);
  CHECK(strstr(manpage, "\\-m, \\-\\-mode MODE") != NULL);
  CHECK(strstr(manpage, "choices: fast, slow") != NULL);
  CHECK(strstr(manpage, "default: fast") != NULL);
  CHECK(strstr(manpage, "required: yes") != NULL);
  CHECK(strstr(manpage, ".SH SUBCOMMANDS") != NULL);
  CHECK(strstr(manpage, "config") != NULL);
  CHECK(strstr(manpage, "set a value") != NULL);
  CHECK(strstr(manpage, ".SH ERRORS") != NULL);

  free(manpage);
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

TEST(RequiredStoreTrueOptionMustBePresent) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options force = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  force.type = AP_TYPE_BOOL;
  force.action = AP_ACTION_STORE_TRUE;
  force.required = true;
  LONGS_EQUAL(0, ap_add_argument(p, "--force", force, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("--force", err.argument);
  STRCMP_EQUAL("option '--force' is required", err.message);

  ap_parser_free(p);
}

TEST(RequiredStoreConstOptionMustBePresent) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options color = ap_arg_options_default();
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  color.action = AP_ACTION_STORE_CONST;
  color.const_value = "always";
  color.required = true;
  LONGS_EQUAL(0, ap_add_argument(p, "--color", color, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  STRCMP_EQUAL("--color", err.argument);
  STRCMP_EQUAL("option '--color' is required", err.message);

  ap_parser_free(p);
}

TEST(ParseKnownArgsRequiresAllOutputPointers) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  int unknown_count = 0;
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(-1,
              ap_parse_known_args(p, 1, argv, &ns, NULL, &unknown_count, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser, outputs and unknown outputs are required", err.message);

  ap_parser_free(p);
}

TEST(FormatErrorUsesFallbackMessageWhenErrorMissing) {
  ap_parser *p = ap_parser_new("prog", "desc");
  char *text = NULL;

  CHECK(p != NULL);
  text = ap_format_error(p, NULL);
  CHECK(text != NULL);
  CHECK(strstr(text, "error: unknown error") != NULL);
  CHECK(strstr(text, "usage: prog") != NULL);

  free(text);
  ap_parser_free(p);
}

TEST(FormatErrorReturnsNullWithoutParser) {
  CHECK(ap_format_error(NULL, NULL) == NULL);
}

TEST(NullFreeHelpersAreNoOps) {
  ap_parser_free(NULL);
  ap_namespace_free(NULL);
  ap_free_tokens(NULL, 0);
  CHECK_TRUE(true);
}

TEST(NamespaceGetterFailurePathsReturnFalseOrNull) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options count = ap_arg_options_default();
  ap_arg_options name = ap_arg_options_default();
  bool bool_out = false;
  int32_t int_out = 0;
  const char *str_out = NULL;
  char *argv[] = {(char *)"prog",   (char *)"--verbose", (char *)"--count",
                  (char *)"--name", (char *)"alice",     NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  count.type = AP_TYPE_INT32;
  count.action = AP_ACTION_COUNT;
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--count", count, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--name", name, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 5, argv, &ns, &err));
  LONGS_EQUAL(1, ap_ns_get_count(ns, "verbose"));
  CHECK(!ap_ns_get_bool(ns, "name", &bool_out));
  CHECK(!ap_ns_get_bool(ns, "verbose", NULL));
  CHECK(!ap_ns_get_string(ns, "verbose", &str_out));
  CHECK(!ap_ns_get_string(ns, "missing", &str_out));
  CHECK(!ap_ns_get_int32(ns, "name", &int_out));
  CHECK(!ap_ns_get_int32(ns, "count", NULL));
  LONGS_EQUAL(-1, ap_ns_get_count(ns, "missing"));
  CHECK(ap_ns_get_string_at(ns, "missing", 0) == NULL);
  CHECK(ap_ns_get_string_at(ns, "name", 1) == NULL);
  CHECK(!ap_ns_get_int32_at(ns, "count", -1, &int_out));
  CHECK(!ap_ns_get_int32_at(ns, "count", 1, &int_out));
  CHECK(!ap_ns_get_int32_at(ns, "count", 0, NULL));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(FormatHelpShowsPositionalOneOrMoreAndActionFlags) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options quiet = ap_arg_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options color = ap_arg_options_default();
  ap_arg_options inputs = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  quiet.type = AP_TYPE_BOOL;
  quiet.action = AP_ACTION_STORE_FALSE;
  quiet.help = "disable output";
  verbose.type = AP_TYPE_INT32;
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  verbose.help = "increase verbosity";
  color.action = AP_ACTION_STORE_CONST;
  color.const_value = "always";
  color.help = "force color";
  inputs.nargs = AP_NARGS_ONE_OR_MORE;
  inputs.action = AP_ACTION_APPEND;
  inputs.help = "input files";
  inputs.metavar = "INPUT";

  LONGS_EQUAL(0, ap_add_argument(p, "--quiet", quiet, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--color", color, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "inputs", inputs, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "--quiet") != NULL);
  CHECK(strstr(help, "-v, --verbose") != NULL);
  CHECK(strstr(help, "--color") != NULL);
  CHECK(strstr(help, "INPUT [INPUT ...]") != NULL);
  CHECK(strstr(help, "disable output") != NULL);
  CHECK(strstr(help, "increase verbosity") != NULL);
  CHECK(strstr(help, "force color") != NULL);
  CHECK(strstr(help, "input files") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(ParserIntrospectionEnumeratesArgumentsAndFlags) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "top level parser");
  ap_parser_info parser_info = {};
  ap_arg_info help_info = {};
  ap_arg_info output_info = {};
  ap_arg_info input_info = {};
  const char *modes[] = {"fast", "slow"};
  ap_arg_options output = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();

  CHECK(p != NULL);
  output.help = "write output";
  output.metavar = "FILE";
  output.required = true;
  output.choices.count = 2;
  output.choices.items = modes;
  output.type = AP_TYPE_STRING;
  output.action = AP_ACTION_APPEND;
  output.nargs = AP_NARGS_FIXED;
  output.nargs_count = 2;
  input.help = "input path";
  input.metavar = "INPUT";
  input.nargs = AP_NARGS_OPTIONAL;

  LONGS_EQUAL(0, ap_add_argument(p, "-o, --output", output, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "input", input, &err));

  LONGS_EQUAL(0, ap_parser_get_info(p, &parser_info));
  STRCMP_EQUAL("tool", parser_info.prog);
  STRCMP_EQUAL("top level parser", parser_info.description);
  LONGS_EQUAL(3, parser_info.argument_count);
  LONGS_EQUAL(0, parser_info.subcommand_count);

  LONGS_EQUAL(0, ap_parser_get_argument(p, 0, &help_info));
  LONGS_EQUAL(AP_ARG_KIND_OPTIONAL, help_info.kind);
  LONGS_EQUAL(2, help_info.flag_count);
  STRCMP_EQUAL("-h", help_info.flags[0]);
  STRCMP_EQUAL("--help", help_info.flags[1]);
  LONGS_EQUAL(1, ap_arg_short_flag_count(&help_info));
  LONGS_EQUAL(1, ap_arg_long_flag_count(&help_info));
  STRCMP_EQUAL("-h", ap_arg_short_flag_at(&help_info, 0));
  STRCMP_EQUAL("--help", ap_arg_long_flag_at(&help_info, 0));

  LONGS_EQUAL(0, ap_parser_get_argument(p, 1, &output_info));
  LONGS_EQUAL(AP_ARG_KIND_OPTIONAL, output_info.kind);
  STRCMP_EQUAL("output", output_info.dest);
  STRCMP_EQUAL("write output", output_info.help);
  STRCMP_EQUAL("FILE", output_info.metavar);
  CHECK_TRUE(output_info.required);
  LONGS_EQUAL(AP_NARGS_FIXED, output_info.nargs);
  LONGS_EQUAL(2, output_info.nargs_count);
  LONGS_EQUAL(AP_TYPE_STRING, output_info.type);
  LONGS_EQUAL(AP_ACTION_APPEND, output_info.action);
  LONGS_EQUAL(2, output_info.choices.count);
  STRCMP_EQUAL("fast", output_info.choices.items[0]);
  STRCMP_EQUAL("slow", output_info.choices.items[1]);
  LONGS_EQUAL(1, ap_arg_short_flag_count(&output_info));
  LONGS_EQUAL(1, ap_arg_long_flag_count(&output_info));
  STRCMP_EQUAL("-o", ap_arg_short_flag_at(&output_info, 0));
  STRCMP_EQUAL("--output", ap_arg_long_flag_at(&output_info, 0));

  LONGS_EQUAL(0, ap_parser_get_argument(p, 2, &input_info));
  LONGS_EQUAL(AP_ARG_KIND_POSITIONAL, input_info.kind);
  LONGS_EQUAL(1, input_info.flag_count);
  STRCMP_EQUAL("input", input_info.flags[0]);
  STRCMP_EQUAL("input", input_info.dest);
  STRCMP_EQUAL("input path", input_info.help);
  STRCMP_EQUAL("INPUT", input_info.metavar);
  LONGS_EQUAL(0, ap_arg_short_flag_count(&input_info));
  LONGS_EQUAL(0, ap_arg_long_flag_count(&input_info));
  CHECK(ap_arg_short_flag_at(&input_info, 0) == NULL);
  CHECK(ap_arg_long_flag_at(&input_info, 0) == NULL);

  ap_parser_free(p);
}

TEST(ParserIntrospectionRejectsInvalidInputs) {
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_parser_info parser_info = {};
  ap_arg_info arg_info = {};
  ap_subcommand_info subcommand_info = {};

  CHECK(p != NULL);
  LONGS_EQUAL(-1, ap_parser_get_info(NULL, &parser_info));
  LONGS_EQUAL(-1, ap_parser_get_info(p, NULL));
  LONGS_EQUAL(-1, ap_parser_get_argument(NULL, 0, &arg_info));
  LONGS_EQUAL(-1, ap_parser_get_argument(p, -1, &arg_info));
  LONGS_EQUAL(-1, ap_parser_get_argument(p, 99, &arg_info));
  LONGS_EQUAL(-1, ap_parser_get_argument(p, 0, NULL));
  LONGS_EQUAL(-1, ap_parser_get_subcommand(NULL, 0, &subcommand_info));
  LONGS_EQUAL(-1, ap_parser_get_subcommand(p, -1, &subcommand_info));
  LONGS_EQUAL(-1, ap_parser_get_subcommand(p, 0, &subcommand_info));
  LONGS_EQUAL(-1, ap_parser_get_subcommand(p, 0, NULL));
  LONGS_EQUAL(0, ap_arg_short_flag_count(NULL));
  LONGS_EQUAL(0, ap_arg_long_flag_count(NULL));
  CHECK(ap_arg_short_flag_at(NULL, 0) == NULL);
  CHECK(ap_arg_long_flag_at(NULL, 0) == NULL);

  ap_parser_free(p);
}

TEST(ApiGuardsRejectMissingParserPointers) {
  ap_error err = {};

  LONGS_EQUAL(-1,
              ap_add_argument(NULL, "--name", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser and argument name are required", err.message);

  CHECK(ap_add_mutually_exclusive_group(NULL, false, &err) == NULL);
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser is required", err.message);
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
  char *argv[] = {(char *)"prog",      (char *)"--include", (char *)"a",
                  (char *)"--include", (char *)"b",         NULL};

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
  char *argv[] = {(char *)"prog", (char *)"--range", (char *)"10", (char *)"20",
                  NULL};
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
  STRCMP_EQUAL("one argument from a mutually exclusive group is required",
               err.message);

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

TEST(FormatFishCompletionIncludesSubcommandsOptionsAndChoices) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *config = NULL;
  ap_parser *set = NULL;
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options output = ap_arg_options_default();
  ap_arg_options force = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  const char *formats[] = {"json", "yaml"};
  const char *modes[] = {"fast", "slow"};
  char *script = NULL;

  CHECK(p != NULL);
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  verbose.help = "increase verbosity";
  force.type = AP_TYPE_BOOL;
  force.action = AP_ACTION_STORE_TRUE;
  force.help = "force overwrite";
  output.help = "output format";
  output.choices.items = formats;
  output.choices.count = 2;
  mode.help = "mode for config";
  mode.choices.items = modes;
  mode.choices.count = 2;
  target.help = "set target";
  target.metavar = "KEY";

  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--output", output, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--force", force, &err));
  config = ap_add_subcommand(p, "config", "config commands", &err);
  CHECK(config != NULL);
  LONGS_EQUAL(0, ap_add_argument(config, "--mode", mode, &err));
  set = ap_add_subcommand(config, "set", "set values", &err);
  CHECK(set != NULL);
  LONGS_EQUAL(0, ap_add_argument(set, "--target", target, &err));

  script = ap_format_fish_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "complete -c \"prog\" -f") != NULL);
  CHECK(strstr(script, "function __ap_prog_parser_key") != NULL);
  CHECK(strstr(script, "case \"root:config\"") != NULL);
  CHECK(strstr(script, "set key \"root/config\"") != NULL);
  CHECK(strstr(script, "case \"root/config:set\"") != NULL);
  CHECK(strstr(script, "set key \"root/config/set\"") != NULL);
  CHECK(strstr(script, "complete -c \"prog\" -n '__ap_prog_parser_is root' -f "
                       "-a \"config\" -d \"config commands\"") != NULL);
  CHECK(strstr(script,
               "complete -c \"prog\" -n '__ap_prog_parser_is root' -s "
               "h -l help -d \"show this help message and exit\"") != NULL);
  CHECK(strstr(script, "complete -c \"prog\" -n '__ap_prog_parser_is root' -s "
                       "v -l verbose -d \"increase verbosity\"") != NULL);
  CHECK(strstr(script, "complete -c \"prog\" -n '__ap_prog_parser_is root' -l "
                       "output -d \"output format\" -r -a "
                       "'(__ap_prog_value_choices root:--output)'") != NULL);
  CHECK(strstr(script, "complete -c \"prog\" -n '__ap_prog_parser_is root' -l "
                       "force -d \"force overwrite\"") != NULL);
  CHECK(strstr(script,
               "complete -c \"prog\" -n '__ap_prog_parser_is "
               "root/config' -l mode -d \"mode for config\" -r -a "
               "'(__ap_prog_value_choices root/config:--mode)'") != NULL);
  CHECK(strstr(script, "complete -c \"prog\" -n '__ap_prog_parser_is "
                       "root/config' -f -a \"set\" -d \"set values\"") != NULL);
  CHECK(strstr(script,
               "complete -c \"prog\" -n '__ap_prog_parser_is "
               "root/config/set' -l target -d \"set target\" -r") != NULL);
  CHECK(strstr(script, "case \"root:--output\"") != NULL);
  CHECK(strstr(script, "\"json\" \"yaml\"") != NULL);
  CHECK(strstr(script, "case \"root/config:--mode\"") != NULL);
  CHECK(strstr(script, "\"fast\" \"slow\"") != NULL);

  free(script);
  ap_parser_free(p);
}

TEST(FormatBashCompletionIncludesRootSubcommandsAndChoices) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *config = NULL;
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options output = ap_arg_options_default();
  ap_arg_options force = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  const char *formats[] = {"json", "yaml"};
  const char *modes[] = {"fast", "slow"};
  char *script = NULL;

  CHECK(p != NULL);
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  force.type = AP_TYPE_BOOL;
  force.action = AP_ACTION_STORE_TRUE;
  output.choices.items = formats;
  output.choices.count = 2;
  mode.choices.items = modes;
  mode.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--output", output, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--force", force, &err));
  config = ap_add_subcommand(p, "config", "config commands", &err);
  CHECK(config != NULL);
  LONGS_EQUAL(0, ap_add_argument(config, "--mode", mode, &err));

  script = ap_format_bash_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "complete -F _prog 'prog'") != NULL);
  CHECK(strstr(script, "parser_subcommands='config'") != NULL);
  CHECK(strstr(script,
               "parser_options='-h --help -v --verbose --output --force'") !=
        NULL);
  CHECK(strstr(script, "parser_value_options='--output'") != NULL);
  CHECK(strstr(script,
               "parser_flag_only_options='-h --help -v --verbose --force'") !=
        NULL);
  CHECK(strstr(script, "root:--output)") != NULL);
  CHECK(strstr(script, "'json' 'yaml'") != NULL);
  CHECK(strstr(script, "root/config:--mode)") != NULL);
  CHECK(strstr(script, "'fast' 'slow'") != NULL);
  CHECK(strstr(script, "root/config)\n        parser_subcommands=''\n        "
                       "parser_options='-h --help --mode'") != NULL);
  CHECK(strstr(script, "__ap_completion_filter_mutex_candidates") != NULL);

  free(script);
  ap_parser_free(p);
}


TEST(FormatGeneratorsEscapeSpecialCharactersAndDeepNestedCompletionPaths) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("my-prog.v1", "desc");
  ap_parser *build = NULL;
  ap_parser *lint = NULL;
  ap_parser *check = NULL;
  ap_parser *leaf = NULL;
  ap_arg_options bash_choice = ap_arg_options_default();
  ap_arg_options fish_opt = ap_arg_options_default();
  ap_arg_options man_opt = ap_arg_options_default();
  ap_arg_options leaf_mode = ap_arg_options_default();
  const char *bash_choices[] = {"two words", "it's"};
  const char *fish_choices[] = {"quote\"", "path\\dir", "$HOME", "line1\nline2"};
  const char *man_choices[] = {"-dash", "\\slash", ".dot", "'tick"};
  const char *leaf_choices[] = {"fast", "slow"};
  char *bash = NULL;
  char *fish = NULL;
  char *manpage = NULL;

  CHECK(p != NULL);
  bash_choice.choices.items = bash_choices;
  bash_choice.choices.count = 2;
  fish_opt.help = "say \"hi\"\\$USER\nnext";
  fish_opt.choices.items = fish_choices;
  fish_opt.choices.count = 4;
  man_opt.help = ".lead\n'quote\n-dash\\trail";
  man_opt.default_value = "-default\\value";
  man_opt.choices.items = man_choices;
  man_opt.choices.count = 4;
  leaf_mode.help = "leaf mode";
  leaf_mode.choices.items = leaf_choices;
  leaf_mode.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "--bash-choice", bash_choice, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--fish-opt", fish_opt, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--man-opt", man_opt, &err));
  build = ap_add_subcommand(p, "build-tools", "build helpers", &err);
  CHECK(build != NULL);
  lint = ap_add_subcommand(build, "lint.v2", "lint helpers", &err);
  CHECK(lint != NULL);
  check = ap_add_subcommand(lint, "check-x", "check helpers", &err);
  CHECK(check != NULL);
  leaf = ap_add_subcommand(check, "final.step", "final leaf", &err);
  CHECK(leaf != NULL);
  LONGS_EQUAL(0, ap_add_argument(leaf, "--leaf-mode", leaf_mode, &err));

  bash = ap_format_bash_completion(p);
  fish = ap_format_fish_completion(p);
  manpage = ap_format_manpage(p);

  CHECK(bash != NULL);
  CHECK(fish != NULL);
  CHECK(manpage != NULL);

  CHECK(strstr(bash, "complete -F _my_prog_v1 'my-prog.v1'") != NULL);
  CHECK(strstr(bash, "parser_subcommands='build-tools'") != NULL);
  CHECK(strstr(bash, "root:--bash-choice)") != NULL);
  CHECK(strstr(bash, "'two words' 'it'\\''s'") != NULL);
  CHECK(strstr(bash, "root/build-tools/lint.v2/check-x)") != NULL);
  CHECK(strstr(bash, "parser_subcommands='final.step'") != NULL);
  CHECK(strstr(bash, "root/build-tools/lint.v2/check-x/final.step)") != NULL);
  CHECK(strstr(bash, "parser_options='-h --help --leaf-mode'") != NULL);
  CHECK(strstr(bash, "root/build-tools/lint.v2/check-x/final.step:--leaf-mode)") != NULL);
  CHECK(strstr(bash, "'fast' 'slow'") != NULL);

  CHECK(strstr(fish, "complete -c \"my-prog.v1\" -f") != NULL);
  CHECK(strstr(fish, "function __ap_my_prog_v1_parser_key") != NULL);
  CHECK(strstr(fish, "-l fish-opt -d \"say \\\"hi\\\"\\\\\\$USER next\" -r -a '(__ap_my_prog_v1_value_choices root:--fish-opt)'") != NULL);
  CHECK(strstr(fish, "case \"root:--fish-opt\"") != NULL);
  CHECK(strstr(fish, "\"quote\\\"\" \"path\\\\dir\" \"\\$HOME\" \"line1 line2\"") != NULL);
  CHECK(strstr(fish, "case \"root:build-tools\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools\"") != NULL);
  CHECK(strstr(fish, "case \"root/build-tools:lint.v2\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools/lint.v2\"") != NULL);
  CHECK(strstr(fish, "case \"root/build-tools/lint.v2:check-x\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools/lint.v2/check-x\"") != NULL);
  CHECK(strstr(fish, "case \"root/build-tools/lint.v2/check-x:final.step\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools/lint.v2/check-x/final.step\"") != NULL);
  CHECK(strstr(fish, "root/build-tools/lint.v2/check-x/final.step' -l leaf-mode -d \"leaf mode\" -r -a '(__ap_my_prog_v1_value_choices root/build-tools/lint.v2/check-x/final.step:--leaf-mode)'") != NULL);

  CHECK(strstr(manpage, "\\&.lead") != NULL);
  CHECK(strstr(manpage, "\\&'quote") != NULL);
  CHECK(strstr(manpage, "\\-dash\\\\trail") != NULL);
  CHECK(strstr(manpage, "default: \\-default\\\\value") != NULL);
  CHECK(strstr(manpage, "choices: \\-dash, \\\\slash, \\&.dot, \\&'tick") != NULL);

  free(bash);
  free(fish);
  free(manpage);
  ap_parser_free(p);
}

TEST(ArgumentInfoExposesCompletionMetadata) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options cmd = ap_arg_options_default();
  ap_arg_info info = {};
  static const char *const commands[] = {"git", nullptr};

  CHECK(p != NULL);
  cmd.completion_kind = AP_COMPLETION_KIND_COMMAND;
  cmd.completion_hint = "shell command";
  cmd.completion_callback = dynamic_exec_completion;
  cmd.completion_user_data = (void *)commands;

  LONGS_EQUAL(0, ap_add_argument(p, "--exec", cmd, &err));
  LONGS_EQUAL(0, ap_parser_get_argument(p, 1, &info));
  LONGS_EQUAL(AP_COMPLETION_KIND_COMMAND, info.completion_kind);
  STRCMP_EQUAL("shell command", info.completion_hint);
  CHECK(info.has_completion_callback);

  ap_parser_free(p);
}

TEST(CompleteUsesDynamicCallbackAndStaticChoiceFallback) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options exec = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  ap_completion_result result = {};
  static const char *const commands[] = {"git", "grep", "ls", nullptr};
  const char *modes[] = {"fast", "slow"};
  char arg0[] = "--exec";
  char arg1[] = "gr";
  char *argv1[] = {arg0, arg1};
  char arg2[] = "--mode";
  char arg3[] = "s";
  char *argv2[] = {arg2, arg3};

  CHECK(p != NULL);
  exec.completion_callback = dynamic_exec_completion;
  exec.completion_user_data = (void *)commands;
  mode.choices.items = modes;
  mode.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "--exec", exec, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(0, ap_complete(p, 2, argv1, "bash", &result, &err));
  LONGS_EQUAL(1, result.count);
  STRCMP_EQUAL("grep", result.items[0].value);
  STRCMP_EQUAL("gr", arg1);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 2, argv2, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("fast", result.items[0].value);
  STRCMP_EQUAL("slow", result.items[1].value);
  ap_completion_result_free(&result);

  ap_parser_free(p);
}

TEST(CompleteRejectsNullArgvWhenArgcIsPositive) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_completion_result result = {};

  CHECK(p != NULL);

  LONGS_EQUAL(-1, ap_complete(p, 1, NULL, "bash", &result, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);

  ap_parser_free(p);
}

TEST(FormatCompletionUsesStaticCompletionMetadata) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options dir = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  const char *modes[] = {"fast", "slow"};
  char *bash = NULL;
  char *fish = NULL;

  CHECK(p != NULL);
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  input.completion_hint = "input file";
  dir.completion_kind = AP_COMPLETION_KIND_DIRECTORY;
  exec.completion_kind = AP_COMPLETION_KIND_COMMAND;
  mode.choices.items = modes;
  mode.choices.count = 2;
  mode.completion_kind = AP_COMPLETION_KIND_CHOICES;

  LONGS_EQUAL(0, ap_add_argument(p, "--input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--dir", dir, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--exec", exec, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  bash = ap_format_bash_completion(p);
  fish = ap_format_fish_completion(p);
  CHECK(bash != NULL);
  CHECK(fish != NULL);
  CHECK(strstr(bash, "root:--input) printf '%s\\n' 'file' ;;") != NULL);
  CHECK(strstr(bash, "root:--dir) printf '%s\\n' 'directory' ;;") != NULL);
  CHECK(strstr(bash, "root:--exec) printf '%s\\n' 'command' ;;") != NULL);
  CHECK(strstr(bash, "compgen -f -- \"$cur\"") != NULL);
  CHECK(strstr(bash, "compgen -d -- \"$cur\"") != NULL);
  CHECK(strstr(bash, "compgen -c -- \"$cur\"") != NULL);
  CHECK(strstr(fish, "-l input -d \"INPUT\" -r -F") != NULL);
  CHECK(
      strstr(fish, "-l dir -d \"DIR\" -r -a '(__fish_complete_directories)'") !=
      NULL);
  CHECK(strstr(fish, "-l exec -d \"EXEC\" -r -a '(__fish_complete_command)'") !=
        NULL);
  CHECK(strstr(fish, "-l mode -d \"MODE\" -r -a '(__ap_prog_value_choices "
                     "root:--mode)'") != NULL);

  free(bash);
  free(fish);
  ap_parser_free(p);
}

TEST(FormatCompletionScriptsCallApplicationForDynamicCallbacks) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options exec = ap_arg_options_default();
  char *bash = NULL;
  char *fish = NULL;
  static const char *const commands[] = {"git", nullptr};

  CHECK(p != NULL);
  exec.completion_callback = dynamic_exec_completion;
  exec.completion_user_data = (void *)commands;

  LONGS_EQUAL(0, ap_add_argument(p, "--exec", exec, &err));

  bash = ap_format_bash_completion(p);
  fish = ap_format_fish_completion(p);
  CHECK(bash != NULL);
  CHECK(fish != NULL);
  CHECK(strstr(bash, "__complete --shell bash -- \"${COMP_WORDS[@]:1}\"") !=
        NULL);
  CHECK(strstr(bash, "root:--exec) printf '%s\\n' 'dynamic' ;;") != NULL);
  CHECK(strstr(fish, "\"prog\" __complete --shell fish -- $tokens $current") !=
        NULL);
  CHECK(strstr(fish,
               "-l exec -d \"EXEC\" -r -a '(__ap_prog_dynamic_complete)'") !=
        NULL);

  free(bash);
  free(fish);
  ap_parser_free(p);
}

TEST(FormatBashCompletionMarksValueModesAndFlagOnlyOptions) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options quiet = ap_arg_options_default();
  char *script = NULL;

  CHECK(p != NULL);
  maybe.nargs = AP_NARGS_OPTIONAL;
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  quiet.type = AP_TYPE_BOOL;
  quiet.action = AP_ACTION_STORE_FALSE;

  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--quiet", quiet, &err));

  script = ap_format_bash_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "parser_value_options='--maybe --extras --pair'") !=
        NULL);
  CHECK(strstr(script, "parser_flag_only_options=") != NULL);
  CHECK(strstr(script, "--quiet") != NULL);
  CHECK(strstr(script, "root:--maybe)") != NULL);
  CHECK(strstr(script, "'optional' ;;") != NULL);
  CHECK(strstr(script, "root:--extras)") != NULL);
  CHECK(strstr(script, "'multi' ;;") != NULL);
  CHECK(strstr(script, "root:--pair)") != NULL);
  CHECK(strstr(script, "'fixed' ;;") != NULL);
  CHECK(strstr(script, "root:--pair) printf") != NULL);
  CHECK(strstr(script, " 2 ;;") != NULL);

  free(script);
  ap_parser_free(p);
}

TEST(GeneratedBashCompletionScriptPassesBashSyntaxCheck) {
  const std::string temp_dir = make_temp_dir();
  const std::filesystem::path script_path =
      std::filesystem::path(temp_dir) / "example_completion.bash";
  const std::string generate_command = shell_quote(AP_EXAMPLE_COMPLETION_PATH) +
                                       " --generate-bash-completion > " +
                                       shell_quote(script_path.string());

  CHECK(run_command(generate_command) == 0);
  CHECK(std::filesystem::exists(script_path));
  CHECK(run_command("bash -n " + shell_quote(script_path.string())) == 0);

  std::filesystem::remove_all(temp_dir);
}

TEST(GeneratedManpageRendersWithMan) {
  const std::string temp_dir = make_temp_dir();
  const std::filesystem::path manpage_path =
      std::filesystem::path(temp_dir) / "example_manpage.1";
  const std::string generate_command = shell_quote(AP_EXAMPLE_MANPAGE_PATH) +
                                       " --generate-manpage > " +
                                       shell_quote(manpage_path.string());
  const std::string render_command = "MANPAGER=cat MANWIDTH=80 man -l " +
                                     shell_quote(manpage_path.string()) +
                                     " > /dev/null";

  CHECK(run_command(generate_command) == 0);
  CHECK(std::filesystem::exists(manpage_path));
  CHECK(run_command(render_command) == 0);

  std::filesystem::remove_all(temp_dir);
}
