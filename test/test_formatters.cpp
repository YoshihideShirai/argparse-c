#include "test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "../lib/ap_internal.h"
int ap_test_sb_before_appendf(void);
int ap_test_sb_before_free(void);
#ifdef __cplusplus
}
#endif

#include <array>
#include <cmath>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

struct StringBuilderHookState {
  int appendf_fail_on_call = -1;
  int free_fail_on_call = -1;
  int appendf_call_count = 0;
  int free_call_count = 0;
  int free_fail_hit_count = 0;
};

StringBuilderHookState g_sb_hook_state;

void reset_sb_hooks() { g_sb_hook_state = {}; }

void fail_appendf_on_call(int call_index) {
  g_sb_hook_state.appendf_fail_on_call = call_index;
}

void fail_free_on_call(int call_index) {
  g_sb_hook_state.free_fail_on_call = call_index;
}

int successful_manpage_append_count(ap_parser *parser) {
  char *manpage = NULL;
  int append_count = 0;

  reset_sb_hooks();
  manpage = ap_format_manpage(parser);
  CHECK(manpage != NULL);
  append_count = g_sb_hook_state.appendf_call_count;
  free(manpage);
  reset_sb_hooks();

  return append_count;
}

int successful_usage_append_count(ap_parser *parser) {
  char *usage = NULL;
  int append_count = 0;

  reset_sb_hooks();
  usage = ap_format_usage(parser);
  CHECK(usage != NULL);
  append_count = g_sb_hook_state.appendf_call_count;
  free(usage);
  reset_sb_hooks();

  return append_count;
}

int successful_help_append_count(ap_parser *parser) {
  char *help = NULL;
  int append_count = 0;

  reset_sb_hooks();
  help = ap_format_help(parser);
  CHECK(help != NULL);
  append_count = g_sb_hook_state.appendf_call_count;
  free(help);
  reset_sb_hooks();

  return append_count;
}

int successful_bash_completion_append_count(ap_parser *parser) {
  char *script = NULL;
  int append_count = 0;

  reset_sb_hooks();
  script = ap_format_bash_completion(parser);
  CHECK(script != NULL);
  append_count = g_sb_hook_state.appendf_call_count;
  free(script);
  reset_sb_hooks();

  return append_count;
}

int successful_fish_completion_append_count(ap_parser *parser) {
  char *script = NULL;
  int append_count = 0;

  reset_sb_hooks();
  script = ap_format_fish_completion(parser);
  CHECK(script != NULL);
  append_count = g_sb_hook_state.appendf_call_count;
  free(script);
  reset_sb_hooks();

  return append_count;
}

int successful_zsh_completion_append_count(ap_parser *parser) {
  char *script = NULL;
  int append_count = 0;

  reset_sb_hooks();
  script = ap_format_zsh_completion(parser);
  CHECK(script != NULL);
  append_count = g_sb_hook_state.appendf_call_count;
  free(script);
  reset_sb_hooks();

  return append_count;
}

void assert_manpage_append_failures_return_null(ap_parser *parser) {
  int append_count = successful_manpage_append_count(parser);

  CHECK(append_count > 0);
  for (int i = 1; i <= append_count; i++) {
    reset_sb_hooks();
    fail_appendf_on_call(i);
    CHECK(ap_format_manpage(parser) == NULL);
    CHECK(g_sb_hook_state.free_call_count > 0);
  }
  reset_sb_hooks();
}

void assert_usage_append_failures_return_null(ap_parser *parser) {
  int append_count = successful_usage_append_count(parser);

  CHECK(append_count > 0);
  for (int i = 1; i <= append_count; i++) {
    reset_sb_hooks();
    fail_appendf_on_call(i);
    CHECK(ap_format_usage(parser) == NULL);
    CHECK(g_sb_hook_state.free_call_count > 0);
  }
  reset_sb_hooks();
}

void assert_help_append_failures_return_null(ap_parser *parser) {
  int append_count = successful_help_append_count(parser);

  CHECK(append_count > 0);
  for (int i = 1; i <= append_count; i++) {
    reset_sb_hooks();
    fail_appendf_on_call(i);
    CHECK(ap_format_help(parser) == NULL);
    CHECK(g_sb_hook_state.free_call_count > 0);
  }
  reset_sb_hooks();
}

void assert_bash_completion_append_failures_return_null(ap_parser *parser) {
  int append_count = successful_bash_completion_append_count(parser);

  CHECK(append_count > 0);
  for (int i = 1; i <= append_count; i++) {
    reset_sb_hooks();
    fail_appendf_on_call(i);
    CHECK(ap_format_bash_completion(parser) == NULL);
    CHECK(g_sb_hook_state.free_call_count > 0);
  }
  reset_sb_hooks();
}

void assert_fish_completion_append_failures_return_null(ap_parser *parser) {
  int append_count = successful_fish_completion_append_count(parser);

  CHECK(append_count > 0);
  for (int i = 1; i <= append_count; i++) {
    reset_sb_hooks();
    fail_appendf_on_call(i);
    CHECK(ap_format_fish_completion(parser) == NULL);
    CHECK(g_sb_hook_state.free_call_count > 0);
  }
  reset_sb_hooks();
}

void assert_zsh_completion_append_failures_return_null(ap_parser *parser) {
  int append_count = successful_zsh_completion_append_count(parser);

  CHECK(append_count > 0);
  for (int i = 1; i <= append_count; i++) {
    reset_sb_hooks();
    fail_appendf_on_call(i);
    CHECK(ap_format_zsh_completion(parser) == NULL);
    CHECK(g_sb_hook_state.free_call_count > 0);
  }
  reset_sb_hooks();
}

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

extern "C" int ap_test_sb_before_appendf(void) {
  g_sb_hook_state.appendf_call_count++;
  return g_sb_hook_state.appendf_fail_on_call >= 0 &&
                 g_sb_hook_state.appendf_call_count ==
                     g_sb_hook_state.appendf_fail_on_call
             ? 1
             : 0;
}

extern "C" int ap_test_sb_before_free(void) {
  g_sb_hook_state.free_call_count++;
  if (g_sb_hook_state.free_fail_on_call >= 0 &&
      g_sb_hook_state.free_call_count == g_sb_hook_state.free_fail_on_call) {
    g_sb_hook_state.free_fail_hit_count++;
    return 1;
  }
  return 0;
}

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

