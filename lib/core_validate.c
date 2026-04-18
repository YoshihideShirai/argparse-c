#include <string.h>

#include "ap_internal.h"

static bool has_value(const ap_parsed_arg *parsed) {
  return parsed->values.count > 0;
}

static int check_choices(const ap_arg_def *def, const ap_parsed_arg *parsed,
                         ap_error *err) {
  int i;
  int j;
  char label[96];
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
      ap_error_label_for_arg(def, label, sizeof(label));
      ap_error_set(err, AP_ERR_INVALID_CHOICE, ap_error_argument_name(def),
                   "invalid choice '%s' for %s", parsed->values.items[i],
                   label);
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
    char label[96];

    if (def->opts.action == AP_ACTION_STORE_TRUE ||
        def->opts.action == AP_ACTION_STORE_FALSE ||
        def->opts.action == AP_ACTION_COUNT ||
        def->opts.action == AP_ACTION_STORE_CONST) {
      if (def->opts.required && !p->seen) {
        ap_error_label_for_arg(def, label, sizeof(label));
        ap_error_set(err, AP_ERR_MISSING_REQUIRED, ap_error_argument_name(def),
                     "%s is required", label);
        return -1;
      }
      continue;
    }

    if (def->opts.required) {
      if (def->is_optional) {
        if (!p->seen) {
          ap_error_label_for_arg(def, label, sizeof(label));
          ap_error_set(err, AP_ERR_MISSING_REQUIRED,
                       ap_error_argument_name(def), "%s is required", label);
          return -1;
        }
      } else if (!has_parsed_value) {
        ap_error_label_for_arg(def, label, sizeof(label));
        ap_error_set(err, AP_ERR_MISSING_REQUIRED, ap_error_argument_name(def),
                     "%s is required", label);
        return -1;
      }
    }

    if (def->opts.nargs == AP_NARGS_ONE && p->seen && !has_parsed_value) {
      ap_error_label_for_arg(def, label, sizeof(label));
      ap_error_set(err, AP_ERR_MISSING_VALUE, ap_error_argument_name(def),
                   "%s requires a value", label);
      return -1;
    }

    if (def->opts.nargs == AP_NARGS_ONE_OR_MORE && p->seen &&
        !has_parsed_value) {
      ap_error_label_for_arg(def, label, sizeof(label));
      ap_error_set(err, AP_ERR_INVALID_NARGS, ap_error_argument_name(def),
                   "%s requires at least one value", label);
      return -1;
    }

    if (def->opts.nargs == AP_NARGS_FIXED && p->values.count > 0 &&
        p->values.count != def->opts.nargs_count) {
      ap_error_label_for_arg(def, label, sizeof(label));
      ap_error_set(err, AP_ERR_INVALID_NARGS, ap_error_argument_name(def),
                   "%s requires exactly %d values", label,
                   def->opts.nargs_count);
      return -1;
    }

    if (check_choices(def, p, err) != 0) {
      return -1;
    }
  }

  for (i = 0; i < parser->mutex_groups_count; i++) {
    const ap_mutex_group_def *group = &parser->mutex_groups[i];
    int j;
    int seen_count = 0;
    const char *first_name = NULL;
    for (j = 0; j < group->arg_count; j++) {
      const ap_arg_def *def = &parser->defs[group->arg_indexes[j]];
      const ap_parsed_arg *p = &parsed[group->arg_indexes[j]];
      bool active = p->seen || p->values.count > 0;
      if (!active) {
        continue;
      }
      seen_count++;
      if (!first_name) {
        first_name = def->is_optional ? def->flags[0] : def->dest;
      }
    }
    if (seen_count > 1) {
      ap_error_set(err, AP_ERR_INVALID_NARGS, first_name ? first_name : "",
                   "mutually exclusive arguments cannot be used together");
      return -1;
    }
    if (group->required && seen_count == 0) {
      ap_error_set(err, AP_ERR_MISSING_REQUIRED, "",
                   "one argument from a mutually exclusive group is required");
      return -1;
    }
  }
  return 0;
}
