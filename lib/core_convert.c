#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "ap_internal.h"

static int parse_int32(const char *text, int32_t *out_value) {
  char *end = NULL;
  long value;
  if (!text || !out_value) {
    return -1;
  }
  errno = 0;
  value = strtol(text, &end, 10);
  if (*text == '\0' || *end != '\0' || errno == ERANGE) {
    return -1;
  }
  if (value < INT32_MIN || value > INT32_MAX) {
    return -1;
  }
  *out_value = (int32_t)value;
  return 0;
}

static int parse_int64(const char *text, int64_t *out_value) {
  char *end = NULL;
  long long value;
  if (!text || !out_value) {
    return -1;
  }
  errno = 0;
  value = strtoll(text, &end, 10);
  if (*text == '\0' || *end != '\0' || errno == ERANGE) {
    return -1;
  }
  if (value < INT64_MIN || value > INT64_MAX) {
    return -1;
  }
  *out_value = (int64_t)value;
  return 0;
}

static int parse_uint64(const char *text, uint64_t *out_value) {
  char *end = NULL;
  unsigned long long value;
  if (!text || !out_value) {
    return -1;
  }
  errno = 0;
  value = strtoull(text, &end, 10);
  if (*text == '\0' || *end != '\0' || errno == ERANGE || text[0] == '-') {
    return -1;
  }
  *out_value = (uint64_t)value;
  return 0;
}

static int parse_double(const char *text, double *out_value) {
  char *end = NULL;
  double value;
  if (!text || !out_value) {
    return -1;
  }
  errno = 0;
  value = strtod(text, &end);
  if (*text == '\0' || *end != '\0' || errno == ERANGE) {
    return -1;
  }
  *out_value = value;
  return 0;
}

static int fill_values_from_default(const ap_arg_def *def, ap_strvec *values,
                                    ap_error *err) {
  char *copy;
  if (!def->opts.default_value) {
    return 0;
  }
  copy = ap_strdup(def->opts.default_value);
  if (!copy) {
    ap_error_set(err, AP_ERR_NO_MEMORY, def->dest, "out of memory");
    return -1;
  }
  if (ap_strvec_push(values, copy) != 0) {
    free(copy);
    ap_error_set(err, AP_ERR_NO_MEMORY, def->dest, "out of memory");
    return -1;
  }
  return 0;
}

static int copy_string_values(const ap_strvec *src, ap_ns_entry *dst,
                              ap_error *err) {
  int i;
  if (src->count == 0) {
    dst->as.strings = NULL;
    dst->count = 0;
    return 0;
  }
  dst->as.strings = calloc((size_t)src->count, sizeof(char *));
  if (!dst->as.strings) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dst->dest, "out of memory");
    return -1;
  }
  dst->count = src->count;
  for (i = 0; i < src->count; i++) {
    dst->as.strings[i] = ap_strdup(src->items[i]);
    if (!dst->as.strings[i]) {
      ap_error_set(err, AP_ERR_NO_MEMORY, dst->dest, "out of memory");
      return -1;
    }
  }
  return 0;
}

static int copy_int_values(const ap_arg_def *def, const ap_strvec *src,
                           ap_ns_entry *dst, ap_error *err) {
  int i;
  if (src->count == 0) {
    dst->as.ints = NULL;
    dst->count = 0;
    return 0;
  }
  dst->as.ints = calloc((size_t)src->count, sizeof(int32_t));
  if (!dst->as.ints) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dst->dest, "out of memory");
    return -1;
  }
  dst->count = src->count;
  for (i = 0; i < src->count; i++) {
    if (parse_int32(src->items[i], &dst->as.ints[i]) != 0) {
      ap_error_set(err, AP_ERR_INVALID_INT32, ap_error_argument_name(def),
                   "argument '%s' must be a valid int32: '%s'",
                   ap_error_argument_name(def), src->items[i]);
      return -1;
    }
  }
  return 0;
}