TEST(FormatManpageForParserWithoutOptionsOrSubcommandsUsesFallbackSections) {
  ap_parser *p = ap_parser_new("tool", "");
  char *manpage = NULL;

  CHECK(p != NULL);
  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, ".SH DESCRIPTION\n") != NULL);
  CHECK(strstr(manpage, "Command line interface generated by argparse\\-c.") !=
        NULL);
  CHECK(strstr(manpage, ".SH OPTIONS\n") != NULL);
  CHECK(strstr(manpage, "\\-h, \\-\\-help") != NULL);
  CHECK(strstr(manpage, ".SH SUBCOMMANDS\n") != NULL);
  CHECK(strstr(manpage, "This command does not define subcommands.\n") != NULL);

  free(manpage);
  ap_parser_free(p);
}

TEST(FormatManpageCoversAllOptionalNargsSuffixVariants) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options one = ap_arg_options_default();
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options many = ap_arg_options_default();
  ap_arg_options plus = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  char *manpage = NULL;

  CHECK(p != NULL);
  one.metavar = "ONE";
  maybe.nargs = AP_NARGS_OPTIONAL;
  maybe.metavar = "MAYBE";
  many.nargs = AP_NARGS_ZERO_OR_MORE;
  many.metavar = "MANY";
  plus.nargs = AP_NARGS_ONE_OR_MORE;
  plus.metavar = "PLUS";
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  pair.metavar = "PAIR";

  LONGS_EQUAL(0, ap_add_argument(p, "--one", one, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--many", many, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--plus", plus, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--pair", pair, &err));

  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, "\\-\\-one ONE") != NULL);
  CHECK(strstr(manpage, "\\-\\-maybe [MAYBE]") != NULL);
  CHECK(strstr(manpage, "\\-\\-many [MANY ...]") != NULL);
  CHECK(strstr(manpage, "\\-\\-plus PLUS [PLUS ...]") != NULL);
  CHECK(strstr(manpage, "\\-\\-pair PAIR PAIR") != NULL);

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

TEST(FormatHelpIncludesPositionalMetadataAndFallbackMetavarTransform) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options user = ap_arg_options_default();
  const char *choices[] = {"u-1", "u-2"};
  char *help = NULL;

  CHECK(p != NULL);
  user.nargs = AP_NARGS_OPTIONAL;
  user.help = "user identifier";
  user.default_value = "u-1";
  user.required = true;
  user.choices.items = choices;
  user.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "user_id2", user, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "  USER-ID2 [USER-ID2]") != NULL);
  CHECK(strstr(help, "user identifier") != NULL);
  CHECK(strstr(help, "choices: u-1, u-2") != NULL);
  CHECK(strstr(help, "default: u-1") != NULL);
  CHECK(strstr(help, "required: true") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatHelpFormatterModeSwitchAndDefaultsSnapshot) {
  ap_error err = {};
  ap_parser_options options = ap_parser_options_default();
  ap_parser *standard = NULL;
  ap_parser *show_defaults = NULL;
  ap_parser *raw = NULL;
  ap_arg_options mode = ap_arg_options_default();
  char *standard_help = NULL;
  char *show_defaults_help = NULL;
  char *raw_help = NULL;
  const char *expected_raw = "usage: tool [-h] [--mode MODE]\n"
                             "\ndesc\n"
                             "\noptional arguments:\n"
                             "  -h, --help - show this help message and exit\n"
                             "  --mode MODE - mode value | default: fast\n";

  mode.help = "mode value";
  mode.default_value = "fast";

  standard = ap_parser_new("tool", "desc");
  CHECK(standard != NULL);
  LONGS_EQUAL(0, ap_add_argument(standard, "--mode", mode, &err));
  standard_help = ap_format_help(standard);
  CHECK(standard_help != NULL);
  CHECK(strstr(standard_help, "\n    default: fast\n") != NULL);

  options.help_formatter_mode = AP_HELP_FORMATTER_SHOW_DEFAULTS;
  show_defaults = ap_parser_new_with_options("tool", "desc", options);
  CHECK(show_defaults != NULL);
  LONGS_EQUAL(0, ap_add_argument(show_defaults, "--mode", mode, &err));
  show_defaults_help = ap_format_help(show_defaults);
  CHECK(show_defaults_help != NULL);
  STRCMP_EQUAL(standard_help, show_defaults_help);

  options.help_formatter_mode = AP_HELP_FORMATTER_RAW_TEXT;
  raw = ap_parser_new_with_options("tool", "desc", options);
  CHECK(raw != NULL);
  LONGS_EQUAL(0, ap_add_argument(raw, "--mode", mode, &err));
  raw_help = ap_format_help(raw);
  CHECK(raw_help != NULL);
  STRCMP_EQUAL(expected_raw, raw_help);

  free(raw_help);
  free(show_defaults_help);
  free(standard_help);
  ap_parser_free(raw);
  ap_parser_free(show_defaults);
  ap_parser_free(standard);
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

TEST(FormatUsageCoversFixedNargsForRequiredAndOptionalOptions) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options required_pair = ap_arg_options_default();
  ap_arg_options optional_triple = ap_arg_options_default();
  char *usage = NULL;

  CHECK(p != NULL);
  required_pair.nargs = AP_NARGS_FIXED;
  required_pair.nargs_count = 2;
  required_pair.required = true;
  required_pair.metavar = "PAIR";
  optional_triple.nargs = AP_NARGS_FIXED;
  optional_triple.nargs_count = 3;
  optional_triple.required = false;
  optional_triple.metavar = "TRIPLE";

  LONGS_EQUAL(0, ap_add_argument(p, "--pair", required_pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--triple", optional_triple, &err));

  usage = ap_format_usage(p);
  CHECK(usage != NULL);
  CHECK(strstr(usage, " --pair PAIR PAIR") != NULL);
  CHECK(strstr(usage, " [--triple TRIPLE TRIPLE TRIPLE]") != NULL);

  free(usage);
  ap_parser_free(p);
}

TEST(FormatUsageCoversRequiredOptionalNargsVariantsAndSubcommandMarker) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options many = ap_arg_options_default();
  ap_arg_options plus = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  char *usage = NULL;

  CHECK(p != NULL);
  maybe.required = true;
  maybe.nargs = AP_NARGS_OPTIONAL;
  maybe.metavar = "MAYBE";
  many.required = true;
  many.nargs = AP_NARGS_ZERO_OR_MORE;
  many.metavar = "MANY";
  plus.required = true;
  plus.nargs = AP_NARGS_ONE_OR_MORE;
  plus.metavar = "PLUS";
  pair.required = true;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  pair.metavar = "PAIR";

  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--many", many, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--plus", plus, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--pair", pair, &err));
  CHECK(ap_add_subcommand(p, "run", "run command", &err) != NULL);

  usage = ap_format_usage(p);
  CHECK(usage != NULL);
  CHECK(strstr(usage, " --maybe [MAYBE]") != NULL);
  CHECK(strstr(usage, " --many [MANY ...]") != NULL);
  CHECK(strstr(usage, " --plus PLUS [PLUS ...]") != NULL);
  CHECK(strstr(usage, " --pair PAIR PAIR") != NULL);
  CHECK(strstr(usage, " <subcommand>") != NULL);

  free(usage);
  ap_parser_free(p);
}

TEST(FormatManpageEscapesRoffControlCharactersAndBackslashes) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", ".dot\n'quote\npath\\name-with-dash");
  ap_arg_options arg = ap_arg_options_default();
  char *manpage = NULL;

  CHECK(p != NULL);
  arg.help = ".flag-help\n'flag-help\npath\\flag-with-dash";
  LONGS_EQUAL(0, ap_add_argument(p, "--flag", arg, &err));

  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, "\\&.dot") != NULL);
  CHECK(strstr(manpage, "\\&'quote") != NULL);
  CHECK(strstr(manpage, "path\\\\name\\-with\\-dash") != NULL);
  CHECK(strstr(manpage, "\\&.flag\\-help") != NULL);
  CHECK(strstr(manpage, "\\&'flag\\-help") != NULL);
  CHECK(strstr(manpage, "path\\\\flag\\-with\\-dash") != NULL);

  free(manpage);
  ap_parser_free(p);
}

