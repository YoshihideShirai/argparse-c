#include "test_framework.h"

TEST(ParseSubcommandArguments) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *run = NULL;
  ap_arg_options config = ap_arg_options_default();
  ap_arg_options force = ap_arg_options_default();
  const char *subcommand = NULL;
  const char *subcommand_path = NULL;
  const char *config_value = NULL;
  bool force_enabled = false;
  char *argv[] = {(char *)"prog",      (char *)"run",     (char *)"--config",
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
  CHECK(ap_ns_get_string(ns, "subcommand_path", &subcommand_path));
  CHECK(ap_ns_get_string(ns, "config", &config_value));
  CHECK(ap_ns_get_bool(ns, "force", &force_enabled));
  STRCMP_EQUAL("run", subcommand);
  STRCMP_EQUAL("run", subcommand_path);
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
  char *argv[] = {
      (char *)"prog",    (char *)"config", (char *)"set",   (char *)"--global",
      (char *)"--value", (char *)"blue",   (char *)"theme", NULL};

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
  CHECK(ap_ns_get_string(ns, "subcommand_path", &subcommand_path));
  CHECK(!ap_ns_get_string(ns, "config", &parent_subcommand));
  STRCMP_EQUAL("set", subcommand);
  STRCMP_EQUAL("config set", subcommand_path);
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
  LONGS_EQUAL(
      -1, ap_add_argument(p, "-v, verbose", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("verbose", err.argument);
  STRCMP_EQUAL("optional argument flags must start with '-'", err.message);

  ap_parser_free(p);
}

TEST(PositionalDeclarationRejectsMultipleTokens) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  CHECK(p != NULL);
  LONGS_EQUAL(
      -1, ap_add_argument(p, "input, output", ap_arg_options_default(), &err));
  LONGS_EQUAL(AP_ERR_INVALID_DEFINITION, err.code);
  STRCMP_EQUAL("input, output", err.argument);
  STRCMP_EQUAL("positional argument must be a single non-flag token",
               err.message);

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

  LONGS_EQUAL(-1, ap_group_add_argument(NULL, "--json",
                                        ap_arg_options_default(), &err));
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
  char *argv[] = {(char *)"prog",   (char *)"--name", (char *)"a",
                  (char *)"--name", (char *)"b",      NULL};

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
  char *argv[] = {(char *)"prog",  (char *)"-xz",      (char *)"-t",
                  (char *)"hello", (char *)"file.txt", NULL};

  CHECK(p != NULL);
  LONGS_EQUAL(
      0, ap_parse_known_args(p, 5, argv, &ns, &unknown, &unknown_count, &err));
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
