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
  AP_TYPE_BOOL,
} ap_type;

typedef enum {
  AP_ACTION_STORE = 0,
  AP_ACTION_STORE_TRUE,
  AP_ACTION_STORE_FALSE,
} ap_action;

typedef enum {
  AP_NARGS_ONE = 0,
  AP_NARGS_OPTIONAL,
  AP_NARGS_ZERO_OR_MORE,
  AP_NARGS_ONE_OR_MORE,
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

typedef struct {
  ap_type type;
  ap_action action;
  ap_nargs nargs;
  bool required;
  const char *help;
  const char *metavar;
  const char *default_value;
  ap_choices choices;
  const char *dest;
} ap_arg_options;

typedef struct ap_parser ap_parser;
typedef struct ap_namespace ap_namespace;

ap_parser *ap_parser_new(const char *prog, const char *description);
ap_parser *ap_add_subcommand(ap_parser *parser, const char *name,
                             const char *description, ap_error *err);
void ap_parser_free(ap_parser *parser);

int ap_add_argument(ap_parser *parser, const char *name_or_flags,
                    ap_arg_options options, ap_error *err);

int ap_parse_args(ap_parser *parser, int argc, char **argv, ap_namespace **out_ns,
                  ap_error *err);
int ap_parse_known_args(ap_parser *parser, int argc, char **argv,
                        ap_namespace **out_ns, char ***out_unknown_args,
                        int *out_unknown_count, ap_error *err);
void ap_namespace_free(ap_namespace *ns);
void ap_free_tokens(char **tokens, int count);

char *ap_format_usage(const ap_parser *parser);
char *ap_format_help(const ap_parser *parser);
char *ap_format_error(const ap_parser *parser, const ap_error *err);

bool ap_ns_get_bool(const ap_namespace *ns, const char *dest, bool *out_value);
bool ap_ns_get_string(const ap_namespace *ns, const char *dest,
                      const char **out_value);
bool ap_ns_get_int32(const ap_namespace *ns, const char *dest,
                     int32_t *out_value);
int ap_ns_get_count(const ap_namespace *ns, const char *dest);
const char *ap_ns_get_string_at(const ap_namespace *ns, const char *dest,
                                int index);
bool ap_ns_get_int32_at(const ap_namespace *ns, const char *dest, int index,
                        int32_t *out_value);

ap_arg_options ap_arg_options_default(void);

#ifdef __cplusplus
}
#endif

#endif