TEST(FormatHelpWithoutPositionalsDoesNotEmitPositionalSection) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options verbose = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  verbose.type = AP_TYPE_BOOL;
  verbose.action = AP_ACTION_STORE_TRUE;
  LONGS_EQUAL(0, ap_add_argument(p, "--verbose", verbose, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "positional arguments:\n") == NULL);
  CHECK(strstr(help, "optional arguments:\n") != NULL);
  CHECK(strstr(help, "--verbose") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatHelpWithoutSubcommandsDoesNotEmitSubcommandsSection) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options output = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  output.metavar = "FILE";
  LONGS_EQUAL(0, ap_add_argument(p, "--output", output, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "subcommands:\n") == NULL);
  CHECK(strstr(help, "optional arguments:\n") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatHelpPositionalOneNargsPrintsSingleMetavar) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options src = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  src.nargs = AP_NARGS_ONE;
  src.metavar = "SRC";
  src.help = "source path";
  LONGS_EQUAL(0, ap_add_argument(p, "src", src, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "  SRC\n    source path") != NULL);
  CHECK(strstr(help, "  SRC [SRC]") == NULL);
  CHECK(strstr(help, "  SRC SRC") == NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatHelpPositionalOneOrMoreShowsExpandedMetavarSuffix) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options files = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  files.nargs = AP_NARGS_ONE_OR_MORE;
  files.metavar = "FILE";
  files.help = "input files";
  LONGS_EQUAL(0, ap_add_argument(p, "files", files, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "  FILE [FILE ...]\n    input files") != NULL);

  free(help);
  ap_parser_free(p);
}

TEST(FormatHelpAndManpageCoverRequiredOptionalNargsVariants) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options many = ap_arg_options_default();
  ap_arg_options plus = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  char *help = NULL;
  char *manpage = NULL;

  CHECK(p != NULL);
  maybe.required = true;
  maybe.nargs = AP_NARGS_OPTIONAL;
  maybe.metavar = "MAYBE";
  maybe.help = "maybe value";
  many.required = true;
  many.nargs = AP_NARGS_ZERO_OR_MORE;
  many.metavar = "MANY";
  many.help = "many values";
  plus.required = true;
  plus.nargs = AP_NARGS_ONE_OR_MORE;
  plus.metavar = "PLUS";
  plus.help = "plus values";
  pair.required = true;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  pair.metavar = "PAIR";
  pair.help = "pair values";

  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--many", many, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--plus", plus, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--pair", pair, &err));

  help = ap_format_help(p);
  manpage = ap_format_manpage(p);
  CHECK(help != NULL);
  CHECK(manpage != NULL);

  CHECK(
      strstr(help, "  --maybe [MAYBE]\n    maybe value\n    required: true") !=
      NULL);
  CHECK(strstr(help,
               "  --many [MANY ...]\n    many values\n    required: true") !=
        NULL);
  CHECK(
      strstr(help,
             "  --plus PLUS [PLUS ...]\n    plus values\n    required: true") !=
      NULL);
  CHECK(
      strstr(help, "  --pair PAIR PAIR\n    pair values\n    required: true") !=
      NULL);

  CHECK(strstr(manpage, ".SH SYNOPSIS\n.B tool") != NULL);
  CHECK(strstr(manpage, "\\-\\-maybe [MAYBE]") != NULL);
  CHECK(strstr(manpage, "\\-\\-many [MANY ...]") != NULL);
  CHECK(strstr(manpage, "\\-\\-plus PLUS [PLUS ...]") != NULL);
  CHECK(strstr(manpage, "\\-\\-pair PAIR PAIR") != NULL);
  CHECK(strstr(manpage, ".TP\n.B \\-\\-maybe [MAYBE]") != NULL);
  CHECK(strstr(manpage, ".TP\n.B \\-\\-many [MANY ...]") != NULL);
  CHECK(strstr(manpage, ".TP\n.B \\-\\-plus PLUS [PLUS ...]") != NULL);
  CHECK(strstr(manpage, ".TP\n.B \\-\\-pair PAIR PAIR") != NULL);
  CHECK(strstr(manpage, "required: yes") != NULL);

  free(help);
  free(manpage);
  ap_parser_free(p);
}

