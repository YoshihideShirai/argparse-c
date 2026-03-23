#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ap_internal.h"

static int ensure_defs_capacity(ap_parser *parser) {
  if (parser->defs_count < parser->defs_cap) {
    return 0;
  }
  int next_cap = parser->defs_cap == 0 ? 8 : parser->defs_cap * 2;
  ap_arg_def *next =
      realloc(parser->defs, sizeof(ap_arg_def) * (size_t)next_cap);
  if (!next) {
    return -1;
  }
  parser->defs = next;
  parser->defs_cap = next_cap;
  return 0;
}

static int ensure_subcommands_capacity(ap_parser *parser) {
  if (parser->subcommands_count < parser->subcommands_cap) {
    return 0;
  }
  int next_cap = parser->subcommands_cap == 0 ? 4 : parser->subcommands_cap * 2;
  ap_subcommand_def *next = realloc(
      parser->subcommands, sizeof(ap_subcommand_def) * (size_t)next_cap);
  if (!next) {
    return -1;
  }
  parser->subcommands = next;
  parser->subcommands_cap = next_cap;
  return 0;
}

static int ensure_mutex_groups_capacity(ap_parser *parser) {
  if (parser->mutex_groups_count < parser->mutex_groups_cap) {
    return 0;
  }
  int next_cap =
      parser->mutex_groups_cap == 0 ? 4 : parser->mutex_groups_cap * 2;
  ap_mutex_group_def *next = realloc(
      parser->mutex_groups, sizeof(ap_mutex_group_def) * (size_t)next_cap);
  if (!next) {
    return -1;
  }
  parser->mutex_groups = next;
  parser->mutex_groups_cap = next_cap;
  return 0;
}

static int mutex_group_push_arg(ap_mutex_group_def *group, int arg_index) {
  int *next;
  int next_cap;
  if (group->arg_count < group->arg_cap) {
    group->arg_indexes[group->arg_count++] = arg_index;
    return 0;
  }
  next_cap = group->arg_cap == 0 ? 4 : group->arg_cap * 2;
  next = realloc(group->arg_indexes, sizeof(int) * (size_t)next_cap);
  if (!next) {
    return -1;
  }
  group->arg_indexes = next;
  group->arg_cap = next_cap;
  group->arg_indexes[group->arg_count++] = arg_index;
  return 0;
}

static void arg_def_free(ap_arg_def *def) {
  int i;
  if (!def) {
    return;
  }
  free(def->dest);
  for (i = 0; i < def->flags_count; i++) {
    free(def->flags[i]);
  }
  free(def->flags);
}

void ap_error_set(ap_error *err, ap_error_code code, const char *argument,
                  const char *fmt, ...) {
  va_list ap;
  if (!err) {
    return;
  }
  err->code = code;
  err->argument[0] = '\0';
  err->message[0] = '\0';
  if (argument) {
    snprintf(err->argument, sizeof(err->argument), "%s", argument);
  }
  if (!fmt) {
    return;
  }
  va_start(ap, fmt);
  vsnprintf(err->message, sizeof(err->message), fmt, ap);
  va_end(ap);
}

const char *ap_error_argument_name(const ap_arg_def *def) {
  if (!def) {
    return "";
  }
  if (def->flags_count > 0 && def->flags[0]) {
    return def->flags[0];
  }
  return def->dest ? def->dest : "";
}

void ap_error_label_for_arg(const ap_arg_def *def, char *buf, size_t buf_size) {
  const char *kind;
  const char *name;

  if (!buf || buf_size == 0) {
    return;
  }

  kind = (def && def->is_optional) ? "option" : "argument";
  name = ap_error_argument_name(def);
  snprintf(buf, buf_size, "%s '%s'", kind, name);
}

char *ap_strdup(const char *s) {
  size_t len;
  char *buf;
  if (!s) {
    return NULL;
  }
  len = strlen(s);
  buf = malloc(len + 1);
  if (!buf) {
    return NULL;
  }
  memcpy(buf, s, len + 1);
  return buf;
}

char *ap_strndup(const char *s, size_t n) {
  char *buf;
  if (!s) {
    return NULL;
  }
  buf = malloc(n + 1);
  if (!buf) {
    return NULL;
  }
  memcpy(buf, s, n);
  buf[n] = '\0';
  return buf;
}

void ap_trim_inplace(char *s) {
  char *start;
  char *end;
  size_t len;
  if (!s) {
    return;
  }
  start = s;
  while (*start && isspace((unsigned char)*start)) {
    start++;
  }
  if (start != s) {
    memmove(s, start, strlen(start) + 1);
  }
  len = strlen(s);
  if (len == 0) {
    return;
  }
  end = s + len - 1;
  while (end >= s && isspace((unsigned char)*end)) {
    *end = '\0';
    end--;
  }
}

bool ap_starts_with_dash(const char *s) {
  return s && s[0] == '-' && s[1] != '\0';
}

bool ap_is_long_flag(const char *flag) {
  return flag && strncmp(flag, "--", 2) == 0 && flag[2] != '\0';
}

bool ap_is_short_flag(const char *flag) {
  return flag && flag[0] == '-' && flag[1] != '-' && flag[1] != '\0' &&
         flag[2] == '\0';
}

int ap_strvec_push(ap_strvec *vec, char *item) {
  char **next_items;
  int next_cap;
  if (vec->count < vec->cap) {
    vec->items[vec->count++] = item;
    return 0;
  }
  next_cap = vec->cap == 0 ? 4 : vec->cap * 2;
  next_items = realloc(vec->items, sizeof(char *) * (size_t)next_cap);
  if (!next_items) {
    return -1;
  }
  vec->items = next_items;
  vec->cap = next_cap;
  vec->items[vec->count++] = item;
  return 0;
}

