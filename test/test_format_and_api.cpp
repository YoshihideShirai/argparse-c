#include "test_framework.h"

#include <array>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

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

struct action_probe {
  int call_count;
  int argc;
  std::string first_token;
  bool verbose_value;
};

int inspect_action_callback(const ap_action_request *request, ap_namespace *ns,
                            void *user_data, ap_error *err) {
  action_probe *probe = static_cast<action_probe *>(user_data);
  bool verbose = false;
  (void)err;
  if (!probe || !request || !ns) {
    return -1;
  }
  probe->call_count++;
  probe->argc = request->argc;
  probe->first_token =
      request->argc > 0 && request->argv ? request->argv[0] : "";
  if (ap_ns_get_bool(ns, request->dest, &verbose)) {
    probe->verbose_value = verbose;
  }
  return 0;
}

int failing_action_without_error(const ap_action_request *request,
                                 ap_namespace *ns, void *user_data,
                                 ap_error *err) {
  (void)request;
  (void)ns;
  (void)user_data;
  (void)err;
  return -1;
}

int failing_action_with_error(const ap_action_request *request,
                              ap_namespace *ns, void *user_data,
                              ap_error *err) {
  (void)request;
  (void)ns;
  (void)user_data;
  if (err) {
    std::memset(err, 0, sizeof(*err));
    err->code = AP_ERR_INVALID_CHOICE;
    std::snprintf(err->argument, sizeof(err->argument), "%s", "mode");
    std::snprintf(err->message, sizeof(err->message), "%s",
                  "mode rejected by callback");
  }
  return -1;
}

} // namespace