TEST(FormatHelpAndManpageRenderExpandedMetadataMatrix) {
  ap_error err = {};
  ap_parser_options raw_opts = ap_parser_options_default();
  ap_parser *standard = ap_parser_new("tool", "desc");
  ap_parser *raw = NULL;
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options many = ap_arg_options_default();
  ap_arg_options fixed = ap_arg_options_default();
  ap_arg_options pos = ap_arg_options_default();
  const char *choices[] = {"red", "blue"};
  char *std_help = NULL;
  char *raw_help = NULL;
  char *man = NULL;

  CHECK(standard != NULL);
  raw_opts.help_formatter_mode = AP_HELP_FORMATTER_RAW_TEXT;
  raw = ap_parser_new_with_options("tool", "desc", raw_opts);
  CHECK(raw != NULL);

  maybe.nargs = AP_NARGS_OPTIONAL;
  maybe.help = "optional maybe";
  maybe.default_value = "auto";
  maybe.choices.items = choices;
  maybe.choices.count = 2;

  many.nargs = AP_NARGS_ZERO_OR_MORE;
  many.help = "accept many";

  fixed.nargs = AP_NARGS_FIXED;
  fixed.nargs_count = 3;
  fixed.required = true;
  fixed.help = "fixed triple";

  pos.nargs = AP_NARGS_FIXED;
  pos.nargs_count = 2;
  pos.help = "destination";

  LONGS_EQUAL(0, ap_add_argument(standard, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(standard, "--many", many, &err));
  LONGS_EQUAL(0, ap_add_argument(standard, "--fixed", fixed, &err));
  LONGS_EQUAL(0, ap_add_argument(standard, "target_path", pos, &err));

  LONGS_EQUAL(0, ap_add_argument(raw, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(raw, "--many", many, &err));
  LONGS_EQUAL(0, ap_add_argument(raw, "--fixed", fixed, &err));
  LONGS_EQUAL(0, ap_add_argument(raw, "target_path", pos, &err));

  std_help = ap_format_help(standard);
  raw_help = ap_format_help(raw);
  man = ap_format_manpage(standard);
  CHECK(std_help != NULL);
  CHECK(raw_help != NULL);
  CHECK(man != NULL);

  CHECK(strstr(std_help, "  --maybe [MAYBE]") != NULL);
  CHECK(strstr(std_help, "choices: red, blue") != NULL);
  CHECK(strstr(std_help, "default: auto") != NULL);
  CHECK(strstr(std_help, "  --many [MANY ...]") != NULL);
  CHECK(strstr(std_help, "  --fixed FIXED FIXED FIXED") != NULL);
  CHECK(strstr(std_help, "required: true") != NULL);
  CHECK(strstr(std_help, "  TARGET-PATH TARGET-PATH") != NULL);

  CHECK(strstr(raw_help, "--maybe [MAYBE] - optional maybe") != NULL);
  CHECK(strstr(raw_help, "| choices: red, blue") != NULL);
  CHECK(strstr(raw_help, "--many [MANY ...]") != NULL);
  CHECK(strstr(raw_help, "--fixed FIXED FIXED FIXED - fixed triple") != NULL);
  CHECK(strstr(raw_help, "TARGET-PATH TARGET-PATH - destination") != NULL);

  CHECK(strstr(man, ".B \\-\\-maybe [MAYBE]") != NULL);
  CHECK(strstr(man, "choices: red, blue") != NULL);
  CHECK(strstr(man, "default: auto") != NULL);
  CHECK(strstr(man, ".B \\-\\-fixed FIXED FIXED FIXED") != NULL);

  free(std_help);
  free(raw_help);
  free(man);
  ap_parser_free(raw);
  ap_parser_free(standard);
}

TEST(FormatManpageNestedSubcommandsBalanceRsReIndentation) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("tool", "desc");
  ap_parser *alpha = NULL;
  ap_parser *beta = NULL;
  char *manpage = NULL;
  const char *cursor = NULL;
  int rs_count = 0;
  int re_count = 0;

  CHECK(root != NULL);
  alpha = ap_add_subcommand(root, "alpha", "alpha help", &err);
  CHECK(alpha != NULL);
  beta = ap_add_subcommand(alpha, "beta", "beta help", &err);
  CHECK(beta != NULL);
  CHECK(ap_add_subcommand(beta, "gamma", "gamma help", &err) != NULL);

  manpage = ap_format_manpage(root);
  CHECK(manpage != NULL);

  cursor = manpage;
  while ((cursor = strstr(cursor, ".RS\n")) != NULL) {
    rs_count++;
    cursor += 4;
  }
  cursor = manpage;
  while ((cursor = strstr(cursor, ".RE\n")) != NULL) {
    re_count++;
    cursor += 4;
  }

  LONGS_EQUAL(rs_count, re_count);
  CHECK(rs_count > 0);
  CHECK(strstr(manpage, ".B alpha") != NULL);
  CHECK(strstr(manpage, ".B beta") != NULL);
  CHECK(strstr(manpage, ".B gamma") != NULL);

  free(manpage);
  ap_parser_free(root);
}

TEST(FormatManpageShowsBuiltinHelpOptionInsteadOfNoneFallback) {
  ap_parser *p = ap_parser_new("tool", "desc");
  char *manpage = NULL;

  CHECK(p != NULL);
  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, ".SH OPTIONS\nNone.\n") == NULL);
  CHECK(strstr(manpage, "\\-h, \\-\\-help") != NULL);

  free(manpage);
  ap_parser_free(p);
}

TEST(FormatManpageSynopsisUsesFallbackMetavarAndOptionsExcludePositionals) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options positional = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  char *manpage = NULL;

  CHECK(p != NULL);
  positional.help = "path input";
  LONGS_EQUAL(0, ap_add_argument(p, "file2_path", positional, &err));
  mode.metavar = "MODE";
  mode.help = "execution mode";
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));

  manpage = ap_format_manpage(p);
  CHECK(manpage != NULL);
  CHECK(strstr(manpage, ".SH SYNOPSIS\n.B tool") != NULL);
  CHECK(strstr(manpage, "FILE2\\-PATH") != NULL);
  CHECK(strstr(manpage, ".SH OPTIONS\n") != NULL);
  CHECK(strstr(manpage, "\\-\\-mode MODE") != NULL);
  CHECK(strstr(manpage, ".TP\n.B FILE2\\-PATH") == NULL);

  free(manpage);
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

TEST(ParserCompletionCanBeCustomizedAndScriptsUseConfiguredEntrypoint) {
  ap_parser_options options = ap_parser_options_default();
  ap_parser *p = NULL;
  char *bash = NULL;
  char *fish = NULL;

  options.completion_entrypoint = "__ap_complete";
  p = ap_parser_new_with_options("prog", "desc", options);
  CHECK(p != NULL);
  STRCMP_EQUAL("__ap_complete", ap_parser_completion_entrypoint(p));

  bash = ap_format_bash_completion(p);
  fish = ap_format_fish_completion(p);
  CHECK(bash != NULL);
  CHECK(fish != NULL);
  CHECK(strstr(bash, "'__ap_complete' --shell bash --") != NULL);
  CHECK(strstr(fish, "\"__ap_complete\" --shell fish --") != NULL);

  free(bash);
  free(fish);
  ap_parser_free(p);
}