void ap_strvec_free(ap_strvec *vec) {
  int i;
  if (!vec) {
    return;
  }
  for (i = 0; i < vec->count; i++) {
    free(vec->items[i]);
  }
  free(vec->items);
  vec->items = NULL;
  vec->count = 0;
  vec->cap = 0;
}

static char *normalize_auto_dest(const char *raw) {
  size_t i;
  char *dest = ap_strdup(raw);
  if (!dest) {
    return NULL;
  }
  for (i = 0; dest[i] != '\0'; i++) {
    if (dest[i] == '-') {
      dest[i] = '_';
    }
  }
  return dest;
}

static char *pick_default_dest(bool is_optional, char **flags,
                               int flags_count) {
  const char *raw_dest = NULL;
  int i;

  if (!is_optional) {
    if (flags_count < 1) {
      return NULL;
    }
    raw_dest = flags[0];
  } else {
    for (i = 0; i < flags_count; i++) {
      if (strncmp(flags[i], "--", 2) == 0 && flags[i][2] != '\0') {
        raw_dest = flags[i] + 2;
        break;
      }
    }
    if (!raw_dest && flags_count > 0) {
      raw_dest = flags[0];
      while (*raw_dest == '-') {
        raw_dest++;
      }
    }
  }

  if (!raw_dest || raw_dest[0] == '\0') {
    return NULL;
  }
  return normalize_auto_dest(raw_dest);
}

static int split_flags(const char *name_or_flags, char ***out_flags,
                       int *out_count) {
  const char *p = name_or_flags;
  char **flags = NULL;
  int flags_count = 0;
  int flags_cap = 0;

  while (*p) {
    const char *start;
    const char *end;
    size_t len;
    char *token;
    char **next;

    while (*p == ' ' || *p == ',') {
      p++;
    }
    if (*p == '\0') {
      break;
    }

    start = p;
    while (*p && *p != ',') {
      p++;
    }
    end = p;
    while (end > start && isspace((unsigned char)*(end - 1))) {
      end--;
    }
    while (start < end && isspace((unsigned char)*start)) {
      start++;
    }
    len = (size_t)(end - start);
    if (len == 0) {
      continue;
    }

    token = ap_strndup(start, len);
    if (!token) {
      goto fail;
    }
    ap_trim_inplace(token);
    if (token[0] == '\0') {
      free(token);
      continue;
    }

    if (flags_count == flags_cap) {
      int next_cap = flags_cap == 0 ? 2 : flags_cap * 2;
      next = realloc(flags, sizeof(char *) * (size_t)next_cap);
      if (!next) {
        free(token);
        goto fail;
      }
      flags = next;
      flags_cap = next_cap;
    }
    flags[flags_count++] = token;
  }

  if (flags_count == 0) {
    return -1;
  }

  *out_flags = flags;
  *out_count = flags_count;
  return 0;

fail:
  while (flags_count > 0) {
    free(flags[--flags_count]);
  }
  free(flags);
  return -1;
}

ap_arg_options ap_arg_options_default(void) {
  ap_arg_options options;
  memset(&options, 0, sizeof(options));
  options.type = AP_TYPE_STRING;
  options.action = AP_ACTION_STORE;
  options.nargs = AP_NARGS_ONE;
  options.nargs_count = 1;
  return options;
}

void ap_completion_result_init(ap_completion_result *result) {
  if (!result) {
    return;
  }
  memset(result, 0, sizeof(*result));
}

void ap_completion_result_free(ap_completion_result *result) {
  int i;

  if (!result) {
    return;
  }
  for (i = 0; i < result->count; i++) {
    free((void *)result->items[i].value);
    free((void *)result->items[i].help);
  }
  free(result->items);
  ap_completion_result_init(result);
}

int ap_completion_result_add(ap_completion_result *result, const char *value,
                             const char *help, ap_error *err) {
  ap_completion_candidate *next;
  int next_cap;

  if (!result || !value || value[0] == '\0') {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "completion value is required");
    return -1;
  }
  if (result->count == result->cap) {
    next_cap = result->cap == 0 ? 4 : result->cap * 2;
    next = realloc(result->items,
                   sizeof(ap_completion_candidate) * (size_t)next_cap);
    if (!next) {
      ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
      return -1;
    }
    result->items = next;
    result->cap = next_cap;
  }
  result->items[result->count].value = ap_strdup(value);
  result->items[result->count].help = help ? ap_strdup(help) : NULL;
  if (!result->items[result->count].value ||
      (help && !result->items[result->count].help)) {
    free((void *)result->items[result->count].value);
    free((void *)result->items[result->count].help);
    result->items[result->count].value = NULL;
    result->items[result->count].help = NULL;
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }
  result->count++;
  return 0;
}

static int add_builtin_help(ap_parser *parser) {
  ap_arg_options opts;
  ap_error ignored_err;

  opts = ap_arg_options_default();
  opts.type = AP_TYPE_BOOL;
  opts.action = AP_ACTION_STORE_TRUE;
  opts.nargs = AP_NARGS_ONE;
  opts.help = "show this help message and exit";
  opts.dest = "help";
  return ap_add_argument(parser, "-h, --help", opts, &ignored_err);
}

static ap_parser *parser_alloc(const char *prog, const char *description,
                               const char *command_name, ap_parser *parent) {
  ap_parser *parser = calloc(1, sizeof(ap_parser));
  if (!parser) {
    return NULL;
  }

  parser->prog = ap_strdup(prog ? prog : "program");
  parser->description = ap_strdup(description ? description : "");
  parser->command_name = ap_strdup(command_name ? command_name : "");
  parser->parent = parent;
  if (!parser->prog || !parser->description || !parser->command_name) {
    ap_parser_free(parser);
    return NULL;
  }

  if (add_builtin_help(parser) != 0) {
    ap_parser_free(parser);
    return NULL;
  }
  return parser;
}

