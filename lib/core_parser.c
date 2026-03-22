#include <stdlib.h>
#include <string.h>

#include "ap_internal.h"

static bool action_takes_no_value(ap_action action) {
  return action == AP_ACTION_STORE_TRUE || action == AP_ACTION_STORE_FALSE ||
         action == AP_ACTION_COUNT || action == AP_ACTION_STORE_CONST;
}

static bool action_allows_multiple_occurrences(ap_action action) {
  return action == AP_ACTION_APPEND || action == AP_ACTION_COUNT ||
         action == AP_ACTION_STORE_CONST;
}

static int push_value(ap_parsed_arg *parsed, int def_index, const char *token,
                      ap_error *err);

static int find_flag_index(const ap_parser *parser, const char *token) {
  int i;
  int j;
  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!def->is_optional) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (strcmp(def->flags[j], token) == 0) {
        return i;
      }
    }
  }
  return -1;
}

static int find_flag_index_n(const ap_parser *parser, const char *token,
                             size_t token_len) {
  int i;
  int j;
  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!def->is_optional) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      const char *flag = def->flags[j];
      if (strlen(flag) == token_len && strncmp(flag, token, token_len) == 0) {
        return i;
      }
    }
  }
  return -1;
}

static int parse_short_cluster(const ap_parser *parser, const char *token,
                               bool allow_unknown, ap_parsed_arg *parsed,
                               ap_error *err) {
  size_t i;
  char short_flag[3];

  if (!token || token[0] != '-' || token[1] == '-' || token[2] == '\0') {
    return 1;
  }
  if (strchr(token, '=') != NULL) {
    return 1;
  }

  short_flag[0] = '-';
  short_flag[2] = '\0';
  for (i = 1; token[i] != '\0'; i++) {
    int def_index;
    short_flag[1] = token[i];
    def_index = find_flag_index(parser, short_flag);
    if (def_index < 0) {
      if (!allow_unknown) {
        ap_error_set(err, AP_ERR_UNKNOWN_OPTION, short_flag,
                     "unknown option '%s'", short_flag);
      }
      return allow_unknown ? 1 : -1;
    }
    if (parser->defs[def_index].opts.action != AP_ACTION_STORE_TRUE &&
        parser->defs[def_index].opts.action != AP_ACTION_STORE_FALSE &&
        parser->defs[def_index].opts.action != AP_ACTION_COUNT) {
      if (allow_unknown) {
        return 1;
      }
      ap_error_set(err, AP_ERR_INVALID_NARGS, short_flag,
                   "option '%s' cannot be used in a short option cluster",
                   short_flag);
      return -1;
    }
    if (parsed[def_index].seen &&
        !action_allows_multiple_occurrences(parser->defs[def_index].opts.action)) {
      ap_error_set(err, AP_ERR_DUPLICATE_OPTION, short_flag,
                   "duplicate option '%s'", short_flag);
      return -1;
    }
    parsed[def_index].seen = true;
    if (parser->defs[def_index].opts.action == AP_ACTION_COUNT &&
        push_value(parsed, def_index, "1", err) != 0) {
      return -1;
    }
  }

  return 0;
}

static int positional_min_required(const ap_arg_def *def) {
  if (action_takes_no_value(def->opts.action)) {
    return 0;
  }
  switch (def->opts.nargs) {
  case AP_NARGS_ONE:
    return 1;
  case AP_NARGS_ONE_OR_MORE:
    return 1;
  case AP_NARGS_FIXED:
    return def->opts.nargs_count;
  case AP_NARGS_OPTIONAL:
  case AP_NARGS_ZERO_OR_MORE:
  default:
    return 0;
  }
}

static int remaining_min_required(const ap_parser *parser,
                                  const int *positional_defs,
                                  int positional_count, int from_index) {
  int min_required = 0;
  int i;
  for (i = from_index; i < positional_count; i++) {
    min_required += positional_min_required(&parser->defs[positional_defs[i]]);
  }
  return min_required;
}