TEST(FormatCompletionUsesStaticCompletionMetadata) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options dir = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options positional_file = ap_arg_options_default();
  ap_arg_options positional_mode = ap_arg_options_default();
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
  positional_file.completion_kind = AP_COMPLETION_KIND_FILE;
  positional_mode.choices.items = modes;
  positional_mode.choices.count = 2;

  LONGS_EQUAL(0, ap_add_argument(p, "--input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--dir", dir, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--exec", exec, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "source", positional_file, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "mode_name", positional_mode, &err));

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
  CHECK(
      strstr(bash, "mapfile -t COMPREPLY < <(__ap_completion_dynamic_query)") !=
      NULL);
  CHECK(strstr(fish, "-l input -d \"INPUT\" -r -F") != NULL);
  CHECK(
      strstr(fish, "-l dir -d \"DIR\" -r -a '(__fish_complete_directories)'") !=
      NULL);
  CHECK(strstr(fish, "-l exec -d \"EXEC\" -r -a '(__fish_complete_command)'") !=
        NULL);
  CHECK(strstr(fish, "-l mode -d \"MODE\" -r -a '(__ap_prog_value_choices "
                     "root:--mode)'") != NULL);
  CHECK(
      strstr(fish,
             "complete -c \"prog\" -f -a '(__ap_prog_positional_complete)'") !=
      NULL);

  free(bash);
  free(fish);
  ap_parser_free(p);
}

TEST(FormatCompletionEscapesSpecialCharactersAndDerivesFallbackDescriptions) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("my-tool", "desc");
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options dry_run = ap_arg_options_default();
  ap_arg_options flag = ap_arg_options_default();
  const char *modes[] = {"o'reilly", "plain"};
  char *bash = NULL;
  char *fish = NULL;
  char *zsh = NULL;

  CHECK(p != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  dry_run.help = NULL;
  dry_run.metavar = NULL;
  flag.type = AP_TYPE_BOOL;
  flag.action = AP_ACTION_STORE_TRUE;

  LONGS_EQUAL(0, ap_add_argument(p, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--dry-run", dry_run, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--flag", flag, &err));
  CHECK(ap_add_subcommand(p, "alpha", "", &err) != NULL);
  CHECK(ap_add_subcommand(p, "beta", "", &err) != NULL);

  bash = ap_format_bash_completion(p);
  fish = ap_format_fish_completion(p);
  zsh = ap_format_zsh_completion(p);
  CHECK(bash != NULL);
  CHECK(fish != NULL);
  CHECK(zsh != NULL);

  CHECK(strstr(bash, "root:--mode) printf '%s\\n' 'o'\\''reilly' 'plain';;") !=
        NULL);
  CHECK(strstr(zsh, "root:--mode) reply=( 'o'\\''reilly' 'plain' )") != NULL);
  CHECK(strstr(bash, "parser_subcommands='alpha beta'") != NULL);
  CHECK(strstr(zsh, "parser_subcommands=( 'alpha' 'beta' )") != NULL);
  CHECK(strstr(fish, "complete -c \"my-tool\" -n '") != NULL);
  CHECK(strstr(fish, "-l dry-run -d \"DRY-RUN\" -r") != NULL);

  free(bash);
  free(fish);
  free(zsh);
  ap_parser_free(p);
}

TEST(FormatCompletionFailureInjectionCoversEscapingBranches) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("my-tool", "line with \\ and $ and\nnewline");
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options weird = ap_arg_options_default();
  ap_arg_options path = ap_arg_options_default();
  const char *modes[] = {"o'reilly", "plain"};

  CHECK(root != NULL);
  mode.choices.items = modes;
  mode.choices.count = 2;
  weird.help = "contains \\ and $ and\nnewline";
  weird.completion_hint = "contains \\ and $ and\nnewline";
  path.completion_kind = AP_COMPLETION_KIND_DIRECTORY;

  LONGS_EQUAL(0, ap_add_argument(root, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--dry-run", weird, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--path", path, &err));
  CHECK(ap_add_subcommand(root, "alpha", "", &err) != NULL);
  CHECK(ap_add_subcommand(root, "beta", "", &err) != NULL);

  assert_bash_completion_append_failures_return_null(root);
  assert_fish_completion_append_failures_return_null(root);
  assert_zsh_completion_append_failures_return_null(root);

  ap_parser_free(root);
}