ap_parser *ap_parser_new(const char *prog, const char *description) {
  return parser_alloc(prog, description, "", NULL);
}

static int validate_options(const ap_arg_options *options, bool is_optional,
                            const char *name_or_flags, ap_error *err) {
  if (options->action == AP_ACTION_STORE_TRUE ||
      options->action == AP_ACTION_STORE_FALSE) {
    if (options->type != AP_TYPE_BOOL) {
      ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                   "store_true/store_false requires bool type");
      return -1;
    }
    return 0;
  }
  if (options->action == AP_ACTION_COUNT) {
    if (options->type != AP_TYPE_INT32) {
      ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                   "count action requires int32 type");
      return -1;
    }
    return 0;
  }
  if (options->action == AP_ACTION_STORE_CONST) {
    if (!options->const_value) {
      ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                   "store_const requires const_value");
      return -1;
    }
    return 0;
  }
  if (options->action != AP_ACTION_STORE &&
      options->action != AP_ACTION_APPEND && options->type == AP_TYPE_BOOL) {
    return 0;
  }
  if (options->nargs != AP_NARGS_ONE && options->nargs != AP_NARGS_OPTIONAL &&
      options->nargs != AP_NARGS_ZERO_OR_MORE &&
      options->nargs != AP_NARGS_ONE_OR_MORE &&
      options->nargs != AP_NARGS_FIXED) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "invalid nargs definition");
    return -1;
  }
  if (options->nargs == AP_NARGS_FIXED && options->nargs_count <= 0) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "fixed nargs requires nargs_count > 0");
    return -1;
  }
  if (!is_optional && options->action != AP_ACTION_STORE &&
      options->action != AP_ACTION_APPEND) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "positional arguments support only store/append actions");
    return -1;
  }
  return 0;
}

int ap_add_argument(ap_parser *parser, const char *name_or_flags,
                    ap_arg_options options, ap_error *err) {
  ap_arg_def def;
  int i;

  if (!parser || !name_or_flags || name_or_flags[0] == '\0') {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "parser and argument name are required");
    return -1;
  }

  memset(&def, 0, sizeof(def));
  def.is_optional = ap_starts_with_dash(name_or_flags);
  def.mutex_group_index = -1;

  if (split_flags(name_or_flags, &def.flags, &def.flags_count) != 0) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "invalid argument declaration");
    return -1;
  }

  if (def.is_optional) {
    for (i = 0; i < def.flags_count; i++) {
      if (!ap_starts_with_dash(def.flags[i])) {
        ap_error_set(err, AP_ERR_INVALID_DEFINITION, def.flags[i],
                     "optional argument flags must start with '-'");
        arg_def_free(&def);
        return -1;
      }
    }
  } else if (def.flags_count != 1 || ap_starts_with_dash(def.flags[0])) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "positional argument must be a single non-flag token");
    arg_def_free(&def);
    return -1;
  }

  if (validate_options(&options, def.is_optional, name_or_flags, err) != 0) {
    arg_def_free(&def);
    return -1;
  }

  if (options.action == AP_ACTION_STORE_TRUE ||
      options.action == AP_ACTION_STORE_FALSE ||
      options.action == AP_ACTION_COUNT ||
      options.action == AP_ACTION_STORE_CONST) {
    options.nargs = AP_NARGS_ONE;
    options.nargs_count = 1;
  }

  def.dest = options.dest ? ap_strdup(options.dest)
                          : pick_default_dest(def.is_optional, def.flags,
                                              def.flags_count);
  if (!def.dest) {
    ap_error_set(err, AP_ERR_NO_MEMORY, name_or_flags, "out of memory");
    arg_def_free(&def);
    return -1;
  }

  def.opts = options;

  for (i = 0; i < parser->defs_count; i++) {
    if (strcmp(parser->defs[i].dest, def.dest) == 0) {
      ap_error_set(err, AP_ERR_INVALID_DEFINITION, def.dest,
                   "dest '%s' already exists", def.dest);
      arg_def_free(&def);
      return -1;
    }
  }

  if (ensure_defs_capacity(parser) != 0) {
    ap_error_set(err, AP_ERR_NO_MEMORY, def.dest, "out of memory");
    arg_def_free(&def);
    return -1;
  }

  parser->defs[parser->defs_count++] = def;
  return 0;
}

ap_mutually_exclusive_group *ap_add_mutually_exclusive_group(ap_parser *parser,
                                                             bool required,
                                                             ap_error *err) {
  ap_mutex_group_def *def;

  if (!parser) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "", "parser is required");
    return NULL;
  }
  if (ensure_mutex_groups_capacity(parser) != 0) {
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return NULL;
  }
  def = &parser->mutex_groups[parser->mutex_groups_count];
  memset(def, 0, sizeof(*def));
  def->required = required;
  def->handle.parser = parser;
  def->handle.index = parser->mutex_groups_count;
  parser->mutex_groups_count++;
  return &def->handle;
}

int ap_group_add_argument(ap_mutually_exclusive_group *group,
                          const char *name_or_flags, ap_arg_options options,
                          ap_error *err) {
  ap_parser *parser;
  int arg_index;

  if (!group || !group->parser) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "", "group is required");
    return -1;
  }
  parser = group->parser;
  arg_index = parser->defs_count;
  if (ap_add_argument(parser, name_or_flags, options, err) != 0) {
    return -1;
  }
  parser->defs[arg_index].mutex_group_index = group->index;
  if (mutex_group_push_arg(&parser->mutex_groups[group->index], arg_index) !=
      0) {
    ap_error_set(err, AP_ERR_NO_MEMORY, name_or_flags, "out of memory");
    return -1;
  }
  return 0;
}