static int push_value(ap_parsed_arg *parsed, int def_index, const char *token,
                      ap_error *err) {
  char *copy = ap_strdup(token);
  if (!copy) {
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }
  if (ap_strvec_push(&parsed[def_index].values, copy) != 0) {
    free(copy);
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }
  return 0;
}

static int consume_optional_values(const ap_parser *parser, int argc, char **argv,
                                   int *idx, int def_index,
                                   const char *inline_value,
                                   ap_parsed_arg *parsed, ap_error *err) {
  const ap_arg_def *def = &parser->defs[def_index];
  int start = *idx;

  if (action_takes_no_value(def->opts.action)) {
    if (inline_value) {
      ap_error_set(err, AP_ERR_INVALID_NARGS, def->flags[0],
                   "option '%s' does not take a value", def->flags[0]);
      return -1;
    }
    if (def->opts.action == AP_ACTION_COUNT &&
        push_value(parsed, def_index, "1", err) != 0) {
      return -1;
    }
    if (def->opts.action == AP_ACTION_STORE_CONST &&
        push_value(parsed, def_index, def->opts.const_value, err) != 0) {
      return -1;
    }
    return 0;
  }

  if (def->opts.nargs == AP_NARGS_ONE) {
    if (inline_value) {
      return push_value(parsed, def_index, inline_value, err);
    }
    if (start + 1 >= argc) {
      ap_error_set(err, AP_ERR_MISSING_VALUE, def->flags[0],
                   "option '%s' requires a value", def->flags[0]);
      return -1;
    }
    if (find_flag_index(parser, argv[start + 1]) >= 0) {
      ap_error_set(err, AP_ERR_MISSING_VALUE, def->flags[0],
                   "option '%s' requires a value", def->flags[0]);
      return -1;
    }
    if (push_value(parsed, def_index, argv[start + 1], err) != 0) {
      return -1;
    }
    *idx = start + 1;
    return 0;
  }

  if (def->opts.nargs == AP_NARGS_OPTIONAL) {
    if (inline_value) {
      return push_value(parsed, def_index, inline_value, err);
    }
    if (start + 1 < argc) {
      int next_flag_idx = find_flag_index(parser, argv[start + 1]);
      if (next_flag_idx < 0) {
        if (push_value(parsed, def_index, argv[start + 1], err) != 0) {
          return -1;
        }
        *idx = start + 1;
      }
    }
    return 0;
  }

  if (def->opts.nargs == AP_NARGS_FIXED) {
    int need = def->opts.nargs_count;
    int consumed = 0;
    int j = start + 1;
    if (inline_value) {
      if (push_value(parsed, def_index, inline_value, err) != 0) {
        return -1;
      }
      consumed++;
    }
    while (consumed < need) {
      if (j >= argc || strcmp(argv[j], "--") == 0 ||
          find_flag_index(parser, argv[j]) >= 0) {
        ap_error_set(err, AP_ERR_INVALID_NARGS, def->flags[0],
                     "option '%s' requires exactly %d values", def->flags[0],
                     def->opts.nargs_count);
        return -1;
      }
      if (push_value(parsed, def_index, argv[j], err) != 0) {
        return -1;
      }
      consumed++;
      j++;
    }
    *idx = j - 1;
    return 0;
  }

  {
    int j = start + 1;
    int consumed = 0;
    if (inline_value) {
      if (push_value(parsed, def_index, inline_value, err) != 0) {
        return -1;
      }
      consumed = 1;
    }
    while (j < argc) {
      if (strcmp(argv[j], "--") == 0) {
        break;
      }
      if (find_flag_index(parser, argv[j]) >= 0) {
        break;
      }
      if (push_value(parsed, def_index, argv[j], err) != 0) {
        return -1;
      }
      consumed++;
      j++;
    }
    if (def->opts.nargs == AP_NARGS_ONE_OR_MORE && consumed == 0) {
      ap_error_set(err, AP_ERR_INVALID_NARGS, def->flags[0],
                   "option '%s' requires at least one value", def->flags[0]);
      return -1;
    }
    *idx = j - 1;
    return 0;
  }
}