TEST(FormatZshCompletionIncludesSubcommandsAndCompletionKinds) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_parser *config = NULL;
  ap_arg_options output = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options dir = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  ap_arg_options mode = ap_arg_options_default();
  static const char *const commands[] = {"git", nullptr};
  const char *formats[] = {"json", "yaml"};
  char *script = NULL;

  CHECK(p != NULL);
  output.choices.items = formats;
  output.choices.count = 2;
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  dir.completion_kind = AP_COMPLETION_KIND_DIRECTORY;
  exec.completion_kind = AP_COMPLETION_KIND_COMMAND;
  exec.completion_callback = dynamic_exec_completion;
  exec.completion_user_data = (void *)commands;

  LONGS_EQUAL(0, ap_add_argument(p, "--output", output, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--dir", dir, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--exec", exec, &err));
  config = ap_add_subcommand(p, "config", "config commands", &err);
  CHECK(config != NULL);
  mode.choices.items = formats;
  mode.choices.count = 2;
  LONGS_EQUAL(0, ap_add_argument(config, "--mode", mode, &err));

  script = ap_format_zsh_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "#compdef prog") != NULL);
  CHECK(strstr(script, "parser_subcommands=( 'config' )") != NULL);
  CHECK(strstr(script, "parser_options=( '-h' '--help' '--output' '--input' "
                       "'--dir' '--exec' )") != NULL);
  CHECK(strstr(script, "root:--output) reply=( 'json' 'yaml' )") != NULL);
  CHECK(strstr(script, "root:--input) printf '%s\\n' 'file'") != NULL);
  CHECK(strstr(script, "root:--dir) printf '%s\\n' 'directory'") != NULL);
  CHECK(strstr(script, "root:--exec) printf '%s\\n' 'dynamic'") != NULL);
  CHECK(strstr(script, "root/config:--mode) reply=( 'json' 'yaml' )") != NULL);
  CHECK(strstr(script, "_files -/") != NULL);
  CHECK(strstr(script, "_command_names") != NULL);
  CHECK(strstr(script, "'__complete' --shell zsh --") != NULL);
  CHECK(strstr(script, "compdef _prog 'prog'") != NULL);

  free(script);
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
  CHECK(strstr(bash, "'__complete' --shell bash -- \"${COMP_WORDS[@]:1}\"") !=
        NULL);
  CHECK(strstr(bash, "root:--exec) printf '%s\\n' 'dynamic' ;;") != NULL);
  CHECK(strstr(fish,
               "\"prog\" \"__complete\" --shell fish -- $tokens $current") !=
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

TEST(FormatBashCompletionCoversSingleNoneAndDynamicFallbackBranches) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options plus = ap_arg_options_default();
  ap_arg_options fixed_single = ap_arg_options_default();
  ap_arg_options plain = ap_arg_options_default();
  ap_arg_options dynamic_choice = ap_arg_options_default();
  ap_arg_options feature = ap_arg_options_default();
  const char *modes[] = {"fast", "slow"};
  static const char *const commands[] = {"git", nullptr};
  char *script = NULL;

  CHECK(p != NULL);
  plus.nargs = AP_NARGS_ONE_OR_MORE;
  fixed_single.nargs = AP_NARGS_FIXED;
  fixed_single.nargs_count = 1;
  dynamic_choice.choices.items = modes;
  dynamic_choice.choices.count = 2;
  dynamic_choice.completion_callback = dynamic_exec_completion;
  dynamic_choice.completion_user_data = (void *)commands;
  feature.action = AP_ACTION_STORE_CONST;
  feature.const_value = "enabled";

  LONGS_EQUAL(0, ap_add_argument(p, "--plus", plus, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--fixed-single", fixed_single, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--plain", plain, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--dynamic-choice", dynamic_choice, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--feature", feature, &err));

  script = ap_format_bash_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "parser_value_options='--plus --fixed-single --plain "
                       "--dynamic-choice'") != NULL);
  CHECK(strstr(script, "parser_flag_only_options='-h --help --feature'") !=
        NULL);
  CHECK(strstr(script, "root:--plus) printf '%s\\n' 'multi' ;;") != NULL);
  CHECK(strstr(script, "root:--plus) printf '%d\\n' 1 ;;") != NULL);
  CHECK(strstr(script, "root:--fixed-single) printf '%s\\n' 'single' ;;") !=
        NULL);
  CHECK(strstr(script, "root:--fixed-single) printf '%d\\n' 1 ;;") != NULL);
  CHECK(strstr(script, "root:--plain) printf '%s\\n' 'none' ;;") != NULL);
  CHECK(strstr(script, "root:--dynamic-choice) printf '%s\\n' 'dynamic' ;;") !=
        NULL);
  CHECK(
      strstr(script, "root:--dynamic-choice) printf '%s\\n' 'fast' 'slow';;") !=
      NULL);

  free(script);
  ap_parser_free(p);
}

TEST(FormatZshCompletionMarksValueModesCountsAndNoneFallback) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options maybe = ap_arg_options_default();
  ap_arg_options extras = ap_arg_options_default();
  ap_arg_options pair = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  ap_arg_options plain = ap_arg_options_default();
  char *script = NULL;

  CHECK(p != NULL);
  maybe.nargs = AP_NARGS_OPTIONAL;
  extras.nargs = AP_NARGS_ZERO_OR_MORE;
  pair.nargs = AP_NARGS_FIXED;
  pair.nargs_count = 2;
  exec.completion_kind = AP_COMPLETION_KIND_COMMAND;

  LONGS_EQUAL(0, ap_add_argument(p, "--maybe", maybe, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--extras", extras, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--pair", pair, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--exec", exec, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--plain", plain, &err));

  script = ap_format_zsh_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "root:--maybe) printf '%s\\n' 'optional' ;;") != NULL);
  CHECK(strstr(script, "root:--extras) printf '%s\\n' 'multi' ;;") != NULL);
  CHECK(strstr(script, "root:--pair) printf '%s\\n' 'fixed' ;;") != NULL);
  CHECK(strstr(script, "root:--exec) printf '%s\\n' 'command' ;;") != NULL);
  CHECK(strstr(script, "root:--plain) printf '%s\\n' 'none' ;;") != NULL);
  CHECK(strstr(script, "root:--maybe) printf '%d\\n' 1 ;;") != NULL);
  CHECK(strstr(script, "root:--pair) printf '%d\\n' 2 ;;") != NULL);

  free(script);
  ap_parser_free(p);
}

TEST(FormatFishCompletionUsesFallbackMetavarForUnderscoredDestinations) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options config_path = ap_arg_options_default();
  ap_arg_options versioned = ap_arg_options_default();
  ap_arg_options named = ap_arg_options_default();
  char *script = NULL;

  CHECK(p != NULL);
  LONGS_EQUAL(0, ap_add_argument(p, "--config-path", config_path, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "--v2-mode", versioned, &err));
  named.metavar = "META.FILE";
  LONGS_EQUAL(0, ap_add_argument(p, "--named", named, &err));

  script = ap_format_fish_completion(p);
  CHECK(script != NULL);
  CHECK(strstr(script, "-l config-path -d \"CONFIG-PATH\" -r") != NULL);
  CHECK(strstr(script, "-l v2-mode -d \"V2-MODE\" -r") != NULL);
  CHECK(strstr(script, "-l named -d \"META.FILE\" -r") != NULL);

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

TEST(GeneratedZshCompletionScriptPassesZshSyntaxCheck) {
  const std::string temp_dir = make_temp_dir();
  const std::filesystem::path script_path =
      std::filesystem::path(temp_dir) / "_example_completion";
  const std::string generate_command = shell_quote(AP_EXAMPLE_COMPLETION_PATH) +
                                       " --generate-zsh-completion > " +
                                       shell_quote(script_path.string());

  CHECK(run_command(generate_command) == 0);
  CHECK(std::filesystem::exists(script_path));
  if (run_command("command -v zsh > /dev/null 2>&1") == 0) {
    CHECK(run_command("zsh -n " + shell_quote(script_path.string())) == 0);
  }

  std::filesystem::remove_all(temp_dir);
}