ap_parser *ap_add_subcommand(ap_parser *parser, const char *name,
                             const char *description, ap_error *err) {
  ap_subcommand_def def;
  char *prog = NULL;
  size_t prog_len;
  int i;

  if (!parser || !name || name[0] == '\0' || ap_starts_with_dash(name) ||
      strchr(name, ' ') != NULL) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name ? name : "",
                 "subcommand name must be a single non-flag token");
    return NULL;
  }
  for (i = 0; i < parser->subcommands_count; i++) {
    if (strcmp(parser->subcommands[i].name, name) == 0) {
      ap_error_set(err, AP_ERR_INVALID_DEFINITION, name,
                   "subcommand '%s' already exists", name);
      return NULL;
    }
  }

  prog_len = strlen(parser->prog) + strlen(name) + 2;
  prog = malloc(prog_len);
  if (!prog) {
    ap_error_set(err, AP_ERR_NO_MEMORY, name, "out of memory");
    return NULL;
  }
  snprintf(prog, prog_len, "%s %s", parser->prog, name);

  memset(&def, 0, sizeof(def));
  def.name = ap_strdup(name);
  def.help = ap_strdup(description ? description : "");
  def.parser = parser_alloc(prog, description ? description : "", name, parser);
  free(prog);
  if (!def.name || !def.help || !def.parser) {
    free(def.name);
    free(def.help);
    ap_parser_free(def.parser);
    ap_error_set(err, AP_ERR_NO_MEMORY, name, "out of memory");
    return NULL;
  }

  if (ensure_subcommands_capacity(parser) != 0) {
    free(def.name);
    free(def.help);
    ap_parser_free(def.parser);
    ap_error_set(err, AP_ERR_NO_MEMORY, name, "out of memory");
    return NULL;
  }

  parser->subcommands[parser->subcommands_count++] = def;
  return def.parser;
}

static const ap_ns_entry *find_entry(const ap_namespace *ns, const char *dest) {
  int i;
  if (!ns || !dest) {
    return NULL;
  }
  for (i = 0; i < ns->count; i++) {
    if (strcmp(ns->entries[i].dest, dest) == 0) {
      return &ns->entries[i];
    }
  }
  return NULL;
}

static void free_parsed_args(const ap_parser *parser, ap_parsed_arg *parsed) {
  int i;
  if (!parser || !parsed) {
    return;
  }
  for (i = 0; i < parser->defs_count; i++) {
    ap_strvec_free(&parsed[i].values);
  }
  free(parsed);
}

static int find_subcommand_arg_index(const ap_parser *parser, int argc,
                                     char **argv, int *out_index,
                                     int *out_subcommand_index) {
  int i;
  bool positional_only = false;

  *out_index = -1;
  *out_subcommand_index = -1;

  for (i = 1; i < argc; i++) {
    const char *token = argv[i];
    int j;

    if (!positional_only && strcmp(token, "--") == 0) {
      positional_only = true;
      continue;
    }

    if (!positional_only && ap_starts_with_dash(token)) {
      const char *eq = strchr(token, '=');
      for (j = 0; j < parser->defs_count; j++) {
        const ap_arg_def *def = &parser->defs[j];
        int k;
        if (!def->is_optional) {
          continue;
        }
        for (k = 0; k < def->flags_count; k++) {
          size_t flag_len = strlen(def->flags[k]);
          bool exact_match = strcmp(def->flags[k], token) == 0;
          bool inline_match = eq &&
                              strncmp(def->flags[k], token, flag_len) == 0 &&
                              token[flag_len] == '=';
          if (!exact_match && !inline_match) {
            continue;
          }
          if (!inline_match &&
              (def->opts.action == AP_ACTION_STORE ||
               def->opts.action == AP_ACTION_APPEND) &&
              def->opts.nargs == AP_NARGS_ONE && i + 1 < argc) {
            i++;
          } else if (!inline_match &&
                     (def->opts.action == AP_ACTION_STORE ||
                      def->opts.action == AP_ACTION_APPEND) &&
                     def->opts.nargs == AP_NARGS_OPTIONAL && i + 1 < argc &&
                     !ap_starts_with_dash(argv[i + 1])) {
            i++;
          } else if (!inline_match &&
                     (def->opts.action == AP_ACTION_STORE ||
                      def->opts.action == AP_ACTION_APPEND) &&
                     def->opts.nargs == AP_NARGS_FIXED) {
            i += def->opts.nargs_count;
          } else if (!inline_match &&
                     (def->opts.action == AP_ACTION_STORE ||
                      def->opts.action == AP_ACTION_APPEND) &&
                     (def->opts.nargs == AP_NARGS_ZERO_OR_MORE ||
                      def->opts.nargs == AP_NARGS_ONE_OR_MORE)) {
            while (i + 1 < argc && strcmp(argv[i + 1], "--") != 0 &&
                   !ap_starts_with_dash(argv[i + 1])) {
              i++;
            }
          }
          goto next_token;
        }
      }
    }

    for (j = 0; j < parser->subcommands_count; j++) {
      if (strcmp(parser->subcommands[j].name, token) == 0) {
        *out_index = i;
        *out_subcommand_index = j;
        return 0;
      }
    }
    if (parser->subcommands_count > 0) {
      return 0;
    }

  next_token:
    continue;
  }

  return 0;
}