extern "C" {
#include "../lib/ap_internal.h"
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

TEST(ParseArgsRequiresParserAndOutputNamespace) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  char *argv[] = {(char *)"prog", NULL};

  CHECK(p != NULL);

  LONGS_EQUAL(-1, ap_parse_args(NULL, 1, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser and out_ns are required", err.message);

  LONGS_EQUAL(-1, ap_parse_args(p, 1, argv, NULL, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser and out_ns are required", err.message);

  ap_parser_free(p);
}

TEST(FormattersReturnNullForNullOrEmptyProgram) {
  ap_parser *empty_prog = ap_parser_new("", "desc");
  char *usage = NULL;
  char *help = NULL;
  char *manpage = NULL;

  CHECK(ap_format_usage(NULL) == NULL);
  CHECK(ap_format_help(NULL) == NULL);
  CHECK(ap_format_manpage(NULL) == NULL);
  CHECK(ap_format_bash_completion(NULL) == NULL);
  CHECK(ap_format_fish_completion(NULL) == NULL);
  CHECK(ap_format_zsh_completion(NULL) == NULL);

  CHECK(empty_prog != NULL);
  usage = ap_format_usage(empty_prog);
  help = ap_format_help(empty_prog);
  manpage = ap_format_manpage(empty_prog);
  CHECK(usage != NULL);
  CHECK(help != NULL);
  CHECK(manpage != NULL);
  CHECK(ap_format_bash_completion(empty_prog) == NULL);
  CHECK(ap_format_fish_completion(empty_prog) == NULL);
  CHECK(ap_format_zsh_completion(empty_prog) == NULL);

  free(usage);
  free(help);
  free(manpage);
  ap_parser_free(empty_prog);
}

TEST(ActionCallbackRunsAfterBuiltInActionAndReceivesTokens) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  action_probe probe = {};
  bool value = false;
  char *argv[] = {(char *)"prog", (char *)"--verbose", NULL};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  verbose.action_callback = inspect_action_callback;
  verbose.action_user_data = &probe;
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 2, argv, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "verbose", &value));
  CHECK(value);
  LONGS_EQUAL(1, probe.call_count);
  LONGS_EQUAL(1, probe.argc);
  STRCMP_EQUAL("--verbose", probe.first_token.c_str());
  CHECK(probe.verbose_value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ActionCallbackFailurePropagatesErrorConsistently) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *p2 = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  char *argv[] = {(char *)"prog", (char *)"--mode", (char *)"fast", NULL};

  CHECK(p != NULL);
  mode.action_callback = failing_action_without_error;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(-1, ap_parse_args(p, 3, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--mode", err.argument);
  STRCMP_EQUAL("action callback failed for '--mode'", err.message);

  CHECK(p2 != NULL);
  mode = ap_arg_options_default();
  mode.action_callback = failing_action_with_error;
  LONGS_EQUAL(0, ap_add_argument(p2, "--mode2", mode, &err));
  argv[1] = (char *)"--mode2";
  LONGS_EQUAL(-1, ap_parse_args(p2, 3, argv, &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_CHOICE, err.code);
  STRCMP_EQUAL("mode", err.argument);
  STRCMP_EQUAL("mode rejected by callback", err.message);

  ap_parser_free(p);
  ap_parser_free(p2);
}

TEST(ParseKnownArgsFailureClearsUnknownOutputs) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = new_base_parser();
  char *argv[] = {(char *)"prog", (char *)"file.txt", NULL};
  char **unknown = (char **)0x1;
  int unknown_count = 99;

  CHECK(p != NULL);
  LONGS_EQUAL(
      -1, ap_parse_known_args(p, 2, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(AP_ERR_MISSING_REQUIRED, err.code);
  CHECK(unknown == NULL);
  LONGS_EQUAL(0, unknown_count);

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
  int64_t int64_out = 0;
  uint64_t uint64_out = 0;
  double double_out = 0.0;
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
  CHECK(!ap_ns_get_int64(ns, "count", &int64_out));
  CHECK(!ap_ns_get_uint64(ns, "count", &uint64_out));
  CHECK(!ap_ns_get_double(ns, "count", &double_out));
  LONGS_EQUAL(-1, ap_ns_get_count(ns, "missing"));
  CHECK(ap_ns_get_string_at(ns, "missing", 0) == NULL);
  CHECK(ap_ns_get_string_at(ns, "name", 1) == NULL);
  CHECK(!ap_ns_get_int32_at(ns, "count", -1, &int_out));
  CHECK(!ap_ns_get_int32_at(ns, "count", 1, &int_out));
  CHECK(!ap_ns_get_int32_at(ns, "count", 0, NULL));
  CHECK(!ap_ns_get_int64_at(ns, "count", 0, &int64_out));
  CHECK(!ap_ns_get_uint64_at(ns, "count", 0, &uint64_out));
  CHECK(!ap_ns_get_double_at(ns, "count", 0, &double_out));

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(NamespaceUint64GetterHandlesBoundsUnsetTypeMismatchAndAppend) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options zero = ap_arg_options_default();
  ap_arg_options max = ap_arg_options_default();
  ap_arg_options mismatch = ap_arg_options_default();
  ap_arg_options values = ap_arg_options_default();
  uint64_t value = 0;
  uint64_t first = 0;
  uint64_t second = 0;
  char *argv[] = {
      (char *)"prog",
      (char *)"--zero",
      (char *)"0",
      (char *)"--max",
      (char *)"18446744073709551615",
      (char *)"--mismatch",
      (char *)"11",
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
  CHECK(ap_ns_get_uint64(ns, "zero", &value));
  LONGS_EQUAL(0ULL, value);
  CHECK(ap_ns_get_uint64(ns, "max", &value));
  LONGS_EQUAL(18446744073709551615ULL, value);
  CHECK(!ap_ns_get_uint64(ns, "mismatch", &value));
  CHECK(ap_ns_get_uint64(ns, "values", &value));
  CHECK(ap_ns_get_uint64_at(ns, "values", 0, &first));
  CHECK(ap_ns_get_uint64_at(ns, "values", 1, &second));
  LONGS_EQUAL(first, value);
  LONGS_EQUAL(0ULL, first);
  LONGS_EQUAL(18446744073709551615ULL, second);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(0, ap_parse_args(p, 1, argv_unset, &ns, &err));
  CHECK(!ap_ns_get_uint64(ns, "zero", &value));
  CHECK(!ap_ns_get_uint64(ns, "max", &value));
  CHECK(!ap_ns_get_uint64(ns, "values", &value));
  CHECK(!ap_ns_get_uint64_at(ns, "values", 0, &first));

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

  CHECK(ap_add_argument_group(NULL, "grp", "desc", &err) == NULL);
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser and title are required", err.message);
}

TEST(ParserCompletionApiRejectsMissingParser) {
  ap_error err = {};

  LONGS_EQUAL(-1, ap_parser_set_completion(NULL, true, "__complete", &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("", err.argument);
  STRCMP_EQUAL("parser is required", err.message);
}

TEST(ParserCompletionGettersReturnSafeDefaultsForNullParser) {
  CHECK(!ap_parser_completion_enabled(NULL));
  STRCMP_EQUAL("__complete", ap_parser_completion_entrypoint(NULL));
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

TEST(StoreTrueRejectsUnsupportedChoicesDefaultAndCustomNargs) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  const char *choices[] = {"on", "off"};

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  verbose.choices.items = choices;
  verbose.choices.count = 2;
  verbose.default_value = "on";
  verbose.nargs = AP_NARGS_OPTIONAL;

  LONGS_EQUAL(-1, ap_add_argument(p, "--verbose", verbose, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("--verbose", err.argument);
  STRCMP_EQUAL("store_true/store_false do not support "
               "choices/default_value/const_value/custom nargs",
               err.message);

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

TEST(ArgumentGroupApiRejectsInvalidInputs) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_argument_group *group = NULL;

  CHECK(p != NULL);

  CHECK(ap_add_argument_group(p, NULL, "desc", &err) == NULL);
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("parser and title are required", err.message);

  group = ap_add_argument_group(p, "output", "output controls", &err);
  CHECK(group != NULL);

  LONGS_EQUAL(-1, ap_argument_group_add_argument(
                      NULL, "--color", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("group is required", err.message);

  ap_parser_free(p);
}

TEST(ArgumentGroupAddArgumentParsesLikeRegularArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_argument_group *group = NULL;
  ap_arg_options color = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  const char *parsed_color = NULL;
  const char *parsed_target = NULL;
  char *argv[] = {(char *)"prog", (char *)"--color", (char *)"always",
                  (char *)"dst.txt", NULL};

  CHECK(p != NULL);
  group = ap_add_argument_group(p, "output", "output controls", &err);
  CHECK(group != NULL);

  color.help = "color mode";
  target.help = "target file";
  LONGS_EQUAL(0, ap_argument_group_add_argument(group, "--color", color, &err));
  LONGS_EQUAL(0, ap_argument_group_add_argument(group, "target", target, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 4, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "color", &parsed_color));
  CHECK(ap_ns_get_string(ns, "target", &parsed_target));
  STRCMP_EQUAL("always", parsed_color);
  STRCMP_EQUAL("dst.txt", parsed_target);

  ap_namespace_free(ns);
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

TEST(FormatBashCompletionEscapesChoiceBoundaryCharacters) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("my-prog.v1", "desc");
  ap_arg_options choice = ap_arg_options_default();
  const char *values[] = {"two words", "it's",       "x=y",       "lint.v2",
                          "check-x",   "semi;colon", "brackets[]"};
  char *script = NULL;

  CHECK(p != NULL);
  choice.choices.items = values;
  choice.choices.count = 7;

  LONGS_EQUAL(0, ap_add_argument(p, "--bash-choice", choice, &err));

  script = ap_format_bash_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "complete -F _my_prog_v1 'my-prog.v1'") != NULL);
  CHECK(strstr(script, "root:--bash-choice)") != NULL);
  CHECK(strstr(script, "'two words' 'it'\\''s' 'x=y' 'lint.v2' 'check-x' "
                       "'semi;colon' 'brackets[]'") != NULL);

  free(script);
  ap_parser_free(p);
}

TEST(FormatFishCompletionEscapesHelpAndChoiceBoundaryCharacters) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("my-prog.v1", "desc");
  ap_arg_options fish_opt = ap_arg_options_default();
  const char *choices[] = {"quote\"", "path\\dir", "$HOME", "line1\nline2"};
  char *script = NULL;
  const char *help_fragment =
      "-l fish-opt -d \"say \\\"hi\\\"\\\\\\$USER next\" -r -a "
      "'(__ap_my_prog_v1_value_choices root:--fish-opt)'";

  CHECK(p != NULL);
  fish_opt.help = "say \"hi\"\\$USER\nnext";
  fish_opt.choices.items = choices;
  fish_opt.choices.count = 4;

  LONGS_EQUAL(0, ap_add_argument(p, "--fish-opt", fish_opt, &err));

  script = ap_format_fish_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "complete -c \"my-prog.v1\" -f") != NULL);
  CHECK(strstr(script, "function __ap_my_prog_v1_parser_key") != NULL);
  CHECK(strstr(script, help_fragment) != NULL);
  CHECK(strstr(script, "case \"root:--fish-opt\"") != NULL);
  CHECK(strstr(script,
               "\"quote\\\"\" \"path\\\\dir\" \"\\$HOME\" \"line1 line2\"") !=
        NULL);

  free(script);
  ap_parser_free(p);
}

TEST(FormatManpageEscapesRoffBoundaryCharacters) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_parser *sub = NULL;
  ap_arg_options man_opt = ap_arg_options_default();
  const char *choices[] = {"-dash", "\\slash", ".dot", "'tick", "line1\nline2"};
  char *manpage = NULL;

  CHECK(p != NULL);
  man_opt.help = ".lead\n'quote\n-dash\\trail";
  man_opt.default_value = "-default\\value\n.second";
  man_opt.choices.items = choices;
  man_opt.choices.count = 5;

  LONGS_EQUAL(0, ap_add_argument(p, "--man-opt", man_opt, &err));
  sub = ap_add_subcommand(p, ".lint", "'sub\n-dash", &err);
  CHECK(sub != NULL);

  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, "\\&.lead") != NULL);
  CHECK(strstr(manpage, "\\&'quote") != NULL);
  CHECK(strstr(manpage, "\\-dash\\\\trail") != NULL);
  CHECK(strstr(manpage, "default: \\-default\\\\value") != NULL);
  CHECK(strstr(manpage, "\\&.second") != NULL);
  CHECK(strstr(manpage,
               "choices: \\-dash, \\\\slash, \\&.dot, \\&'tick, line1") !=
        NULL);
  CHECK(strstr(manpage, "line2") != NULL);
  CHECK(strstr(manpage, "\\&.lint") != NULL);
  CHECK(strstr(manpage, "\\&'sub") != NULL);
  CHECK(strstr(manpage, "\\-dash") != NULL);

  free(manpage);
  ap_parser_free(p);
}

TEST(FormatGeneratorsKeepDeepNestedCompletionAndManpageKeysAligned) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("my-prog.v1", "desc");
  ap_parser *build = NULL;
  ap_parser *lint = NULL;
  ap_parser *check = NULL;
  ap_parser *leaf = NULL;
  ap_arg_options leaf_mode = ap_arg_options_default();
  const char *leaf_choices[] = {"fast", "slow"};
  char *bash = NULL;
  char *fish = NULL;
  char *help = NULL;
  char *manpage = NULL;

  CHECK(p != NULL);
  leaf_mode.help = "leaf mode";
  leaf_mode.choices.items = leaf_choices;
  leaf_mode.choices.count = 2;

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
  help = ap_format_help(p);
  manpage = ap_format_manpage(p);

  CHECK(bash != NULL);
  CHECK(fish != NULL);
  CHECK(help != NULL);
  CHECK(manpage != NULL);

  CHECK(strstr(bash, "parser_subcommands='build-tools'") != NULL);
  CHECK(strstr(bash, "root/build-tools/lint.v2/check-x)") != NULL);
  CHECK(strstr(bash, "parser_subcommands='final.step'") != NULL);
  CHECK(strstr(bash, "root/build-tools/lint.v2/check-x/final.step)") != NULL);
  CHECK(strstr(bash, "parser_options='-h --help --leaf-mode'") != NULL);
  CHECK(strstr(bash,
               "root/build-tools/lint.v2/check-x/final.step:--leaf-mode)") !=
        NULL);
  CHECK(strstr(bash, "'fast' 'slow'") != NULL);

  CHECK(strstr(fish, "case \"root:build-tools\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools\"") != NULL);
  CHECK(strstr(fish, "case \"root/build-tools:lint.v2\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools/lint.v2\"") != NULL);
  CHECK(strstr(fish, "case \"root/build-tools/lint.v2:check-x\"") != NULL);
  CHECK(strstr(fish, "set key \"root/build-tools/lint.v2/check-x\"") != NULL);
  CHECK(strstr(fish, "case \"root/build-tools/lint.v2/check-x:final.step\"") !=
        NULL);
  CHECK(
      strstr(fish, "set key \"root/build-tools/lint.v2/check-x/final.step\"") !=
      NULL);
  CHECK(strstr(fish,
               "root/build-tools/lint.v2/check-x/final.step' -l leaf-mode -d "
               "\"leaf mode\" -r -a '(__ap_my_prog_v1_value_choices "
               "root/build-tools/lint.v2/check-x/final.step:--leaf-mode)'") !=
        NULL);

  CHECK(strstr(help, "usage: my-prog.v1") != NULL);
  CHECK(strstr(help, "build-tools") != NULL);
  CHECK(strstr(help, "build helpers") != NULL);

  CHECK(strstr(manpage, ".SH SYNOPSIS") != NULL);
  CHECK(strstr(manpage, "my\\-prog.v1 build\\-tools lint.v2 check\\-x [\\-h] "
                        "[SUBCOMMAND]") != NULL);
  CHECK(strstr(manpage, "final leaf") != NULL);
  CHECK(strstr(manpage,
               "my\\-prog.v1 build\\-tools lint.v2 check\\-x final.step [\\-h] "
               "[\\-\\-leaf\\-mode LEAF-MODE]") != NULL);

  free(bash);
  free(fish);
  free(help);
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

TEST(CompleteUsesPositionalCompletionMetadataAndCallback) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options format = ap_arg_options_default();
  ap_arg_options task = ap_arg_options_default();
  ap_completion_result result = {};
  const char *formats[] = {"json", "yaml"};
  static const char *const tasks[] = {"build", "bench", "bundle", nullptr};
  char arg0[] = "re";
  char *argv0[] = {arg0};
  char arg1[] = "report.txt";
  char arg2[] = "y";
  char *argv1[] = {arg1, arg2};
  char arg3[] = "report.txt";
  char arg4[] = "yaml";
  char arg5[] = "bu";
  char *argv2[] = {arg3, arg4, arg5};

  CHECK(p != NULL);
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  format.choices.items = formats;
  format.choices.count = 2;
  task.completion_callback = dynamic_exec_completion;
  task.completion_user_data = (void *)tasks;

  LONGS_EQUAL(0, ap_add_argument(p, "input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "format", format, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "task", task, &err));

  LONGS_EQUAL(0, ap_complete(p, 1, argv0, "bash", &result, &err));
  LONGS_EQUAL(0, result.count);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 2, argv1, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("json", result.items[0].value);
  STRCMP_EQUAL("yaml", result.items[1].value);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 3, argv2, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("build", result.items[0].value);
  STRCMP_EQUAL("bundle", result.items[1].value);
  ap_completion_result_free(&result);

  ap_parser_free(p);
}

TEST(CompleteHandlesInlineOptionValueAndDoubleDashPositionalMode) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options file = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  ap_completion_result result = {};
  const char *modes[] = {"fast", "slow"};
  const char *files[] = {"alpha.txt", "beta.txt"};
  char arg0[] = "alpha.txt";
  char arg1[] = "--mode=s";
  char arg2[] = "";
  char *argv_inline[] = {arg0, arg1, arg2};
  char arg3[] = "--";
  char arg4[] = "a";
  char *argv_positional[] = {arg3, arg4};
  char arg5[] = "--mode";
  char arg6[] = "fast";
  char arg7[] = "b";
  char *argv_after_value[] = {arg5, arg6, arg7};

  CHECK(p != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  file.choices.items = files;
  file.choices.count = 2;
  target.choices.items = modes;
  target.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "file", file, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", target, &err));

  LONGS_EQUAL(0, ap_complete(p, 3, argv_inline, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("fast", result.items[0].value);
  STRCMP_EQUAL("slow", result.items[1].value);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 2, argv_positional, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("alpha.txt", result.items[0].value);
  STRCMP_EQUAL("beta.txt", result.items[1].value);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 3, argv_after_value, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("alpha.txt", result.items[0].value);
  STRCMP_EQUAL("beta.txt", result.items[1].value);
  ap_completion_result_free(&result);

  ap_parser_free(p);
}

TEST(CompleteHandlesVeryLongInlineOptionNameWithoutOverflow) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_completion_result result = {};
  const char *modes[] = {"fast", "slow"};
  std::string long_flag = "--";
  std::string inline_with_value;
  std::string inline_with_empty_value;
  char trailing_empty[] = "";
  char *argv_scan[] = {NULL, NULL, trailing_empty};
  char *argv_last[] = {NULL, NULL};

  CHECK(p != NULL);
  long_flag.append(160, 'x');
  mode.choices.items = modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, long_flag.c_str(), mode, &err));

  inline_with_value = long_flag + "=f";
  inline_with_empty_value = long_flag + "=";
  argv_scan[0] = inline_with_value.data();
  argv_scan[1] = trailing_empty;
  argv_last[0] = trailing_empty;
  argv_last[1] = inline_with_empty_value.data();

  LONGS_EQUAL(0, ap_complete(p, 3, argv_scan, "bash", &result, &err));
  LONGS_EQUAL(0, result.count);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 2, argv_last, "bash", &result, &err));
  LONGS_EQUAL(0, result.count);
  ap_completion_result_free(&result);

  ap_parser_free(p);
}

TEST(CompleteAppliesCallbackScopeAcrossThreeToFiveSubcommandLevels) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *alpha = NULL;
  ap_parser *beta = NULL;
  ap_parser *gamma = NULL;
  ap_parser *delta = NULL;
  ap_parser *epsilon = NULL;
  ap_arg_options root_exec = ap_arg_options_default();
  ap_arg_options beta_exec = ap_arg_options_default();
  ap_arg_options epsilon_exec = ap_arg_options_default();
  ap_completion_result result = {};
  static const char *const root_commands[] = {"root-run", "root-report",
                                              nullptr};
  static const char *const beta_commands[] = {"beta-build", "beta-bench",
                                              nullptr};
  static const char *const epsilon_commands[] = {"leaf-lint", "leaf-link",
                                                 nullptr};
  char arg0[] = "--exec";
  char arg1[] = "root-r";
  char *argv_root[] = {arg0, arg1};
  char arg2[] = "alpha";
  char arg3[] = "beta";
  char arg4[] = "--exec";
  char arg5[] = "beta-b";
  char *argv_beta[] = {arg2, arg3, arg4, arg5};
  char arg6[] = "alpha";
  char arg7[] = "beta";
  char arg8[] = "gamma";
  char arg9[] = "delta";
  char arg10[] = "epsilon";
  char arg11[] = "--exec";
  char arg12[] = "leaf-l";
  char *argv_epsilon[] = {arg6, arg7, arg8, arg9, arg10, arg11, arg12};
  char arg13[] = "alpha";
  char arg14[] = "beta";
  char arg15[] = "gamma";
  char arg16[] = "--exec";
  char arg17[] = "leaf";
  char *argv_gamma[] = {arg13, arg14, arg15, arg16, arg17};

  CHECK(p != NULL);
  root_exec.completion_callback = dynamic_exec_completion;
  root_exec.completion_user_data = (void *)root_commands;
  beta_exec.completion_callback = dynamic_exec_completion;
  beta_exec.completion_user_data = (void *)beta_commands;
  epsilon_exec.completion_callback = dynamic_exec_completion;
  epsilon_exec.completion_user_data = (void *)epsilon_commands;

  LONGS_EQUAL(0, ap_add_argument(p, "--exec", root_exec, &err));
  alpha = ap_add_subcommand(p, "alpha", "alpha commands", &err);
  CHECK(alpha != NULL);
  beta = ap_add_subcommand(alpha, "beta", "beta commands", &err);
  CHECK(beta != NULL);
  gamma = ap_add_subcommand(beta, "gamma", "gamma commands", &err);
  CHECK(gamma != NULL);
  delta = ap_add_subcommand(gamma, "delta", "delta commands", &err);
  CHECK(delta != NULL);
  epsilon = ap_add_subcommand(delta, "epsilon", "epsilon commands", &err);
  CHECK(epsilon != NULL);
  LONGS_EQUAL(0, ap_add_argument(beta, "--exec", beta_exec, &err));
  LONGS_EQUAL(0, ap_add_argument(epsilon, "--exec", epsilon_exec, &err));

  LONGS_EQUAL(0, ap_complete(p, 2, argv_root, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("root-run", result.items[0].value);
  STRCMP_EQUAL("root-report", result.items[1].value);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 4, argv_beta, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("beta-build", result.items[0].value);
  STRCMP_EQUAL("beta-bench", result.items[1].value);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 7, argv_epsilon, "bash", &result, &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("leaf-lint", result.items[0].value);
  STRCMP_EQUAL("leaf-link", result.items[1].value);
  ap_completion_result_free(&result);

  LONGS_EQUAL(0, ap_complete(p, 5, argv_gamma, "bash", &result, &err));
  LONGS_EQUAL(0, result.count);
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

TEST(CompletionResultInitResetsFieldsAndSupportsAddAndFreeLifecycle) {
  ap_error err = {};
  ap_completion_result result = {};

  /* 1) Fill with dummy values (dirty state). */
  result.items = reinterpret_cast<ap_completion_candidate *>(0x1);
  result.count = 123;
  result.cap = 456;

  /* 2) Initialize and 3) verify normalized fields. */
  ap_completion_result_init(&result);
  CHECK(result.items == NULL);
  LONGS_EQUAL(0, result.count);
  LONGS_EQUAL(0, result.cap);

  /* 4) Ensure add still works after init. */
  LONGS_EQUAL(0, ap_completion_result_add(&result, "value", "help", &err));
  LONGS_EQUAL(1, result.count);
  STRCMP_EQUAL("value", result.items[0].value);
  STRCMP_EQUAL("help", result.items[0].help);

  /* 5) Verify cleanup path. */
  ap_completion_result_free(&result);
  CHECK(result.items == NULL);
  LONGS_EQUAL(0, result.count);
  LONGS_EQUAL(0, result.cap);
}

TEST(CompletionResultAddNoMemoryViaReallocDoesNotCorruptStateAndCanRetry) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }
  ap_error err = {};
  ap_completion_result result = {};

  LONGS_EQUAL(0, ap_completion_result_add(&result, "a", "first", &err));
  LONGS_EQUAL(1, result.count);

  test_alloc_fail_on_nth(2);
  result.cap = result.count;
  LONGS_EQUAL(-1, ap_completion_result_add(&result, "b", "second", &err));
  LONGS_EQUAL(AP_ERR_NO_MEMORY, err.code);
  LONGS_EQUAL(1, result.count);
  STRCMP_EQUAL("a", result.items[0].value);

  test_alloc_fail_disable();
  LONGS_EQUAL(0, ap_completion_result_add(&result, "b", "second", &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("b", result.items[1].value);

  ap_completion_result_free(&result);
}

TEST(CompletionResultAddNoMemoryViaStrdupCanRetryWithoutLeakLikeArtifacts) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }
  ap_error err = {};
  ap_completion_result result = {};

  LONGS_EQUAL(0, ap_completion_result_add(&result, "base", "help", &err));
  LONGS_EQUAL(1, result.count);

  test_alloc_fail_on_nth(2);
  LONGS_EQUAL(-1, ap_completion_result_add(&result, "value", "extra", &err));
  LONGS_EQUAL(AP_ERR_NO_MEMORY, err.code);
  LONGS_EQUAL(1, result.count);

  test_alloc_fail_disable();
  LONGS_EQUAL(0, ap_completion_result_add(&result, "value", "extra", &err));
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("value", result.items[1].value);
  STRCMP_EQUAL("extra", result.items[1].help);

  ap_completion_result_free(&result);
}

TEST(AddArgumentNoMemoryLeavesParserStateAndSucceedsOnRetry) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options opts = ap_arg_options_default();
  int defs_before = 0;
  bool saw_no_memory = false;
  ap_namespace *ns = NULL;
  char *argv[] = {(char *)"prog", (char *)"--name", (char *)"alice", NULL};
  const char *name = NULL;

  CHECK(p != NULL);
  defs_before = ((const ap_parser *)p)->defs_count;

  for (int nth = 1; nth <= 8; nth++) {
    test_alloc_fail_on_nth(nth);
    LONGS_EQUAL(-1, ap_add_argument(p, "--name", opts, &err));
    LONGS_EQUAL(defs_before, ((const ap_parser *)p)->defs_count);
    if (err.code == AP_ERR_NO_MEMORY) {
      saw_no_memory = true;
      break;
    }
  }
  CHECK(saw_no_memory);

  test_alloc_fail_disable();
  LONGS_EQUAL(0, ap_add_argument(p, "--name", opts, &err));
  LONGS_EQUAL(defs_before + 1, ((const ap_parser *)p)->defs_count);
  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "name", &name));
  STRCMP_EQUAL("alice", name);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseKnownArgsNoMemoryOnUnknownCopyClearsOutputsAndCanRetry) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }
  ap_error err = {};
  ap_parser *p = new_base_parser();
  ap_namespace *ns = (ap_namespace *)0x1;
  char **unknown = (char **)0x1;
  int unknown_count = 77;
  char *argv[] = {(char *)"prog",
                  (char *)"-t",
                  (char *)"hello",
                  (char *)"file.txt",
                  (char *)"--x",
                  (char *)"1",
                  NULL};

  CHECK(p != NULL);

  test_alloc_fail_on_nth(1);
  LONGS_EQUAL(
      -1, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(AP_ERR_NO_MEMORY, err.code);
  CHECK(ns == NULL);
  CHECK(unknown == NULL);
  LONGS_EQUAL(0, unknown_count);

  test_alloc_fail_disable();
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 6, argv, &ns, &unknown, &unknown_count, &err));
  LONGS_EQUAL(2, unknown_count);
  STRCMP_EQUAL("--x", unknown[0]);
  STRCMP_EQUAL("1", unknown[1]);

  ap_free_tokens(unknown, unknown_count);
  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParseArgsNoMemoryLeavesOutNsNullAndParserCanBeReused) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }

  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options name = ap_arg_options_default();
  ap_namespace *ns = NULL;
  char *argv[] = {(char *)"prog", (char *)"--name", (char *)"alice", NULL};
  const char *parsed_name = NULL;
  bool saw_no_memory = false;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "--name", name, &err));

  for (int nth = 1; nth <= 128; nth++) {
    test_alloc_fail_on_nth(nth);
    ns = (ap_namespace *)0x1;
    if (ap_parse_args(p, 3, argv, &ns, &err) == 0) {
      ap_namespace_free(ns);
      ns = NULL;
      continue;
    }
    CHECK(ns == NULL);
    if (err.code == AP_ERR_NO_MEMORY) {
      saw_no_memory = true;
      break;
    }
  }

  CHECK(saw_no_memory);

  test_alloc_fail_disable();
  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "name", &parsed_name));
  STRCMP_EQUAL("alice", parsed_name);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParserNewWithOptionsNoMemoryCanRetryAndRemainUsable) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }

  ap_parser_options options = ap_parser_options_default();
  ap_parser *p = NULL;
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_arg_options mode = ap_arg_options_default();
  const char *value = NULL;
  bool saw_allocation_failure = false;
  char *argv[] = {(char *)"prog", (char *)"--mode", (char *)"fast", NULL};

  options.completion_entrypoint = "__custom_complete";

  for (int nth = 1; nth <= 64; nth++) {
    test_alloc_fail_on_nth(nth);
    p = ap_parser_new_with_options("prog", "desc", options);
    if (p == NULL) {
      saw_allocation_failure = true;
      break;
    }
    ap_parser_free(p);
    p = NULL;
  }
  CHECK(saw_allocation_failure);

  test_alloc_fail_disable();
  p = ap_parser_new_with_options("prog", "desc", options);
  CHECK(p != NULL);
  STRCMP_EQUAL("__custom_complete", ap_parser_completion_entrypoint(p));

  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_parse_args(p, 3, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "mode", &value));
  STRCMP_EQUAL("fast", value);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ParserOptionsCanInheritArgumentsGroupsAndMutexRules) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *base = ap_parser_new("base", "base parser");
  ap_parser *child = NULL;
  ap_parser_options options = ap_parser_options_default();
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options json = ap_arg_options_default();
  ap_arg_options yaml = ap_arg_options_default();
  ap_mutually_exclusive_group *format = NULL;
  ap_argument_group *shared = NULL;
  bool verbose_value = false;
  bool json_value = false;
  char *argv_ok[] = {(char *)"child", (char *)"--verbose", (char *)"--json",
                     NULL};
  char *argv_conflict[] = {(char *)"child", (char *)"--json", (char *)"--yaml",
                           NULL};

  CHECK(base != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  verbose.help = "enable verbose mode";
  LONGS_EQUAL(0, ap_add_argument(base, "--verbose", verbose, &err));

  shared = ap_add_argument_group(base, "shared", "common options", &err);
  CHECK(shared != NULL);

  format = ap_add_mutually_exclusive_group(base, false, &err);
  CHECK(format != NULL);

  json.type = AP_TYPE_BOOL;
  json.action = AP_ACTION_STORE_TRUE;
  json.dest = "json";
  LONGS_EQUAL(0, ap_group_add_argument(format, "--json", json, &err));
  yaml.type = AP_TYPE_BOOL;
  yaml.action = AP_ACTION_STORE_TRUE;
  yaml.dest = "yaml";
  LONGS_EQUAL(0, ap_group_add_argument(format, "--yaml", yaml, &err));

  options.inherit_from = base;
  child = ap_parser_new_with_options("child", "child parser", options);
  CHECK(child != NULL);

  LONGS_EQUAL(0, ap_parse_args(child, 3, argv_ok, &ns, &err));
  CHECK(ap_ns_get_bool(ns, "verbose", &verbose_value));
  CHECK_TRUE(verbose_value);
  CHECK(ap_ns_get_bool(ns, "json", &json_value));
  CHECK_TRUE(json_value);
  ap_namespace_free(ns);
  ns = NULL;

  LONGS_EQUAL(-1, ap_parse_args(child, 3, argv_conflict, &ns, &err));

  ap_parser_free(child);
  ap_parser_free(base);
}

