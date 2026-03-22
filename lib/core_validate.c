#include <string.h>

#include "ap_internal.h"

static bool has_value(const ap_parsed_arg *parsed) {
  return parsed->values.count > 0;
}

static int check_choices(const ap_arg_def *def, const ap_parsed_arg *parsed,
                         ap_error *err) {
  int i;
  int j;
  if (!def->opts.choices.items || def->opts.choices.count <= 0) {
    return 0;
  }

  for (i = 0; i < parsed->values.count; i++) {
    bool found = false;
    for (j = 0; j < def->opts.choices.count; j++) {
      if (strcmp(parsed->values.items[i], def->opts.choices.items[j]) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      ap_error_set(err, AP_ERR_INVALID_CHOICE, def->dest,
                   "invalid choice '%s' for argument '%s'",
                   parsed->values.items[i], def->dest);
      return -1;
    }
  }
  return 0;
}

int ap_validate_args(const ap_parser *parser, const ap_parsed_arg *parsed,
                     ap_error *err) {
  int i;
  bool help_requested = false;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (def->opts.action == AP_ACTION_STORE_TRUE &&
        strcmp(def->dest, "help") == 0 && parsed[i].seen) {
      help_requested = true;
      break;
    }
  }

  if (help_requested) {
    return 0;
  }

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    const ap_parsed_arg *p = &parsed[i];
    bool has_parsed_value = has_value(p);

    if (def->opts.action == AP_ACTION_STORE_TRUE ||
        def->opts.action == AP_ACTION_STORE_FALSE) {
      if (def->opts.required && !p->seen) {
        ap_error_set(err, AP_ERR_MISSING_REQUIRED, def->dest,
                     "option '%s' is required", def->flags[0]);
        return -1;
      }
      continue;
    }

    if (def->opts.required) {
      if (def->is_optional) {
        if (!p->seen) {
          ap_error_set(err, AP_ERR_MISSING_REQUIRED, def->dest,
                       "option '%s' is required", def->flags[0]);
          return -1;
        }
      } else if (!has_parsed_value) {
        ap_error_set(err, AP_ERR_MISSING_REQUIRED, def->dest,
                     "argument '%s' is required", def->dest);
        return -1;
      }
    }

    if (def->opts.nargs == AP_NARGS_ONE && p->seen && !has_parsed_value) {
      ap_error_set(err, AP_ERR_MISSING_VALUE, def->dest,
                   "argument '%s' needs a value", def->dest);
      return -1;
    }

    if (def->opts.nargs == AP_NARGS_ONE_OR_MORE && p->seen &&
        !has_parsed_value) {
      ap_error_set(err, AP_ERR_INVALID_NARGS, def->dest,
                   "argument '%s' requires one or more values", def->dest);
      return -1;
    }

    if (check_choices(def, p, err) != 0) {
      return -1;
    }
  }

  for (i = 0; i < parser->mutex_groups_count; i++) {
    const ap_mutually_exclusive_group *group = &parser->mutex_groups[i];
    int selected_count = 0;
    const ap_arg_def *selected_def = NULL;
    int j;

    for (j = 0; j < group->count; j++) {
      int def_index = group->def_indices[j];
      const ap_arg_def *def = &parser->defs[def_index];
      const ap_parsed_arg *p = &parsed[def_index];
      bool selected = false;

      if (def->opts.action == AP_ACTION_STORE_TRUE ||
          def->opts.action == AP_ACTION_STORE_FALSE) {
        selected = p->seen;
      } else {
        selected = p->seen || has_value(p);
      }

      if (selected) {
        selected_count++;
        if (!selected_def) {
          selected_def = def;
        }
      }
    }

    if (selected_count > 1) {
      ap_error_set(err, AP_ERR_MUTUALLY_EXCLUSIVE,
                   selected_def ? selected_def->dest : "",
                   "arguments in a mutually exclusive group cannot be used together");
      return -1;
    }

    if (group->required && selected_count == 0) {
      ap_error_set(err, AP_ERR_MISSING_REQUIRED, "mutually_exclusive_group",
                   "one argument from a mutually exclusive group is required");
      return -1;
    }
  }
  return 0;
}