int ap_parser_parse(const ap_parser *parser, int argc, char **argv,
                    bool allow_unknown, ap_parsed_arg **out_parsed,
                    ap_strvec *positionals, ap_strvec *unknown_args,
                    ap_error *err) {
  ap_parsed_arg *parsed;
  int positional_defs_count = 0;
  int *positional_defs = NULL;
  int i;
  int positional_cursor = 0;
  int token_index = 1;
  bool positional_only = false;

  parsed = calloc((size_t)parser->defs_count, sizeof(ap_parsed_arg));
  if (!parsed) {
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }

  for (i = 0; i < parser->defs_count; i++) {
    if (!parser->defs[i].is_optional) {
      positional_defs_count++;
    }
  }

  if (positional_defs_count > 0) {
    positional_defs = malloc(sizeof(int) * (size_t)positional_defs_count);
    if (!positional_defs) {
      free(parsed);
      ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
      return -1;
    }
    positional_defs_count = 0;
    for (i = 0; i < parser->defs_count; i++) {
      if (!parser->defs[i].is_optional) {
        positional_defs[positional_defs_count++] = i;
      }
    }
  }

  while (token_index < argc) {
    const char *token = argv[token_index];

    if (!positional_only && strcmp(token, "--") == 0) {
      if (allow_unknown && unknown_args) {
        int j;
        for (j = token_index + 1; j < argc; j++) {
          char *copy = ap_strdup(argv[j]);
          if (!copy || ap_strvec_push(unknown_args, copy) != 0) {
            free(copy);
            free(positional_defs);
            free(parsed);
            ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
            return -1;
          }
        }
        token_index = argc;
        break;
      }
      positional_only = true;
      token_index++;
      continue;
    }

    if (!positional_only && ap_starts_with_dash(token)) {
      int cluster_rc =
          parse_short_cluster(parser, token, allow_unknown, parsed, err);
      if (cluster_rc == 0) {
        token_index++;
        continue;
      }
      if (cluster_rc < 0) {
        int k;
        for (k = 0; k < parser->defs_count; k++) {
          ap_strvec_free(&parsed[k].values);
        }
        free(positional_defs);
        free(parsed);
        return -1;
      }

      const char *eq = strchr(token, '=');
      const char *inline_value = NULL;
      int def_index = -1;
      if (eq && eq != token) {
        def_index =
            find_flag_index_n(parser, token, (size_t)(eq - token));
        if (def_index >= 0) {
          inline_value = eq + 1;
        }
      } else {
        def_index = find_flag_index(parser, token);
      }
      if (def_index < 0) {
        if (allow_unknown) {
          char *copy = ap_strdup(token);
          if (!copy || !unknown_args ||
              ap_strvec_push(unknown_args, copy) != 0) {
            free(copy);
            free(positional_defs);
            free(parsed);
            ap_error_set(err, AP_ERR_NO_MEMORY, token, "out of memory");
            return -1;
          }
          if (token_index + 1 < argc && strcmp(argv[token_index + 1], "--") != 0 &&
              find_flag_index(parser, argv[token_index + 1]) < 0 &&
              !ap_starts_with_dash(argv[token_index + 1])) {
            char *next_copy = ap_strdup(argv[token_index + 1]);
            if (!next_copy || ap_strvec_push(unknown_args, next_copy) != 0) {
              free(next_copy);
              free(positional_defs);
              free(parsed);
              ap_error_set(err, AP_ERR_NO_MEMORY, token, "out of memory");
              return -1;
            }
            token_index += 2;
            continue;
          }
          token_index++;
          continue;
        }
        free(positional_defs);
        free(parsed);
        ap_error_set(err, AP_ERR_UNKNOWN_OPTION, token,
                     "unknown option '%s'", token);
        return -1;
      }
      if (parsed[def_index].seen &&
          !action_allows_multiple_occurrences(parser->defs[def_index].opts.action)) {
        free(positional_defs);
        free(parsed);
        ap_error_set(err, AP_ERR_DUPLICATE_OPTION, token,
                     "duplicate option '%s'", token);
        return -1;
      }
      if (consume_optional_values(parser, argc, argv, &token_index, def_index,
                                  inline_value, parsed, err) != 0) {
        int k;
        for (k = 0; k < parser->defs_count; k++) {
          ap_strvec_free(&parsed[k].values);
        }
        free(positional_defs);
        free(parsed);
        return -1;
      }
      parsed[def_index].seen = true;
      token_index++;
      continue;
    }

    if (ap_strvec_push(positionals, ap_strdup(token)) != 0) {
      int k;
      for (k = 0; k < parser->defs_count; k++) {
        ap_strvec_free(&parsed[k].values);
      }
      free(positional_defs);
      free(parsed);
      ap_error_set(err, AP_ERR_NO_MEMORY, token, "out of memory");
      return -1;
    }
    token_index++;
  }

  for (i = 0; i < positional_defs_count; i++) {
    const ap_arg_def *def = &parser->defs[positional_defs[i]];
    int values_left = positionals->count - positional_cursor;
    int min_after = remaining_min_required(parser, positional_defs,
                                           positional_defs_count, i + 1);
    int take = 0;

    switch (def->opts.nargs) {
    case AP_NARGS_ONE:
      take = values_left > 0 ? 1 : 0;
      break;
    case AP_NARGS_OPTIONAL:
      take = values_left > min_after ? 1 : 0;
      break;
    case AP_NARGS_ZERO_OR_MORE:
      take = values_left - min_after;
      if (take < 0) {
        take = 0;
      }
      break;
    case AP_NARGS_ONE_OR_MORE:
      take = values_left - min_after;
      if (take < 1) {
        char label[96];
        ap_error_label_for_arg(def, label, sizeof(label));
        ap_error_set(err, AP_ERR_INVALID_NARGS, ap_error_argument_name(def),
                     "%s requires at least one value", label);
        goto fail;
      }
      break;
    case AP_NARGS_FIXED:
      take = def->opts.nargs_count;
      if (values_left < take) {
        char label[96];
        ap_error_label_for_arg(def, label, sizeof(label));
        ap_error_set(err, AP_ERR_INVALID_NARGS, ap_error_argument_name(def),
                     "%s requires exactly %d values", label,
                     def->opts.nargs_count);
        goto fail;
      }
      break;
    }

    while (take-- > 0 && positional_cursor < positionals->count) {
      if (push_value(parsed, positional_defs[i],
                     positionals->items[positional_cursor], err) != 0) {
        goto fail;
      }
      positional_cursor++;
    }
  }

  if (positional_cursor < positionals->count) {
    if (allow_unknown && unknown_args) {
      while (positional_cursor < positionals->count) {
        char *extra = ap_strdup(positionals->items[positional_cursor]);
        if (!extra || ap_strvec_push(unknown_args, extra) != 0) {
          free(extra);
          ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
          goto fail;
        }
        positional_cursor++;
      }
      free(positional_defs);
      *out_parsed = parsed;
      return 0;
    }
    ap_error_set(err, AP_ERR_UNEXPECTED_POSITIONAL,
                 positionals->items[positional_cursor],
                 "unexpected positional argument '%s'",
                 positionals->items[positional_cursor]);
    goto fail;
  }

  free(positional_defs);
  *out_parsed = parsed;
  return 0;

fail: {
  int k;
  for (k = 0; k < parser->defs_count; k++) {
    ap_strvec_free(&parsed[k].values);
  }
  free(positional_defs);
  free(parsed);
  return -1;
}
}