static int append_namespace_entries(ap_namespace *dst, const ap_namespace *src,
                                    ap_error *err) {
  ap_ns_entry *next;
  int old_count;
  int added_count = 0;
  int i;

  if (!src || src->count == 0) {
    return 0;
  }

  old_count = dst->count;
  next = realloc(dst->entries,
                 sizeof(ap_ns_entry) * (size_t)(old_count + src->count));
  if (!next) {
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }
  dst->entries = next;
  memset(dst->entries + old_count, 0, sizeof(ap_ns_entry) * (size_t)src->count);

  for (i = 0; i < src->count; i++) {
    ap_ns_entry *entry;
    const ap_ns_entry *src_entry = &src->entries[i];
    int j;

    if (strcmp(src_entry->dest, "help") == 0) {
      continue;
    }

    entry = &dst->entries[old_count + added_count];

    for (j = 0; j < old_count + added_count; j++) {
      if (strcmp(dst->entries[j].dest, src_entry->dest) == 0) {
        if (strcmp(src_entry->dest, "subcommand_path") == 0) {
          entry = NULL;
          break;
        }
        ap_error_set(err, AP_ERR_INVALID_DEFINITION, src_entry->dest,
                     "dest '%s' already exists", src_entry->dest);
        return -1;
      }
    }
    if (!entry) {
      continue;
    }

    entry->dest = ap_strdup(src_entry->dest);
    if (!entry->dest) {
      ap_error_set(err, AP_ERR_NO_MEMORY, src_entry->dest, "out of memory");
      return -1;
    }
    entry->type = src_entry->type;
    entry->count = src_entry->count;
    if (src_entry->type == AP_NS_VALUE_STRING && src_entry->count > 0) {
      entry->as.strings = calloc((size_t)src_entry->count, sizeof(char *));
      if (!entry->as.strings) {
        ap_error_set(err, AP_ERR_NO_MEMORY, src_entry->dest, "out of memory");
        return -1;
      }
      for (j = 0; j < src_entry->count; j++) {
        entry->as.strings[j] = ap_strdup(src_entry->as.strings[j]);
        if (!entry->as.strings[j]) {
          ap_error_set(err, AP_ERR_NO_MEMORY, src_entry->dest, "out of memory");
          return -1;
        }
      }
    } else if (src_entry->type == AP_NS_VALUE_INT32 && src_entry->count > 0) {
      entry->as.ints = calloc((size_t)src_entry->count, sizeof(int32_t));
      if (!entry->as.ints) {
        ap_error_set(err, AP_ERR_NO_MEMORY, src_entry->dest, "out of memory");
        return -1;
      }
      memcpy(entry->as.ints, src_entry->as.ints,
             sizeof(int32_t) * (size_t)src_entry->count);
    } else if (src_entry->type == AP_NS_VALUE_BOOL) {
      entry->as.boolean = src_entry->as.boolean;
    }
    added_count++;
  }

  dst->count = old_count + added_count;
  return 0;
}

static int add_string_entry(ap_namespace *ns, const char *dest,
                            const char *value, ap_error *err) {
  ap_ns_entry *next;
  ap_ns_entry *entry;

  next = realloc(ns->entries, sizeof(ap_ns_entry) * (size_t)(ns->count + 1));
  if (!next) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dest ? dest : "", "out of memory");
    return -1;
  }
  ns->entries = next;
  entry = &ns->entries[ns->count];
  memset(entry, 0, sizeof(*entry));
  entry->dest = ap_strdup(dest);
  if (!entry->dest) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dest ? dest : "", "out of memory");
    return -1;
  }
  entry->type = AP_NS_VALUE_STRING;
  entry->count = 1;
  entry->as.strings = calloc(1, sizeof(char *));
  if (!entry->as.strings) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dest ? dest : "", "out of memory");
    return -1;
  }
  entry->as.strings[0] = ap_strdup(value);
  if (!entry->as.strings[0]) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dest ? dest : "", "out of memory");
    return -1;
  }
  ns->count++;
  return 0;
}

static int add_subcommand_entry(ap_namespace *ns, const char *name,
                                ap_error *err) {
  return add_string_entry(ns, "subcommand", name, err);
}

static int add_subcommand_path_entry(ap_namespace *ns, const char *name,
                                     const ap_namespace *sub_ns,
                                     ap_error *err) {
  const ap_ns_entry *sub_path_entry = find_entry(sub_ns, "subcommand_path");
  const char *sub_path = sub_path_entry &&
                                 sub_path_entry->type == AP_NS_VALUE_STRING &&
                                 sub_path_entry->count > 0
                             ? sub_path_entry->as.strings[0]
                             : NULL;

  if (sub_path && sub_path[0] != '\0') {
    size_t path_len = strlen(name) + 1 + strlen(sub_path) + 1;
    char *path = malloc(path_len);
    int rc;

    if (!path) {
      ap_error_set(err, AP_ERR_NO_MEMORY, "subcommand_path", "out of memory");
      return -1;
    }
    snprintf(path, path_len, "%s %s", name, sub_path);
    rc = add_string_entry(ns, "subcommand_path", path, err);
    free(path);
    return rc;
  }

  return add_string_entry(ns, "subcommand_path", name, err);
}