TEST(ParserInheritanceDefaultPolicyDisallowsOverride) {
  ap_error err = {};
  ap_parser_options options = ap_parser_options_default();
  ap_parser *base = ap_parser_new("base", "base parser");
  ap_parser *child = NULL;
  ap_arg_options mode = ap_arg_options_default();

  CHECK(base != NULL);
  LONGS_EQUAL(0, ap_add_argument(base, "--mode", mode, &err));

  options.inherit_from = base;
  child = ap_parser_new_with_options("child", "child parser", options);
  CHECK(child != NULL);

  mode.help = "child override";
  LONGS_EQUAL(-1, ap_add_argument(child, "--mode", mode, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("mode", err.argument);

  ap_parser_free(child);
  ap_parser_free(base);
}

TEST(ParserInheritanceReplacePolicyAllowsOverride) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser_options options = ap_parser_options_default();
  ap_parser *base = ap_parser_new("base", "base parser");
  ap_parser *child = NULL;
  ap_arg_options base_mode = ap_arg_options_default();
  ap_arg_options child_mode = ap_arg_options_default();
  const char *value = NULL;
  char *argv[] = {(char *)"child", (char *)"--mode", (char *)"fast", NULL};

  CHECK(base != NULL);
  base_mode.help = "base mode";
  LONGS_EQUAL(0, ap_add_argument(base, "--mode", base_mode, &err));

  options.inherit_from = base;
  options.conflict_policy = AP_PARSER_CONFLICT_REPLACE;
  child = ap_parser_new_with_options("child", "child parser", options);
  CHECK(child != NULL);

  child_mode.help = "child mode";
  child_mode.required = true;
  LONGS_EQUAL(0, ap_add_argument(child, "--mode", child_mode, &err));
  LONGS_EQUAL(0, ap_parse_args(child, 3, argv, &ns, &err));
  CHECK(ap_ns_get_string(ns, "mode", &value));
  STRCMP_EQUAL("fast", value);

  ap_namespace_free(ns);
  ap_parser_free(child);
  ap_parser_free(base);
}

TEST(ParserResolvePolicyReplacesPriorDefinitionAcrossApiViews) {
  ap_error err = {};
  ap_parser_options parser_options = ap_parser_options_default();
  ap_arg_options old_mode = ap_arg_options_default();
  ap_arg_options final_mode = ap_arg_options_default();
  ap_parser_info parser_info = {};
  ap_arg_info arg_info = {};
  ap_parser *parser = NULL;
  char *usage = NULL;
  char *help = NULL;

  parser_options.conflict_policy = AP_PARSER_CONFLICT_RESOLVE;
  parser = ap_parser_new_with_options("prog", "desc", parser_options);
  CHECK(parser != NULL);

  old_mode.help = "legacy mode";
  LONGS_EQUAL(0, ap_add_argument(parser, "--mode", old_mode, &err));

  final_mode.help = "final mode";
  final_mode.metavar = "STYLE";
  final_mode.required = true;
  final_mode.dest = "mode_final";
  LONGS_EQUAL(0, ap_add_argument(parser, "--mode", final_mode, &err));

  usage = ap_format_usage(parser);
  help = ap_format_help(parser);
  CHECK(usage != NULL);
  CHECK(help != NULL);
  CHECK(strstr(usage, " [--mode MODE]") == NULL);
  CHECK(strstr(usage, " --mode STYLE") != NULL);
  CHECK(strstr(help, "legacy mode") == NULL);
  CHECK(strstr(help, "final mode") != NULL);

  LONGS_EQUAL(0, ap_parser_get_info(parser, &parser_info));
  LONGS_EQUAL(2, parser_info.argument_count);
  LONGS_EQUAL(0, ap_parser_get_argument(parser, 1, &arg_info));
  STRCMP_EQUAL("mode_final", arg_info.dest);
  STRCMP_EQUAL("final mode", arg_info.help);
  STRCMP_EQUAL("STYLE", arg_info.metavar);
  CHECK_TRUE(arg_info.required);

  free(usage);
  free(help);
  ap_parser_free(parser);
}

TEST(AddSubcommandNoMemoryLeavesParentStateAndCanRetry) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }

  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *child = NULL;
  bool saw_no_memory = false;
  int count_before = 0;

  CHECK(p != NULL);
  count_before = ((const ap_parser *)p)->subcommands_count;

  for (int nth = 1; nth <= 16; nth++) {
    test_alloc_fail_on_nth(nth);
    child = ap_add_subcommand(p, "config", "config commands", &err);
    if (child == NULL && err.code == AP_ERR_NO_MEMORY) {
      saw_no_memory = true;
      LONGS_EQUAL(count_before, ((const ap_parser *)p)->subcommands_count);
      break;
    }
    if (child != NULL) {
      ap_parser_free(p);
      p = ap_parser_new("prog", "desc");
      CHECK(p != NULL);
      count_before = ((const ap_parser *)p)->subcommands_count;
    }
  }
  CHECK(saw_no_memory);

  test_alloc_fail_disable();
  child = ap_add_subcommand(p, "config", "config commands", &err);
  CHECK(child != NULL);
  LONGS_EQUAL(count_before + 1, ((const ap_parser *)p)->subcommands_count);
  CHECK(child->parent == p);

  ap_parser_free(p);
}