static int copy_int64_values(const ap_arg_def *def, const ap_strvec *src,
                             ap_ns_entry *dst, ap_error *err) {
  int i;
  if (src->count == 0) {
    dst->as.int64s = NULL;
    dst->count = 0;
    return 0;
  }
  dst->as.int64s = calloc((size_t)src->count, sizeof(int64_t));
  if (!dst->as.int64s) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dst->dest, "out of memory");
    return -1;
  }
  dst->count = src->count;
  for (i = 0; i < src->count; i++) {
    if (parse_int64(src->items[i], &dst->as.int64s[i]) != 0) {
      ap_error_set(err, AP_ERR_INVALID_INT64, ap_error_argument_name(def),
                   "argument '%s' must be a valid int64: '%s'",
                   ap_error_argument_name(def), src->items[i]);
      return -1;
    }
  }
  return 0;
}

static int copy_uint64_values(const ap_arg_def *def, const ap_strvec *src,
                              ap_ns_entry *dst, ap_error *err) {
  int i;
  if (src->count == 0) {
    dst->as.uint64s = NULL;
    dst->count = 0;
    return 0;
  }
  dst->as.uint64s = calloc((size_t)src->count, sizeof(uint64_t));
  if (!dst->as.uint64s) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dst->dest, "out of memory");
    return -1;
  }
  dst->count = src->count;
  for (i = 0; i < src->count; i++) {
    if (parse_uint64(src->items[i], &dst->as.uint64s[i]) != 0) {
      ap_error_set(err, AP_ERR_INVALID_UINT64, ap_error_argument_name(def),
                   "argument '%s' must be a valid uint64: '%s'",
                   ap_error_argument_name(def), src->items[i]);
      return -1;
    }
  }
  return 0;
}

static int copy_double_values(const ap_arg_def *def, const ap_strvec *src,
                              ap_ns_entry *dst, ap_error *err) {
  int i;
  if (src->count == 0) {
    dst->as.doubles = NULL;
    dst->count = 0;
    return 0;
  }
  dst->as.doubles = calloc((size_t)src->count, sizeof(double));
  if (!dst->as.doubles) {
    ap_error_set(err, AP_ERR_NO_MEMORY, dst->dest, "out of memory");
    return -1;
  }
  dst->count = src->count;
  for (i = 0; i < src->count; i++) {
    if (parse_double(src->items[i], &dst->as.doubles[i]) != 0) {
      ap_error_set(err, AP_ERR_INVALID_DOUBLE, ap_error_argument_name(def),
                   "argument '%s' must be a valid double: '%s'",
                   ap_error_argument_name(def), src->items[i]);
      return -1;
    }
  }
  return 0;
}