static int parse_internal(ap_parser *parser, int argc, char **argv,
                          bool allow_unknown, ap_namespace **out_ns,
                          char ***out_unknown_args, int *out_unknown_count,
                          ap_error *err) {
  ap_parsed_arg *parsed = NULL;
  ap_namespace *ns = NULL;
  ap_namespace *sub_ns = NULL;
  ap_strvec positionals;
  ap_strvec unknown_args;
  char **sub_unknown_args = NULL;
  int sub_unknown_count = 0;
  int subcommand_arg_index = -1;
  int subcommand_index = -1;
  int rc;

  if (!parser || !out_ns) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "parser and out_ns are required");
    return -1;
  }

  memset(&positionals, 0, sizeof(positionals));
  memset(&unknown_args, 0, sizeof(unknown_args));
  *out_ns = NULL;
  if (out_unknown_args) {
    *out_unknown_args = NULL;
  }
  if (out_unknown_count) {
    *out_unknown_count = 0;
  }

  rc = find_subcommand_arg_index(parser, argc, argv, &subcommand_arg_index,
                                 &subcommand_index);
  if (rc != 0) {
    goto done;
  }

  rc = ap_parser_parse(parser,
                       subcommand_arg_index >= 0 ? subcommand_arg_index : argc,
                       argv, allow_unknown, &parsed, &positionals,
                       allow_unknown ? &unknown_args : NULL, err);
  if (rc != 0) {
    goto done;
  }

  rc = ap_validate_args(parser, parsed, err);
  if (rc != 0) {
    goto done;
  }

  rc = ap_build_namespace(parser, parsed, &ns, err);
  if (rc != 0) {
    goto done;
  }

  if (subcommand_arg_index >= 0) {
    ap_parser *subparser = parser->subcommands[subcommand_index].parser;
    rc = parse_internal(subparser, argc - subcommand_arg_index,
                        argv + subcommand_arg_index, allow_unknown, &sub_ns,
                        allow_unknown ? &sub_unknown_args : NULL,
                        allow_unknown ? &sub_unknown_count : NULL, err);
    if (rc != 0) {
      goto done;
    }
    if (!find_entry(sub_ns, "subcommand")) {
      rc = add_subcommand_entry(ns, parser->subcommands[subcommand_index].name,
                                err);
      if (rc != 0) {
        goto done;
      }
    }
    rc = add_subcommand_path_entry(
        ns, parser->subcommands[subcommand_index].name, sub_ns, err);
    if (rc != 0) {
      goto done;
    }
    rc = append_namespace_entries(ns, sub_ns, err);
    if (rc != 0) {
      goto done;
    }
  } else if (parser->subcommands_count > 0 && !allow_unknown) {
    bool help_requested = false;
    int i;
    for (i = 0; i < parser->defs_count; i++) {
      if (strcmp(parser->defs[i].dest, "help") == 0 && parsed[i].seen) {
        help_requested = true;
        break;
      }
    }
    if (!help_requested) {
      ap_error_set(err, AP_ERR_MISSING_REQUIRED, "subcommand",
                   "a subcommand is required");
      rc = -1;
      goto done;
    }
  }

  *out_ns = ns;
  ns = NULL;
  if (allow_unknown && out_unknown_args && out_unknown_count) {
    int i;
    for (i = 0; i < sub_unknown_count; i++) {
      char *copy = ap_strdup(sub_unknown_args[i]);
      if (!copy || ap_strvec_push(&unknown_args, copy) != 0) {
        free(copy);
        ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
        rc = -1;
        goto done;
      }
    }
    *out_unknown_args = unknown_args.items;
    *out_unknown_count = unknown_args.count;
    unknown_args.items = NULL;
    unknown_args.count = 0;
    unknown_args.cap = 0;
  }

done:
  free_parsed_args(parser, parsed);
  ap_strvec_free(&positionals);
  ap_strvec_free(&unknown_args);
  ap_free_tokens(sub_unknown_args, sub_unknown_count);
  ap_namespace_free(sub_ns);
  ap_namespace_free(ns);
  return rc;
}

int ap_parse_args(ap_parser *parser, int argc, char **argv,
                  ap_namespace **out_ns, ap_error *err) {
  return parse_internal(parser, argc, argv, false, out_ns, NULL, NULL, err);
}

int ap_parse_known_args(ap_parser *parser, int argc, char **argv,
                        ap_namespace **out_ns, char ***out_unknown_args,
                        int *out_unknown_count, ap_error *err) {
  if (!parser || !out_ns || !out_unknown_args || !out_unknown_count) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "parser, outputs and unknown outputs are required");
    return -1;
  }
  return parse_internal(parser, argc, argv, true, out_ns, out_unknown_args,
                        out_unknown_count, err);
}

char *ap_format_usage(const ap_parser *parser) {
  return ap_usage_build(parser);
}

char *ap_format_help(const ap_parser *parser) { return ap_help_build(parser); }

char *ap_format_manpage(const ap_parser *parser) {
  return ap_manpage_build(parser);
}

char *ap_format_bash_completion(const ap_parser *parser) {
  return ap_bash_completion_build(parser);
}

char *ap_format_fish_completion(const ap_parser *parser) {
  return ap_fish_completion_build(parser);
}

static bool completion_action_takes_no_value(ap_action action) {
  return action == AP_ACTION_STORE_TRUE || action == AP_ACTION_STORE_FALSE ||
         action == AP_ACTION_COUNT || action == AP_ACTION_STORE_CONST;
}

static const ap_arg_def *find_def_by_flag(const ap_parser *parser,
                                          const char *flag) {
  int i;
  int j;

  if (!parser || !flag) {
    return NULL;
  }
  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!def->is_optional) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (strcmp(def->flags[j], flag) == 0) {
        return def;
      }
    }
  }
  return NULL;
}

static const ap_subcommand_def *find_subcommand_def(const ap_parser *parser,
                                                    const char *name) {
  int i;

  if (!parser || !name) {
    return NULL;
  }
  for (i = 0; i < parser->subcommands_count; i++) {
    if (strcmp(parser->subcommands[i].name, name) == 0) {
      return &parser->subcommands[i];
    }
  }
  return NULL;
}

static int append_completion_path(char *buf, size_t buf_size,
                                  const char *current_path,
                                  const char *segment) {
  if (!buf || buf_size == 0) {
    return -1;
  }
  if (!segment || segment[0] == '\0') {
    return snprintf(buf, buf_size, "%s", current_path ? current_path : "");
  }
  if (!current_path || current_path[0] == '\0') {
    return snprintf(buf, buf_size, "%s", segment);
  }
  return snprintf(buf, buf_size, "%s %s", current_path, segment);
}

static int add_static_completion_items(const ap_arg_def *def,
                                       ap_completion_result *result,
                                       ap_error *err) {
  int i;
  ap_completion_kind kind;

  if (!def || !result) {
    return 0;
  }
  kind = def->opts.completion_kind;
  if (kind == AP_COMPLETION_KIND_NONE && def->opts.choices.items &&
      def->opts.choices.count > 0) {
    kind = AP_COMPLETION_KIND_CHOICES;
  }
  if (kind != AP_COMPLETION_KIND_CHOICES) {
    return 0;
  }
  for (i = 0; i < def->opts.choices.count; i++) {
    if (ap_completion_result_add(result, def->opts.choices.items[i], NULL,
                                 err) != 0) {
      return -1;
    }
  }
  return 0;
}