TEST(ExampleManpageExposesZshCompletionGenerator) {
  const std::string temp_dir = make_temp_dir();
  const std::filesystem::path script_path =
      std::filesystem::path(temp_dir) / "_example_manpage";
  const std::string generate_command = shell_quote(AP_EXAMPLE_MANPAGE_PATH) +
                                       " --generate-zsh-completion > " +
                                       shell_quote(script_path.string());
  std::string script;

  CHECK(run_command(generate_command) == 0);
  CHECK(std::filesystem::exists(script_path));

  {
    std::ifstream in(script_path);
    CHECK(in.good());
    script.assign((std::istreambuf_iterator<char>(in)),
                  std::istreambuf_iterator<char>());
  }

  CHECK(script.find("#compdef example_manpage") != std::string::npos);
  CHECK(script.find("--generate-zsh-completion") != std::string::npos);
  CHECK(script.find("'__complete' --shell zsh --") != std::string::npos);

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

TEST(FormatErrorSurvivesVeryLongInvalidNumericMessages) {
  ap_error err = {};
  ap_namespace *ns = NULL;
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options limit = ap_arg_options_default();
  std::string huge_bad_number(4096, '9');
  std::vector<char *> argv;
  char *formatted = NULL;

  CHECK(p != NULL);
  limit.type = AP_TYPE_INT64;
  LONGS_EQUAL(0, ap_add_argument(p, "--limit", limit, &err));

  argv.push_back((char *)"prog");
  argv.push_back((char *)"--limit");
  argv.push_back((char *)huge_bad_number.c_str());

  LONGS_EQUAL(-1, ap_parse_args(p, (int)argv.size(), argv.data(), &ns, &err));
  LONGS_EQUAL(AP_ERR_INVALID_INT64, err.code);
  STRCMP_EQUAL("--limit", err.argument);
  CHECK(ns == NULL);

  formatted = ap_format_error(p, &err);
  CHECK(formatted != NULL);
  CHECK(strstr(formatted, "error: argument '--limit' must be a valid int64") !=
        NULL);
  CHECK(strstr(formatted, "usage: prog") != NULL);

  free(formatted);
  ap_parser_free(p);
}

TEST(StringBuilderFailureHooksCoverAppendAndFreePaths) {
  ap_string_builder sb = {};
  char *allocated = NULL;

  reset_sb_hooks();
  ap_sb_init(&sb);
  fail_appendf_on_call(1);
  LONGS_EQUAL(-1, ap_sb_appendf(&sb, "boom"));
  LONGS_EQUAL(1, g_sb_hook_state.appendf_call_count);
  ap_sb_free(&sb);

  reset_sb_hooks();
  ap_sb_init(&sb);
  LONGS_EQUAL(0, ap_sb_appendf(&sb, "hello"));
  CHECK(sb.data != NULL);
  allocated = sb.data;
  fail_free_on_call(1);
  ap_sb_free(&sb);
  LONGS_EQUAL(1, g_sb_hook_state.free_call_count);
  LONGS_EQUAL(1, g_sb_hook_state.free_fail_hit_count);
  CHECK(sb.data == NULL);
  LONGS_EQUAL(0U, sb.len);
  LONGS_EQUAL(0U, sb.cap);
  free(allocated);

  reset_sb_hooks();
}

TEST(FormatterBuildersReturnNullWhenAppendFailsAndCleanUp) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");
  ap_arg_options output = ap_arg_options_default();
  char *text = NULL;

  CHECK(p != NULL);
  output.help = "write output";
  output.metavar = "FILE";
  LONGS_EQUAL(0, ap_add_argument(p, "--output", output, &err));

  reset_sb_hooks();
  fail_appendf_on_call(1);
  CHECK(ap_format_usage(p) == NULL);

  reset_sb_hooks();
  fail_appendf_on_call(3);
  CHECK(ap_format_help(p) == NULL);
  CHECK(g_sb_hook_state.free_call_count > 0);

  reset_sb_hooks();
  fail_appendf_on_call(2);
  CHECK(ap_format_manpage(p) == NULL);
  CHECK(g_sb_hook_state.free_call_count > 0);

  reset_sb_hooks();
  fail_appendf_on_call(1);
  CHECK(ap_format_error(p, NULL) == NULL);

  reset_sb_hooks();
  text = ap_format_help(p);
  CHECK(text != NULL);
  free(text);

  reset_sb_hooks();
  ap_parser_free(p);
}

TEST(FormatManpageFailureInjectionCoversRichStructuredBuilderPaths) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("tool", "top level parser");
  ap_parser *config = NULL;
  ap_parser *set = NULL;
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options output = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  const char *choices[] = {"fast", "slow"};

  CHECK(root != NULL);
  mode.help = "select execution mode";
  mode.metavar = "MODE";
  mode.default_value = "fast";
  mode.required = true;
  mode.choices.items = choices;
  mode.choices.count = 2;
  output.help = "write output";
  output.metavar = "FILE";
  input.help = "input path";

  LONGS_EQUAL(0, ap_add_argument(root, "-m, --mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--output", output, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "input_path", input, &err));

  config = ap_add_subcommand(root, "config", "manage configuration", &err);
  CHECK(config != NULL);
  set = ap_add_subcommand(config, "set", "set a value", &err);
  CHECK(set != NULL);
  CHECK(ap_add_subcommand(set, "leaf", "leaf command", &err) != NULL);

  assert_manpage_append_failures_return_null(root);

  ap_parser_free(root);
}

TEST(FormatManpageFailureInjectionCoversFallbackDescriptionBranch) {
  ap_parser *root = ap_parser_new("tool", "");

  CHECK(root != NULL);
  assert_manpage_append_failures_return_null(root);
  ap_parser_free(root);
}

