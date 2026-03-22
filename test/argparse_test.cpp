#include <stdlib.h>
#include <string.h>

#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestHarness.h"

extern "C" {
#include "argparse-c.h"
}

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

TEST_GROUP(ArgparseC) {
  void teardown() {}
};

TEST(ArgparseC, SuccessParse) {
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

TEST(ArgparseC, UnknownOption) {
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

TEST(ArgparseC, InvalidInt) {
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

TEST(ArgparseC, ChoicesValidation) {
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

TEST(ArgparseC, NargsOneOrMore) {
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

TEST(ArgparseC, DefaultValue) {
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

TEST(ArgparseC, AutoDestPrefersLongFlagAndNormalizesHyphen) {
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

TEST(ArgparseC, AutoDestNormalizesPositionalHyphen) {
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

TEST(ArgparseC, ExplicitDestIsNotNormalized) {
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

TEST(ArgparseC, HelpGeneration) {
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

TEST(ArgparseC, InlineOptionValue) {
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

TEST(ArgparseC, RequiredOptionIgnoresDefaultWhenMissing) {
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

TEST(ArgparseC, DefaultValueMustMatchChoices) {
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

TEST(ArgparseC, StoreTrueAndStoreFalse) {
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

TEST(ArgparseC, StoreTrueRejectsInlineValue) {
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

TEST(ArgparseC, HelpShowsChoicesDefaultAndRequired) {
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

TEST(ArgparseC, FormatErrorIncludesMessageAndUsage) {
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

TEST(ArgparseC, ParseKnownArgsCollectsUnknownOptions) {
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

TEST(ArgparseC, ParseKnownArgsStillValidatesRequired) {
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

TEST(ArgparseC, ParseKnownArgsCollectsUnknownOptionValueToken) {
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

TEST(ArgparseC, ParseKnownArgsCollectsExtraPositionals) {
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

TEST(ArgparseC, ParseKnownArgsCollectsAfterDoubleDash) {
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

TEST(ArgparseC, ShortOptionClusterForBoolFlags) {
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

TEST(ArgparseC, ShortOptionClusterRejectsValueOption) {
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

TEST(ArgparseC, ParseSubcommandArguments) {
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

TEST(ArgparseC, HelpListsSubcommands) {
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

TEST(ArgparseC, ParseNestedSubcommandArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *config = NULL;
  ap_parser *set = NULL;
  ap_arg_options global = ap_arg_options_default();
  ap_arg_options value = ap_arg_options_default();
  const char *subcommand = NULL;
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
  STRCMP_EQUAL("set", subcommand);
  CHECK_TRUE(is_global);
  STRCMP_EQUAL("blue", value_text);
  STRCMP_EQUAL("theme", key);

  ap_namespace_free(ns);
  ap_parser_free(p);
}

TEST(ArgparseC, NestedSubcommandHelpUsesFullCommandPath) {
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

TEST(ArgparseC, MissingSubcommandFails) {
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

TEST(ArgparseC, CountActionAccumulatesShortFlags) {
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

TEST(ArgparseC, AppendActionCollectsRepeatedOptionValues) {
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

TEST(ArgparseC, StoreConstUsesConstValue) {
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

TEST(ArgparseC, FixedNargsForOption) {
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

TEST(ArgparseC, MutuallyExclusiveGroupRejectsConflicts) {
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

TEST(ArgparseC, RequiredMutuallyExclusiveGroupNeedsOneOption) {
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

TEST(ArgparseC, MissingRequiredPositionalUsesConsistentArgumentNameAndMessage) {
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

int main(int ac, char **av) { return CommandLineTestRunner::RunAllTests(ac, av); }
