#ifndef AP_INTERNAL_H
#define AP_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "argparse-c.h"

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} ap_string_builder;

typedef struct {
  char **items;
  int count;
  int cap;
} ap_strvec;

typedef struct {
  bool is_optional;
  char *dest;
  char **flags;
  int flags_count;
  ap_arg_options opts;
  int mutex_group_index;
  int arg_group_index;
} ap_arg_def;

typedef struct ap_subcommand_def {
  char *name;
  char *help;
  struct ap_parser *parser;
} ap_subcommand_def;

struct ap_mutually_exclusive_group {
  struct ap_parser *parser;
  int index;
};

struct ap_argument_group {
  struct ap_parser *parser;
  int index;
};

typedef struct {
  char *title;
  char *description;
  int *arg_indexes;
  int arg_count;
  int arg_cap;
  struct ap_argument_group handle;
} ap_arg_group_def;

typedef struct {
  bool required;
  int *arg_indexes;
  int arg_count;
  int arg_cap;
  struct ap_mutually_exclusive_group handle;
} ap_mutex_group_def;

struct ap_parser {
  char *prog;
  char *description;
  char *command_name;
  char *completion_entrypoint;
  char *prefix_chars;
  bool allow_abbrev;
  char *fromfile_prefix_chars;
  bool completion_enabled;
  struct ap_parser *parent;
  ap_arg_def *defs;
  int defs_count;
  int defs_cap;
  ap_mutex_group_def *mutex_groups;
  int mutex_groups_count;
  int mutex_groups_cap;
  ap_arg_group_def *arg_groups;
  int arg_groups_count;
  int arg_groups_cap;
  ap_subcommand_def *subcommands;
  int subcommands_count;
  int subcommands_cap;
};

typedef enum {
  AP_NS_VALUE_STRING = 0,
  AP_NS_VALUE_INT32,
  AP_NS_VALUE_INT64,
  AP_NS_VALUE_UINT64,
  AP_NS_VALUE_DOUBLE,
  AP_NS_VALUE_BOOL,
} ap_ns_value_type;

typedef struct {
  char *dest;
  ap_ns_value_type type;
  int count;
  union {
    char **strings;
    int32_t *ints;
    int64_t *int64s;
    uint64_t *uint64s;
    double *doubles;
    bool boolean;
  } as;
} ap_ns_entry;

struct ap_namespace {
  ap_ns_entry *entries;
  int count;
};

typedef struct {
  bool seen;
  ap_strvec values;
} ap_parsed_arg;

void ap_error_set(ap_error *err, ap_error_code code, const char *argument,
                  const char *fmt, ...);
const char *ap_error_argument_name(const ap_arg_def *def);
void ap_error_label_for_arg(const ap_arg_def *def, char *buf, size_t buf_size);

char *ap_strdup(const char *s);
char *ap_strndup(const char *s, size_t n);
void ap_trim_inplace(char *s);
bool ap_starts_with_dash(const char *s);
bool ap_token_has_prefix(const ap_parser *parser, const char *s);

int ap_strvec_push(ap_strvec *vec, char *item);
void ap_strvec_free(ap_strvec *vec);

void ap_sb_init(ap_string_builder *sb);
void ap_sb_free(ap_string_builder *sb);
int ap_sb_appendf(ap_string_builder *sb, const char *fmt, ...);

int ap_parser_parse(const ap_parser *parser, int argc, char **argv,
                    bool allow_unknown, ap_parsed_arg **out_parsed,
                    ap_strvec *positionals, ap_strvec *unknown_args,
                    ap_error *err);
const ap_arg_def *ap_next_positional_def(const ap_parser *parser,
                                         int consumed_positionals);
int ap_validate_args(const ap_parser *parser, const ap_parsed_arg *parsed,
                     ap_error *err);
int ap_build_namespace(const ap_parser *parser, const ap_parsed_arg *parsed,
                       ap_namespace **out_ns, ap_error *err);

char *ap_usage_build(const ap_parser *parser);
char *ap_help_build(const ap_parser *parser);
char *ap_manpage_build(const ap_parser *parser);
char *ap_bash_completion_build(const ap_parser *parser);
char *ap_fish_completion_build(const ap_parser *parser);
char *ap_zsh_completion_build(const ap_parser *parser);
bool ap_is_long_flag(const char *flag);
bool ap_is_short_flag(const char *flag);

#endif
