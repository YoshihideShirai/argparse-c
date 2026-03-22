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
  ap_arg_def *next = realloc(parser->defs, sizeof(ap_arg_def) * (size_t)next_cap);
  if (!next) {
    return -1;
  }
  parser->defs = next;
  parser->defs_cap = next_cap;
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

static const char *pick_default_dest(const char *name_or_flags, bool is_optional,
                                     char **flags, int flags_count) {
  int i;
  if (!is_optional) {
    return name_or_flags;
  }
  for (i = 0; i < flags_count; i++) {
    if (strncmp(flags[i], "--", 2) == 0) {
      return flags[i] + 2;
    }
  }
  if (flags_count > 0) {
    const char *f = flags[0];
    while (*f == '-') {
      f++;
    }
    return f;
  }
  return name_or_flags;
}

static char *normalize_dest(const char *raw) {
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
  return options;
}

ap_parser *ap_parser_new(const char *prog, const char *description) {
  ap_parser *parser;
  ap_arg_options opts;
  ap_error ignored_err;

  parser = calloc(1, sizeof(ap_parser));
  if (!parser) {
    return NULL;
  }

  parser->prog = ap_strdup(prog ? prog : "program");
  parser->description = ap_strdup(description ? description : "");
  if (!parser->prog || !parser->description) {
    ap_parser_free(parser);
    return NULL;
  }

  opts = ap_arg_options_default();
  opts.type = AP_TYPE_BOOL;
  opts.action = AP_ACTION_STORE_TRUE;
  opts.nargs = AP_NARGS_ONE;
  opts.help = "show this help message and exit";
  opts.dest = "help";
  if (ap_add_argument(parser, "-h, --help", opts, &ignored_err) != 0) {
    ap_parser_free(parser);
    return NULL;
  }

  return parser;
}

static int validate_options(const ap_arg_options *options, bool is_optional,
                            const char *name_or_flags, ap_error *err) {
  if (options->action != AP_ACTION_STORE && options->type == AP_TYPE_BOOL) {
    return 0;
  }
  if (options->action == AP_ACTION_STORE_TRUE ||
      options->action == AP_ACTION_STORE_FALSE) {
    if (options->type != AP_TYPE_BOOL) {
      ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                   "store_true/store_false requires bool type");
      return -1;
    }
    return 0;
  }
  if (options->nargs != AP_NARGS_ONE && options->nargs != AP_NARGS_OPTIONAL &&
      options->nargs != AP_NARGS_ZERO_OR_MORE &&
      options->nargs != AP_NARGS_ONE_OR_MORE) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "invalid nargs definition");
    return -1;
  }
  if (!is_optional && options->action != AP_ACTION_STORE) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, name_or_flags,
                 "positional arguments support only store action");
    return -1;
  }
  return 0;
}

int ap_add_argument(ap_parser *parser, const char *name_or_flags,
                    ap_arg_options options, ap_error *err) {
  ap_arg_def def;
  const char *raw_dest;
  int i;

  if (!parser || !name_or_flags || name_or_flags[0] == '\0') {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "parser and argument name are required");
    return -1;
  }

  memset(&def, 0, sizeof(def));
  def.is_optional = ap_starts_with_dash(name_or_flags);

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
      options.action == AP_ACTION_STORE_FALSE) {
    options.nargs = AP_NARGS_ONE;
  }

  raw_dest = options.dest
                 ? options.dest
                 : pick_default_dest(def.flags[0], def.is_optional, def.flags,
                                     def.flags_count);
  def.dest = normalize_dest(raw_dest);
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

int ap_parse_args(ap_parser *parser, int argc, char **argv, ap_namespace **out_ns,
                  ap_error *err) {
  ap_parsed_arg *parsed = NULL;
  ap_namespace *ns = NULL;
  ap_strvec positionals;
  int rc;

  if (!parser || !out_ns) {
    ap_error_set(err, AP_ERR_INVALID_DEFINITION, "",
                 "parser and out_ns are required");
    return -1;
  }

  memset(&positionals, 0, sizeof(positionals));
  *out_ns = NULL;

  rc = ap_parser_parse(parser, argc, argv, &parsed, &positionals, err);
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

  *out_ns = ns;
  ns = NULL;

done:
  if (parsed) {
    int i;
    for (i = 0; i < parser->defs_count; i++) {
      ap_strvec_free(&parsed[i].values);
    }
    free(parsed);
  }
  ap_strvec_free(&positionals);
  ap_namespace_free(ns);
  return rc;
}

char *ap_format_usage(const ap_parser *parser) { return ap_usage_build(parser); }

char *ap_format_help(const ap_parser *parser) { return ap_help_build(parser); }

void ap_parser_free(ap_parser *parser) {
  int i;
  if (!parser) {
    return;
  }
  for (i = 0; i < parser->defs_count; i++) {
    arg_def_free(&parser->defs[i]);
  }
  free(parser->defs);
  free(parser->prog);
  free(parser->description);
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