static int validate_choices_merged(const ap_arg_def *def,
                                   const ap_strvec *values, ap_error *err) {
  int i;
  int j;
  char label[96];
  if (!def->opts.choices.items || def->opts.choices.count <= 0) {
    return 0;
  }
  for (i = 0; i < values->count; i++) {
    bool found = false;
    for (j = 0; j < def->opts.choices.count; j++) {
      if (strcmp(values->items[i], def->opts.choices.items[j]) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      ap_error_label_for_arg(def, label, sizeof(label));
      ap_error_set(err, AP_ERR_INVALID_CHOICE, ap_error_argument_name(def),
                   "invalid choice '%s' for %s", values->items[i], label);
      return -1;
    }
  }
  return 0;
}

int ap_build_namespace(const ap_parser *parser, const ap_parsed_arg *parsed,
                       ap_namespace **out_ns, ap_error *err) {
  ap_namespace *ns;
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

  ns = calloc(1, sizeof(ap_namespace));
  if (!ns) {
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }

  ns->entries = calloc((size_t)parser->defs_count, sizeof(ap_ns_entry));
  if (!ns->entries) {
    ap_namespace_free(ns);
    ap_error_set(err, AP_ERR_NO_MEMORY, "", "out of memory");
    return -1;
  }
  ns->count = parser->defs_count;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    const ap_parsed_arg *p = &parsed[i];
    ap_ns_entry *entry = &ns->entries[i];
    ap_strvec merged = {0};

    entry->dest = ap_strdup(def->dest);
    if (!entry->dest) {
      ap_namespace_free(ns);
      ap_error_set(err, AP_ERR_NO_MEMORY, def->dest, "out of memory");
      return -1;
    }

    if (def->opts.action == AP_ACTION_STORE_TRUE) {
      entry->type = AP_NS_VALUE_BOOL;
      entry->count = 1;
      entry->as.boolean = p->seen ? true : false;
      continue;
    }
    if (def->opts.action == AP_ACTION_STORE_FALSE) {
      entry->type = AP_NS_VALUE_BOOL;
      entry->count = 1;
      entry->as.boolean = p->seen ? false : true;
      continue;
    }
    if (def->opts.action == AP_ACTION_COUNT) {
      entry->type = AP_NS_VALUE_INT32;
      entry->count = 1;
      entry->as.ints = calloc(1, sizeof(int32_t));
      if (!entry->as.ints) {
        ap_namespace_free(ns);
        ap_error_set(err, AP_ERR_NO_MEMORY, def->dest, "out of memory");
        return -1;
      }
      entry->as.ints[0] = p->values.count;
      continue;
    }

    if (p->values.count > 0) {
      int j;
      for (j = 0; j < p->values.count; j++) {
        char *copy = ap_strdup(p->values.items[j]);
        if (!copy || ap_strvec_push(&merged, copy) != 0) {
          free(copy);
          ap_strvec_free(&merged);
          ap_namespace_free(ns);
          ap_error_set(err, AP_ERR_NO_MEMORY, def->dest, "out of memory");
          return -1;
        }
      }
    } else if (fill_values_from_default(def, &merged, err) != 0) {
      ap_strvec_free(&merged);
      ap_namespace_free(ns);
      return -1;
    }

    if (validate_choices_merged(def, &merged, err) != 0) {
      ap_strvec_free(&merged);
      ap_namespace_free(ns);
      return -1;
    }

    if (!help_requested && def->opts.required && merged.count == 0 &&
        def->opts.nargs == AP_NARGS_ONE) {
      char label[96];
      ap_strvec_free(&merged);
      ap_namespace_free(ns);
      ap_error_label_for_arg(def, label, sizeof(label));
      ap_error_set(err, AP_ERR_MISSING_REQUIRED, ap_error_argument_name(def),
                   "%s is required", label);
      return -1;
    }

    if (def->opts.type == AP_TYPE_STRING) {
      entry->type = AP_NS_VALUE_STRING;
      if (copy_string_values(&merged, entry, err) != 0) {
        ap_strvec_free(&merged);
        ap_namespace_free(ns);
        return -1;
      }
    } else if (def->opts.type == AP_TYPE_INT32) {
      entry->type = AP_NS_VALUE_INT32;
      if (copy_int_values(def, &merged, entry, err) != 0) {
        ap_strvec_free(&merged);
        ap_namespace_free(ns);
        return -1;
      }
    } else if (def->opts.type == AP_TYPE_INT64) {
      entry->type = AP_NS_VALUE_INT64;
      if (copy_int64_values(def, &merged, entry, err) != 0) {
        ap_strvec_free(&merged);
        ap_namespace_free(ns);
        return -1;
      }
    } else if (def->opts.type == AP_TYPE_UINT64) {
      entry->type = AP_NS_VALUE_UINT64;
      if (copy_uint64_values(def, &merged, entry, err) != 0) {
        ap_strvec_free(&merged);
        ap_namespace_free(ns);
        return -1;
      }
    } else if (def->opts.type == AP_TYPE_DOUBLE) {
      entry->type = AP_NS_VALUE_DOUBLE;
      if (copy_double_values(def, &merged, entry, err) != 0) {
        ap_strvec_free(&merged);
        ap_namespace_free(ns);
        return -1;
      }
    } else {
      entry->type = AP_NS_VALUE_BOOL;
      entry->count = 1;
      entry->as.boolean = merged.count > 0;
    }

    ap_strvec_free(&merged);
  }

  *out_ns = ns;
  return 0;
}