TEST(TryHandleCompletionNoMemoryCanRetryAndSucceed) {
  AllocFailGuard guard;
  if (!test_alloc_injection_available()) {
    CHECK_TRUE(true);
    return;
  }

  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  const char *modes[] = {"fast", "slow"};
  ap_completion_result result = {};
  int handled = 0;
  bool saw_no_memory = false;
  char arg0[] = "prog";
  char arg1[] = "__complete";
  char arg2[] = "--shell";
  char arg3[] = "bash";
  char arg4[] = "--";
  char arg5[] = "--mode";
  char arg6[] = "s";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6};

  CHECK(p != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  for (int nth = 1; nth <= 64; nth++) {
    test_alloc_fail_on_nth(nth);
    handled = 0;
    ap_completion_result_init(&result);
    if (ap_try_handle_completion(p, 7, argv, "bash", &handled, &result, &err) ==
            -1 &&
        err.code == AP_ERR_NO_MEMORY) {
      saw_no_memory = true;
      LONGS_EQUAL(1, handled);
      ap_completion_result_free(&result);
      break;
    }
    ap_completion_result_free(&result);
  }
  CHECK(saw_no_memory);

  test_alloc_fail_disable();
  handled = 0;
  ap_completion_result_init(&result);
  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 7, argv, "bash", &handled, &result, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("fast", result.items[0].value);
  STRCMP_EQUAL("slow", result.items[1].value);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionIsEnabledByDefaultAndHelperHandlesRequests) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_completion_result result = {};
  int handled = 0;
  const char *modes[] = {"fast", "slow"};
  char arg0[] = "prog";
  char arg1[] = "__complete";
  char arg2[] = "--shell";
  char arg3[] = "bash";
  char arg4[] = "--";
  char arg5[] = "--mode";
  char arg6[] = "s";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6};

  CHECK(p != NULL);
  CHECK(ap_parser_completion_enabled(p));
  STRCMP_EQUAL("__complete", ap_parser_completion_entrypoint(p));

  mode.choices.items = modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 7, argv, "fish", &handled, &result, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(2, result.count);
  STRCMP_EQUAL("fast", result.items[0].value);
  STRCMP_EQUAL("slow", result.items[1].value);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionHelperRejectsInvalidArguments) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_completion_result result = {};
  int handled = 7;
  char arg0[] = "prog";
  char *argv[] = {arg0};

  CHECK(p != NULL);

  LONGS_EQUAL(-1, ap_try_handle_completion(NULL, 1, argv, "bash", &handled,
                                           &result, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  LONGS_EQUAL(0, handled);

  LONGS_EQUAL(
      -1, ap_try_handle_completion(p, 1, argv, "bash", &handled, NULL, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  LONGS_EQUAL(0, handled);

  LONGS_EQUAL(-1, ap_try_handle_completion(p, 1, NULL, "bash", &handled,
                                           &result, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  LONGS_EQUAL(0, handled);

  ap_parser_free(p);
}

TEST(ParserCompletionHelperReturnsNoOpWhenCompletionDisabled) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_completion_result result = {};
  int handled = 9;
  char arg0[] = "prog";
  char arg1[] = "__complete";
  char *argv[] = {arg0, arg1};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parser_set_completion(p, false, NULL, &err));

  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 2, argv, "bash", &handled, &result, &err));
  LONGS_EQUAL(0, handled);
  LONGS_EQUAL(0, result.count);
  LONGS_EQUAL(0, err.code);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionHelperReturnsNoOpWhenEntrypointDoesNotMatch) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_completion_result result = {};
  int handled = 5;
  char arg0[] = "prog";
  char arg1[] = "run";
  char *argv[] = {arg0, arg1};

  CHECK(p != NULL);

  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 2, argv, "bash", &handled, &result, &err));
  LONGS_EQUAL(0, handled);
  LONGS_EQUAL(0, result.count);
  LONGS_EQUAL(0, err.code);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionHelperTracksHandledAcrossShellAndSeparatorVariants) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_completion_result with_shell_and_separator = {};
  ap_completion_result with_shell_without_separator = {};
  ap_completion_result without_shell_with_separator = {};
  ap_completion_result without_shell_without_separator = {};
  int handled = -1;
  char a0[] = "prog";
  char a1[] = "__complete";
  char a2[] = "--shell";
  char a3[] = "fish";
  char a4[] = "--";
  char *argv_with_shell_and_separator[] = {a0, a1, a2, a3, a4};
  char b0[] = "prog";
  char b1[] = "__complete";
  char b2[] = "--shell";
  char b3[] = "fish";
  char *argv_with_shell_without_separator[] = {b0, b1, b2, b3};
  char c0[] = "prog";
  char c1[] = "__complete";
  char c2[] = "--";
  char *argv_without_shell_with_separator[] = {c0, c1, c2};
  char d0[] = "prog";
  char d1[] = "__complete";
  char *argv_without_shell_without_separator[] = {d0, d1};

  CHECK(p != NULL);

  handled = -1;
  LONGS_EQUAL(0, ap_try_handle_completion(p, 5, argv_with_shell_and_separator,
                                          "bash", &handled,
                                          &with_shell_and_separator, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(0, with_shell_and_separator.count);
  LONGS_EQUAL(0, err.code);

  handled = -1;
  LONGS_EQUAL(0, ap_try_handle_completion(
                     p, 4, argv_with_shell_without_separator, "bash", &handled,
                     &with_shell_without_separator, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(0, with_shell_without_separator.count);
  LONGS_EQUAL(0, err.code);

  handled = -1;
  LONGS_EQUAL(0, ap_try_handle_completion(
                     p, 3, argv_without_shell_with_separator, "bash", &handled,
                     &without_shell_with_separator, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(0, without_shell_with_separator.count);
  LONGS_EQUAL(0, err.code);

  handled = -1;
  LONGS_EQUAL(0, ap_try_handle_completion(
                     p, 2, argv_without_shell_without_separator, "bash",
                     &handled, &without_shell_without_separator, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(0, without_shell_without_separator.count);
  LONGS_EQUAL(0, err.code);

  ap_completion_result_free(&with_shell_and_separator);
  ap_completion_result_free(&with_shell_without_separator);
  ap_completion_result_free(&without_shell_with_separator);
  ap_completion_result_free(&without_shell_without_separator);
  ap_parser_free(p);
}

TEST(ParserCompletionCanBeDisabledAndHelperIgnoresHiddenEntrypoint) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_completion_result result = {};
  int handled = 0;
  char arg0[] = "prog";
  char arg1[] = "__complete";
  char *argv[] = {arg0, arg1};

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_parser_set_completion(p, false, NULL, &err));
  CHECK(!ap_parser_completion_enabled(p));

  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 2, argv, "bash", &handled, &result, &err));
  LONGS_EQUAL(0, handled);
  LONGS_EQUAL(0, result.count);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionHelperFallsBackToDefaultShellWhenShellValueMissing) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_completion_result result = {};
  int handled = 0;
  const char *modes[] = {"fast", "slow"};
  char arg0[] = "prog";
  char arg1[] = "__complete";
  char arg2[] = "--shell";
  char *argv[] = {arg0, arg1, arg2};

  CHECK(p != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 3, argv, "fish", &handled, &result, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(0, result.count);
  LONGS_EQUAL(0, err.code);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionHelperUsesBashWhenDefaultShellIsNull) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_completion_result result = {};
  int handled = 0;
  const char *modes[] = {"fast", "slow"};
  char arg0[] = "prog";
  char arg1[] = "__complete";
  char arg2[] = "--";
  char arg3[] = "--mode";
  char arg4[] = "s";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};

  CHECK(p != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(
      0, ap_try_handle_completion(p, 5, argv, NULL, &handled, &result, &err));
  LONGS_EQUAL(1, handled);
  LONGS_EQUAL(2, result.count);
  LONGS_EQUAL(0, err.code);
  STRCMP_EQUAL("fast", result.items[0].value);
  STRCMP_EQUAL("slow", result.items[1].value);

  ap_completion_result_free(&result);
  ap_parser_free(p);
}

TEST(ParserCompletionHelperAcceptsCompletionTargetWithoutDoubleDash) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_completion_result with_separator = {};
  ap_completion_result without_separator = {};
  int handled_with_separator = 0;
  int handled_without_separator = 0;
  const char *modes[] = {"fast", "slow"};
  char sep0[] = "prog";
  char sep1[] = "__complete";
  char sep2[] = "--shell";
  char sep3[] = "bash";
  char sep4[] = "--";
  char sep5[] = "--mode";
  char sep6[] = "s";
  char *argv_with_separator[] = {sep0, sep1, sep2, sep3, sep4, sep5, sep6};
  char nsep0[] = "prog";
  char nsep1[] = "__complete";
  char nsep2[] = "--shell";
  char nsep3[] = "bash";
  char nsep4[] = "--mode";
  char nsep5[] = "s";
  char *argv_without_separator[] = {nsep0, nsep1, nsep2, nsep3, nsep4, nsep5};

  CHECK(p != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  LONGS_EQUAL(0, ap_try_handle_completion(p, 7, argv_with_separator, "fish",
                                          &handled_with_separator,
                                          &with_separator, &err));
  LONGS_EQUAL(1, handled_with_separator);
  LONGS_EQUAL(2, with_separator.count);
  STRCMP_EQUAL("fast", with_separator.items[0].value);
  STRCMP_EQUAL("slow", with_separator.items[1].value);
  LONGS_EQUAL(0, err.code);

  LONGS_EQUAL(0, ap_try_handle_completion(p, 6, argv_without_separator, "fish",
                                          &handled_without_separator,
                                          &without_separator, &err));
  LONGS_EQUAL(1, handled_without_separator);
  LONGS_EQUAL(2, without_separator.count);
  STRCMP_EQUAL("fast", without_separator.items[0].value);
  STRCMP_EQUAL("slow", without_separator.items[1].value);
  STRCMP_EQUAL(with_separator.items[0].value, without_separator.items[0].value);
  STRCMP_EQUAL(with_separator.items[1].value, without_separator.items[1].value);
  LONGS_EQUAL(0, err.code);

  ap_completion_result_free(&with_separator);
  ap_completion_result_free(&without_separator);
  ap_parser_free(p);
}

TEST(NamespaceGettersSupportInt64AndDoubleArrays) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options ids = ap_arg_options_default();
  ap_arg_options weights = ap_arg_options_default();
  int64_t id0 = 0;
  int64_t id1 = 0;
  double weight0 = 0.0;
  double weight1 = 0.0;
  char *argv[] = {(char *)"prog", (char *)"--ids",
                  (char *)"7",    (char *)"--ids",
                  (char *)"9",    (char *)"--weights",
                  (char *)"0.5",  (char *)"--weights",
                  (char *)"1.5",  NULL};

  CHECK(p != NULL);
  ids.type = AP_TYPE_INT64;
  ids.action = AP_ACTION_APPEND;
  weights.type = AP_TYPE_DOUBLE;
  weights.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "--ids", ids, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--weights", weights, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 9, argv, &ns, &err));
  CHECK(ap_ns_get_int64_at(ns, "ids", 0, &id0));
  CHECK(ap_ns_get_int64_at(ns, "ids", 1, &id1));
  CHECK(ap_ns_get_double_at(ns, "weights", 0, &weight0));
  CHECK(ap_ns_get_double_at(ns, "weights", 1, &weight1));
  LONGS_EQUAL(7LL, id0);
  LONGS_EQUAL(9LL, id1);
  CHECK(std::fabs(weight0 - 0.5) < 1e-12);
  CHECK(std::fabs(weight1 - 1.5) < 1e-12);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(DefinitionErrorsRejectUnsupportedActionTypeCombinations) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options count = ap_arg_options_default();
  ap_arg_options store_const = ap_arg_options_default();

  CHECK(p != NULL);
  count.type = AP_TYPE_INT32;
  count.action = AP_ACTION_COUNT;
  count.default_value = "1";
  LONGS_EQUAL(-1, ap_add_argument(p, "--count", count, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("count action does not support "
               "choices/default_value/const_value/custom nargs",
               err.message);

  store_const.type = AP_TYPE_DOUBLE;
  store_const.action = AP_ACTION_STORE_CONST;
  store_const.const_value = "3.14";
  store_const.choices.items = (const char **)&store_const.const_value;
  store_const.choices.count = 1;
  LONGS_EQUAL(-1, ap_add_argument(p, "--pi", store_const, &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL(
      "store_const does not support default_value/choices/custom nargs",
      err.message);

  ap_parser_free(p);
}

TEST(NamespaceGettersSupportUint64) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options sizes = ap_arg_options_default();
  uint64_t size0 = 0;
  uint64_t size1 = 0;
  char *argv[] = {(char *)"prog",    (char *)"--sizes", (char *)"1",
                  (char *)"--sizes", (char *)"2",       NULL};

  CHECK(p != NULL);
  sizes.type = AP_TYPE_UINT64;
  sizes.action = AP_ACTION_APPEND;
  LONGS_EQUAL(0, ap_add_argument(p, "--sizes", sizes, &err));

  LONGS_EQUAL(0, ap_parse_args(p, 5, argv, &ns, &err));
  CHECK(ap_ns_get_uint64_at(ns, "sizes", 0, &size0));
  CHECK(ap_ns_get_uint64_at(ns, "sizes", 1, &size1));
  LONGS_EQUAL(1ULL, size0);
  LONGS_EQUAL(2ULL, size1);

  ap_namespace_free(ns);
  ap_parser_free(p);
}