int ap_complete(const ap_parser *parser, int argc, char **argv,
                const char *shell, ap_completion_result *out_result,
                ap_error *err) {
  const ap_parser *active_parser = parser;
  const ap_arg_def *active_def = NULL;
  const char *active_option = NULL;
  const char *current_token = "";
  int scan_count;
  int i;
  char subcommand_path[256];
  ap_completion_request request;

  if (!parser || argc < 0 || !out_result || (argc > 0 && !argv)) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "parser, argv, and completion result are required");
    return -1;
  }

  ap_completion_result_init(out_result);
  subcommand_path[0] = '\0';
  scan_count = argc > 0 ? argc - 1 : 0;
  if (argc > 0 && argv && argv[argc - 1]) {
    current_token = argv[argc - 1];
  }

  for (i = 0; i < scan_count; i++) {
    const char *token = argv[i];
    const ap_subcommand_def *sub;
    const ap_arg_def *def;

    if (!token) {
      continue;
    }
    if (strcmp(token, "--") == 0) {
      break;
    }
    if (active_def &&
        !completion_action_takes_no_value(active_def->opts.action)) {
      active_def = NULL;
      active_option = NULL;
      continue;
    }
    if (token[0] == '-') {
      const char *eq = strchr(token, '=');
      char option_name[128];

      if (eq) {
        size_t name_len = (size_t)(eq - token);
        if (name_len >= sizeof(option_name)) {
          name_len = sizeof(option_name) - 1;
        }
        memcpy(option_name, token, name_len);
        option_name[name_len] = '\0';
        def = find_def_by_flag(active_parser, option_name);
      } else {
        def = find_def_by_flag(active_parser, token);
      }
      if (def && !completion_action_takes_no_value(def->opts.action) && !eq) {
        active_def = def;
        active_option = token;
      }
      continue;
    }

    sub = find_subcommand_def(active_parser, token);
    if (sub) {
      char next_path[256];
      if (append_completion_path(next_path, sizeof(next_path), subcommand_path,
                                 sub->name) < 0) {
        ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
        ap_completion_result_free(out_result);
        return -1;
      }
      memcpy(subcommand_path, next_path, strlen(next_path) + 1);
      active_parser = sub->parser;
    }
  }

  if (scan_count > 0 && argv && argv[scan_count - 1]) {
    const char *token = argv[scan_count - 1];
    const char *eq = strchr(token, '=');

    if (eq && token[0] == '-') {
      char option_name[128];
      size_t name_len = (size_t)(eq - token);
      if (name_len >= sizeof(option_name)) {
        name_len = sizeof(option_name) - 1;
      }
      memcpy(option_name, token, name_len);
      option_name[name_len] = '\0';
      active_def = find_def_by_flag(active_parser, option_name);
      active_option = active_def ? option_name : NULL;
      current_token = eq + 1;
    }
  }

  if (!active_def) {
    return 0;
  }

  memset(&request, 0, sizeof(request));
  request.parser = active_parser;
  request.shell = shell;
  request.current_token = current_token ? current_token : "";
  request.active_option = active_option;
  request.subcommand_path = subcommand_path;
  request.dest = active_def->dest;
  request.argc = argc;
  request.argv = argv;

  if (active_def->opts.completion_callback) {
    if (active_def->opts.completion_callback(
            &request, out_result, active_def->opts.completion_user_data, err) !=
        0) {
      ap_completion_result_free(out_result);
      return -1;
    }
  }

  if (out_result->count == 0 &&
      add_static_completion_items(active_def, out_result, err) != 0) {
    ap_completion_result_free(out_result);
    return -1;
  }
  return 0;
}

int ap_parser_get_info(const ap_parser *parser, ap_parser_info *out_info) {
  if (!parser || !out_info) {
    return -1;
  }

  out_info->prog = parser->prog;
  out_info->description = parser->description;
  out_info->argument_count = parser->defs_count;
  out_info->subcommand_count = parser->subcommands_count;
  return 0;
}

int ap_parser_get_argument(const ap_parser *parser, int index,
                           ap_arg_info *out_info) {
  const ap_arg_def *def;

  if (!parser || !out_info || index < 0 || index >= parser->defs_count) {
    return -1;
  }

  def = &parser->defs[index];
  out_info->kind =
      def->is_optional ? AP_ARG_KIND_OPTIONAL : AP_ARG_KIND_POSITIONAL;
  out_info->flag_count = def->flags_count;
  out_info->flags = (const char *const *)def->flags;
  out_info->dest = def->dest;
  out_info->help = def->opts.help;
  out_info->metavar = def->opts.metavar;
  out_info->choices = def->opts.choices;
  out_info->completion_kind = def->opts.completion_kind;
  out_info->completion_hint = def->opts.completion_hint;
  out_info->has_completion_callback = def->opts.completion_callback != NULL;
  out_info->required = def->opts.required;
  out_info->nargs = def->opts.nargs;
  out_info->nargs_count = def->opts.nargs_count;
  out_info->type = def->opts.type;
  out_info->action = def->opts.action;
  return 0;
}

int ap_parser_get_subcommand(const ap_parser *parser, int index,
                             ap_subcommand_info *out_info) {
  const ap_subcommand_def *def;

  if (!parser || !out_info || index < 0 || index >= parser->subcommands_count) {
    return -1;
  }

  def = &parser->subcommands[index];
  out_info->name = def->name;
  out_info->description = def->help;
  out_info->parser = def->parser;
  return 0;
}

