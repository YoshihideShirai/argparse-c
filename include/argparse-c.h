#ifndef ARGPARSE_C_H
#define ARGPARSE_C_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AP_TYPE_STRING = 0,
  AP_TYPE_INT32,
  AP_TYPE_INT64,
  AP_TYPE_DOUBLE,
  AP_TYPE_BOOL,
} ap_type;

typedef enum {
  AP_ACTION_STORE = 0,
  AP_ACTION_STORE_TRUE,
  AP_ACTION_STORE_FALSE,
  AP_ACTION_APPEND,
  AP_ACTION_COUNT,
  AP_ACTION_STORE_CONST,
} ap_action;

typedef enum {
  AP_NARGS_ONE = 0,
  AP_NARGS_OPTIONAL,
  AP_NARGS_ZERO_OR_MORE,
  AP_NARGS_ONE_OR_MORE,
  AP_NARGS_FIXED,
} ap_nargs;

typedef enum {
  AP_ERR_NONE = 0,
  AP_ERR_NO_MEMORY,
  AP_ERR_INVALID_DEFINITION,
  AP_ERR_UNKNOWN_OPTION,
  AP_ERR_DUPLICATE_OPTION,
  AP_ERR_MISSING_VALUE,
  AP_ERR_INVALID_NARGS,
  AP_ERR_MISSING_REQUIRED,
  AP_ERR_INVALID_CHOICE,
  AP_ERR_INVALID_INT32,
  AP_ERR_INVALID_INT64,
  AP_ERR_INVALID_DOUBLE,
  AP_ERR_UNEXPECTED_POSITIONAL,
} ap_error_code;

typedef struct {
  ap_error_code code;
  char argument[64];
  char message[256];
} ap_error;

typedef struct {
  int count;
  const char **items;
} ap_choices;

typedef enum {
  AP_COMPLETION_KIND_NONE = 0,
  AP_COMPLETION_KIND_CHOICES,
  AP_COMPLETION_KIND_FILE,
  AP_COMPLETION_KIND_DIRECTORY,
  AP_COMPLETION_KIND_COMMAND,
} ap_completion_kind;

typedef struct ap_completion_request ap_completion_request;
typedef struct ap_completion_result ap_completion_result;
typedef struct ap_parser ap_parser;
typedef struct ap_mutually_exclusive_group ap_mutually_exclusive_group;
typedef struct ap_namespace ap_namespace;

typedef struct {
  const char *value;
  const char *help;
} ap_completion_candidate;

struct ap_completion_result {
  ap_completion_candidate *items;
  int count;
  int cap;
};

typedef int (*ap_completion_callback)(const ap_completion_request *request,
                                      ap_completion_result *result,
                                      void *user_data, ap_error *err);

struct ap_completion_request {
  const ap_parser *parser;
  const char *shell;
  const char *current_token;
  const char *active_option;
  const char *subcommand_path;
  const char *dest;
  int argc;
  char **argv;
};

typedef struct {
  ap_type type;
  ap_action action;
  ap_nargs nargs;
  int nargs_count;
  bool required;
  const char *help;
  const char *metavar;
  const char *default_value;
  const char *const_value;
  ap_choices choices;
  ap_completion_kind completion_kind;
  const char *completion_hint;
  ap_completion_callback completion_callback;
  void *completion_user_data;
  const char *dest;
} ap_arg_options;

typedef enum {
  AP_ARG_KIND_POSITIONAL = 0,
  AP_ARG_KIND_OPTIONAL,
} ap_arg_kind;

typedef struct {
  const char *prog;
  const char *description;
  int argument_count;
  int subcommand_count;
} ap_parser_info;

typedef struct {
  ap_arg_kind kind;
  int flag_count;
  const char *const *flags;
  const char *dest;
  const char *help;
  const char *metavar;
  ap_choices choices;
  ap_completion_kind completion_kind;
  const char *completion_hint;
  bool has_completion_callback;
  bool required;
  ap_nargs nargs;
  int nargs_count;
  ap_type type;
  ap_action action;
} ap_arg_info;

typedef struct {
  const char *name;
  const char *description;
  const ap_parser *parser;
} ap_subcommand_info;

ap_parser *ap_parser_new(const char *prog, const char *description);
ap_parser *ap_add_subcommand(ap_parser *parser, const char *name,
                             const char *description, ap_error *err);
ap_mutually_exclusive_group *ap_add_mutually_exclusive_group(ap_parser *parser,
                                                             bool required,
                                                             ap_error *err);
void ap_parser_free(ap_parser *parser);

int ap_add_argument(ap_parser *parser, const char *name_or_flags,
                    ap_arg_options options, ap_error *err);
int ap_group_add_argument(ap_mutually_exclusive_group *group,
                          const char *name_or_flags, ap_arg_options options,
                          ap_error *err);

int ap_parse_args(ap_parser *parser, int argc, char **argv,
                  ap_namespace **out_ns, ap_error *err);
int ap_parse_known_args(ap_parser *parser, int argc, char **argv,
                        ap_namespace **out_ns, char ***out_unknown_args,
                        int *out_unknown_count, ap_error *err);
void ap_namespace_free(ap_namespace *ns);
void ap_free_tokens(char **tokens, int count);

char *ap_format_usage(const ap_parser *parser);
char *ap_format_help(const ap_parser *parser);
char *ap_format_manpage(const ap_parser *parser);
char *ap_format_bash_completion(const ap_parser *parser);
char *ap_format_fish_completion(const ap_parser *parser);
char *ap_format_error(const ap_parser *parser, const ap_error *err);
int ap_complete(const ap_parser *parser, int argc, char **argv,
                const char *shell, ap_completion_result *out_result,
                ap_error *err);
void ap_completion_result_init(ap_completion_result *result);
void ap_completion_result_free(ap_completion_result *result);
int ap_completion_result_add(ap_completion_result *result, const char *value,
                             const char *help, ap_error *err);

int ap_parser_get_info(const ap_parser *parser, ap_parser_info *out_info);
int ap_parser_get_argument(const ap_parser *parser, int index,
                           ap_arg_info *out_info);
int ap_parser_get_subcommand(const ap_parser *parser, int index,
                             ap_subcommand_info *out_info);
int ap_arg_short_flag_count(const ap_arg_info *info);
const char *ap_arg_short_flag_at(const ap_arg_info *info, int index);
int ap_arg_long_flag_count(const ap_arg_info *info);
const char *ap_arg_long_flag_at(const ap_arg_info *info, int index);

bool ap_ns_get_bool(const ap_namespace *ns, const char *dest, bool *out_value);
bool ap_ns_get_string(const ap_namespace *ns, const char *dest,
                      const char **out_value);
bool ap_ns_get_int32(const ap_namespace *ns, const char *dest,
                     int32_t *out_value);
bool ap_ns_get_int64(const ap_namespace *ns, const char *dest,
                     int64_t *out_value);
bool ap_ns_get_double(const ap_namespace *ns, const char *dest,
                      double *out_value);
int ap_ns_get_count(const ap_namespace *ns, const char *dest);
const char *ap_ns_get_string_at(const ap_namespace *ns, const char *dest,
                                int index);
bool ap_ns_get_int32_at(const ap_namespace *ns, const char *dest, int index,
                        int32_t *out_value);
bool ap_ns_get_int64_at(const ap_namespace *ns, const char *dest, int index,
                        int64_t *out_value);
bool ap_ns_get_double_at(const ap_namespace *ns, const char *dest, int index,
                         double *out_value);

ap_arg_options ap_arg_options_default(void);

#ifdef __cplusplus
}
#endif

#endif