TEST(FormatHelpAndUsageFailureInjectionCoversStructuredBuilderPaths) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("tool", "top level parser");
  ap_parser *config = NULL;
  ap_arg_options verbose = ap_arg_options_default();
  ap_arg_options output = ap_arg_options_default();
  ap_arg_options item = ap_arg_options_default();
  const char *choices[] = {"json", "yaml"};

  CHECK(root != NULL);
  verbose.type = AP_TYPE_INT32;
  verbose.action = AP_ACTION_COUNT;
  verbose.help = "increase verbosity";
  output.help = "write output";
  output.metavar = "FILE";
  output.default_value = "stdout";
  output.required = true;
  output.choices.items = choices;
  output.choices.count = 2;
  item.nargs = AP_NARGS_ONE_OR_MORE;
  item.metavar = "ITEM";
  item.help = "input item";

  LONGS_EQUAL(0, ap_add_argument(root, "-v, --verbose", verbose, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--output", output, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "item", item, &err));
  config = ap_add_subcommand(root, "config", "config commands", &err);
  CHECK(config != NULL);

  assert_usage_append_failures_return_null(root);
  assert_help_append_failures_return_null(root);

  ap_parser_free(root);
}

TEST(FormatHelpAndUsageFailureInjectionCoversFallbackBranches) {
  ap_parser *root = ap_parser_new("tool", "");

  CHECK(root != NULL);
  assert_usage_append_failures_return_null(root);
  assert_help_append_failures_return_null(root);
  ap_parser_free(root);
}

TEST(FormatBashCompletionFailureInjectionCoversStructuredBuilderPaths) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("tool", "top level parser");
  ap_parser *config = NULL;
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  const char *choices[] = {"fast", "slow"};
  static const char *const commands[] = {"git", nullptr};

  CHECK(root != NULL);
  mode.choices.items = choices;
  mode.choices.count = 2;
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  exec.completion_callback = dynamic_exec_completion;
  exec.completion_user_data = (void *)commands;

  LONGS_EQUAL(0, ap_add_argument(root, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--exec", exec, &err));
  config = ap_add_subcommand(root, "config", "config commands", &err);
  CHECK(config != NULL);
  LONGS_EQUAL(0, ap_add_argument(config, "--mode", mode, &err));

  assert_bash_completion_append_failures_return_null(root);

  ap_parser_free(root);
}

TEST(FormatFishCompletionFailureInjectionCoversStructuredBuilderPaths) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("tool", "top level parser");
  ap_parser *config = NULL;
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  const char *choices[] = {"fast", "slow"};
  static const char *const commands[] = {"git", nullptr};

  CHECK(root != NULL);
  mode.choices.items = choices;
  mode.choices.count = 2;
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  exec.completion_callback = dynamic_exec_completion;
  exec.completion_user_data = (void *)commands;

  LONGS_EQUAL(0, ap_add_argument(root, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--exec", exec, &err));
  config = ap_add_subcommand(root, "config", "config commands", &err);
  CHECK(config != NULL);
  LONGS_EQUAL(0, ap_add_argument(config, "--mode", mode, &err));

  assert_fish_completion_append_failures_return_null(root);

  ap_parser_free(root);
}

TEST(FormatZshCompletionFailureInjectionCoversStructuredBuilderPaths) {
  ap_error err = {};
  ap_parser *root = ap_parser_new("tool", "top level parser");
  ap_parser *config = NULL;
  ap_arg_options mode = ap_arg_options_default();
  ap_arg_options input = ap_arg_options_default();
  ap_arg_options exec = ap_arg_options_default();
  const char *choices[] = {"fast", "slow"};
  static const char *const commands[] = {"git", nullptr};

  CHECK(root != NULL);
  mode.choices.items = choices;
  mode.choices.count = 2;
  input.completion_kind = AP_COMPLETION_KIND_FILE;
  exec.completion_callback = dynamic_exec_completion;
  exec.completion_user_data = (void *)commands;

  LONGS_EQUAL(0, ap_add_argument(root, "--mode", mode, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--input", input, &err));
  LONGS_EQUAL(0, ap_add_argument(root, "--exec", exec, &err));
  config = ap_add_subcommand(root, "config", "config commands", &err);
  CHECK(config != NULL);
  LONGS_EQUAL(0, ap_add_argument(config, "--mode", mode, &err));

  assert_zsh_completion_append_failures_return_null(root);

  ap_parser_free(root);
}

TEST(FormatHelpAndUsageTolerateUnexpectedInternalNargsValues) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_arg_options opt = ap_arg_options_default();
  ap_arg_options pos = ap_arg_options_default();
  char *usage = NULL;
  char *help = NULL;

  CHECK(p != NULL);
  opt.help = "optional value";
  pos.help = "positional value";
  LONGS_EQUAL(0, ap_add_argument(p, "--mystery", opt, &err));
  LONGS_EQUAL(0, ap_add_argument(p, "target", pos, &err));

  p->defs[1].opts.nargs = (ap_nargs)99;
  p->defs[2].opts.nargs = (ap_nargs)99;

  usage = ap_format_usage(p);
  help = ap_format_help(p);
  CHECK(usage != NULL);
  CHECK(help != NULL);
  CHECK(strstr(usage, "usage: tool") != NULL);
  CHECK(strstr(usage, " TARGET") == NULL);
  CHECK(strstr(help, "optional arguments:\n") != NULL);
  CHECK(strstr(help, "positional arguments:\n") != NULL);
  CHECK(strstr(help, "  TARGET\n    positional value") != NULL);

  free(usage);
  free(help);
  ap_parser_free(p);
}

TEST(FormatHelpRendersCustomArgumentGroups) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("tool", "desc");
  ap_argument_group *output = NULL;
  ap_arg_options color = ap_arg_options_default();
  ap_arg_options target = ap_arg_options_default();
  char *help = NULL;

  CHECK(p != NULL);
  output = ap_add_argument_group(p, "output", "output controls", &err);
  CHECK(output != NULL);

  color.help = "color mode";
  target.help = "target file";
  LONGS_EQUAL(0,
              ap_argument_group_add_argument(output, "--color", color, &err));
  LONGS_EQUAL(0,
              ap_argument_group_add_argument(output, "target", target, &err));

  help = ap_format_help(p);
  CHECK(help != NULL);
  CHECK(strstr(help, "\noutput:\n") != NULL);
  CHECK(strstr(help, "  output controls\n") != NULL);
  CHECK(strstr(help, "  --color COLOR\n    color mode\n") != NULL);
  CHECK(strstr(help, "  TARGET\n    target file\n") != NULL);
  CHECK(strstr(help, "optional arguments:\n") != NULL);
  CHECK(strstr(help, "positional arguments:\n") == NULL);

  free(help);
  ap_parser_free(p);
}

TEST(StringBuilderHelpersRejectNullInputs) {
  ap_string_builder sb = {};

  ap_sb_free(NULL);
  ap_sb_init(&sb);
  LONGS_EQUAL(-1, ap_sb_appendf(NULL, "%s", "x"));
  LONGS_EQUAL(-1, ap_sb_appendf(&sb, NULL));
  ap_sb_free(&sb);
}