static int ap_arg_flag_count_by_kind(const ap_arg_info *info, bool long_flag) {
  int i;
  int count = 0;

  if (!info || !info->flags || info->flag_count < 1) {
    return 0;
  }

  for (i = 0; i < info->flag_count; i++) {
    if (long_flag ? ap_is_long_flag(info->flags[i])
                  : ap_is_short_flag(info->flags[i])) {
      count++;
    }
  }
  return count;
}

static const char *ap_arg_flag_at_by_kind(const ap_arg_info *info, int index,
                                          bool long_flag) {
  int i;
  int current = 0;

  if (!info || !info->flags || index < 0) {
    return NULL;
  }

  for (i = 0; i < info->flag_count; i++) {
    bool matches = long_flag ? ap_is_long_flag(info->flags[i])
                             : ap_is_short_flag(info->flags[i]);
    if (!matches) {
      continue;
    }
    if (current == index) {
      return info->flags[i];
    }
    current++;
  }
  return NULL;
}

int ap_arg_short_flag_count(const ap_arg_info *info) {
  return ap_arg_flag_count_by_kind(info, false);
}

const char *ap_arg_short_flag_at(const ap_arg_info *info, int index) {
  return ap_arg_flag_at_by_kind(info, index, false);
}

int ap_arg_long_flag_count(const ap_arg_info *info) {
  return ap_arg_flag_count_by_kind(info, true);
}

const char *ap_arg_long_flag_at(const ap_arg_info *info, int index) {
  return ap_arg_flag_at_by_kind(info, index, true);
}

char *ap_format_error(const ap_parser *parser, const ap_error *err) {
  char *usage = NULL;
  char *out = NULL;
  int needed;
  const char *msg = "unknown error";

  if (!parser) {
    return NULL;
  }
  if (err && err->message[0] != '\0') {
    msg = err->message;
  }

  usage = ap_usage_build(parser);
  if (!usage) {
    return NULL;
  }

  needed = snprintf(NULL, 0, "error: %s\n%s", msg, usage);
  if (needed < 0) {
    free(usage);
    return NULL;
  }
  out = malloc((size_t)needed + 1);
  if (!out) {
    free(usage);
    return NULL;
  }
  snprintf(out, (size_t)needed + 1, "error: %s\n%s", msg, usage);
  free(usage);
  return out;
}

void ap_parser_free(ap_parser *parser) {
  int i;
  if (!parser) {
    return;
  }
  for (i = 0; i < parser->defs_count; i++) {
    arg_def_free(&parser->defs[i]);
  }
  free(parser->defs);
  for (i = 0; i < parser->mutex_groups_count; i++) {
    free(parser->mutex_groups[i].arg_indexes);
  }
  free(parser->mutex_groups);
  for (i = 0; i < parser->subcommands_count; i++) {
    free(parser->subcommands[i].name);
    free(parser->subcommands[i].help);
    ap_parser_free(parser->subcommands[i].parser);
  }
  free(parser->subcommands);
  free(parser->prog);
  free(parser->description);
  free(parser->command_name);
  free(parser);
}

void ap_namespace_free(ap_namespace *ns) {
  int i;
  int j;
  if (!ns) {
    return;
  }
  for (i = 0; i < ns->count; i++) {
    free(ns->entries[i].dest);
    if (ns->entries[i].type == AP_NS_VALUE_STRING) {
      for (j = 0; j < ns->entries[i].count; j++) {
        free(ns->entries[i].as.strings[j]);
      }
      free(ns->entries[i].as.strings);
    } else if (ns->entries[i].type == AP_NS_VALUE_INT32) {
      free(ns->entries[i].as.ints);
    }
  }
  free(ns->entries);
  free(ns);
}

void ap_free_tokens(char **tokens, int count) {
  int i;
  if (!tokens) {
    return;
  }
  for (i = 0; i < count; i++) {
    free(tokens[i]);
  }
  free(tokens);
}

bool ap_ns_get_bool(const ap_namespace *ns, const char *dest, bool *out_value) {
  const ap_ns_entry *entry = find_entry(ns, dest);
  if (!entry || entry->type != AP_NS_VALUE_BOOL || !out_value) {
    return false;
  }
  *out_value = entry->as.boolean;
  return true;
}

bool ap_ns_get_string(const ap_namespace *ns, const char *dest,
                      const char **out_value) {
  const ap_ns_entry *entry = find_entry(ns, dest);
  if (!entry || entry->type != AP_NS_VALUE_STRING || entry->count < 1 ||
      !out_value) {
    return false;
  }
  *out_value = entry->as.strings[0];
  return true;
}

bool ap_ns_get_int32(const ap_namespace *ns, const char *dest,
                     int32_t *out_value) {
  const ap_ns_entry *entry = find_entry(ns, dest);
  if (!entry || entry->type != AP_NS_VALUE_INT32 || entry->count < 1 ||
      !out_value) {
    return false;
  }
  *out_value = entry->as.ints[0];
  return true;
}

int ap_ns_get_count(const ap_namespace *ns, const char *dest) {
  const ap_ns_entry *entry = find_entry(ns, dest);
  if (!entry) {
    return -1;
  }
  if (entry->type == AP_NS_VALUE_BOOL) {
    return 1;
  }
  return entry->count;
}

const char *ap_ns_get_string_at(const ap_namespace *ns, const char *dest,
                                int index) {
  const ap_ns_entry *entry = find_entry(ns, dest);
  if (!entry || entry->type != AP_NS_VALUE_STRING || index < 0 ||
      index >= entry->count) {
    return NULL;
  }
  return entry->as.strings[index];
}

bool ap_ns_get_int32_at(const ap_namespace *ns, const char *dest, int index,
                        int32_t *out_value) {
  const ap_ns_entry *entry = find_entry(ns, dest);
  if (!entry || entry->type != AP_NS_VALUE_INT32 || index < 0 ||
      index >= entry->count || !out_value) {
    return false;
  }
  *out_value = entry->as.ints[index];
  return true;
}
