#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ap_internal.h"

extern int ap_test_sb_before_appendf(void) __attribute__((weak));
extern int ap_test_sb_before_free(void) __attribute__((weak));

void ap_sb_init(ap_string_builder *sb) {
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

void ap_sb_free(ap_string_builder *sb) {
  if (!sb) {
    return;
  }
  if (!(ap_test_sb_before_free && ap_test_sb_before_free() != 0)) {
    free(sb->data);
  }
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

int ap_sb_appendf(ap_string_builder *sb, const char *fmt, ...) {
  va_list ap;
  va_list ap_copy;
  int needed;
  size_t required;
  char *next;

  if (!sb || !fmt) {
    return -1;
  }
  if (ap_test_sb_before_appendf && ap_test_sb_before_appendf() != 0) {
    return -1;
  }

  va_start(ap, fmt);
  va_copy(ap_copy, ap);
  needed = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);

  if (needed < 0) {
    va_end(ap_copy);
    return -1;
  }

  required = sb->len + (size_t)needed + 1;
  if (required > sb->cap) {
    size_t next_cap = sb->cap == 0 ? 128 : sb->cap;
    while (next_cap < required) {
      next_cap *= 2;
    }
    next = realloc(sb->data, next_cap);
    if (!next) {
      va_end(ap_copy);
      return -1;
    }
    sb->data = next;
    sb->cap = next_cap;
  }

  vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, ap_copy);
  va_end(ap_copy);
  sb->len += (size_t)needed;
  return 0;
}

static const char *metavar_for(const ap_arg_def *def) {
  static char fallback[64];
  size_t i;
  if (def->opts.metavar && def->opts.metavar[0] != '\0') {
    return def->opts.metavar;
  }
  for (i = 0; i < sizeof(fallback) - 1 && def->dest[i] != '\0'; i++) {
    char c = def->dest[i];
    if (c >= 'a' && c <= 'z') {
      fallback[i] = (char)(c - 'a' + 'A');
    } else if (c == '_') {
      fallback[i] = '-';
    } else {
      fallback[i] = c;
    }
  }
  fallback[i] = '\0';
  return fallback;
}

static int append_optional_usage(ap_string_builder *sb, const ap_arg_def *def) {
  const char *mv = metavar_for(def);
  int i;

  if (def->opts.action == AP_ACTION_STORE_TRUE ||
      def->opts.action == AP_ACTION_STORE_FALSE ||
      def->opts.action == AP_ACTION_COUNT ||
      def->opts.action == AP_ACTION_STORE_CONST) {
    return ap_sb_appendf(sb, def->opts.required ? " %s" : " [%s]",
                         def->flags[0]);
  }

  switch (def->opts.nargs) {
  case AP_NARGS_ONE:
    return ap_sb_appendf(sb, def->opts.required ? " %s %s" : " [%s %s]",
                         def->flags[0], mv);
  case AP_NARGS_OPTIONAL:
    return ap_sb_appendf(sb, def->opts.required ? " %s [%s]" : " [%s [%s]]",
                         def->flags[0], mv);
  case AP_NARGS_ZERO_OR_MORE:
    return ap_sb_appendf(sb,
                         def->opts.required ? " %s [%s ...]" : " [%s [%s ...]]",
                         def->flags[0], mv);
  case AP_NARGS_ONE_OR_MORE:
    return ap_sb_appendf(
        sb, def->opts.required ? " %s %s [%s ...]" : " [%s %s [%s ...]]",
        def->flags[0], mv, mv);
  case AP_NARGS_FIXED:
    if (ap_sb_appendf(sb, def->opts.required ? " %s" : " [%s", def->flags[0]) !=
        0) {
      return -1;
    }
    for (i = 0; i < def->opts.nargs_count; i++) {
      if (ap_sb_appendf(sb, " %s", mv) != 0) {
        return -1;
      }
    }
    return ap_sb_appendf(sb, def->opts.required ? "" : "]");
  }
  return 0;
}

static int append_positional_usage(ap_string_builder *sb,
                                   const ap_arg_def *def) {
  const char *mv = metavar_for(def);
  int i;
  switch (def->opts.nargs) {
  case AP_NARGS_ONE:
    return ap_sb_appendf(sb, " %s", mv);
  case AP_NARGS_OPTIONAL:
    return ap_sb_appendf(sb, " [%s]", mv);
  case AP_NARGS_ZERO_OR_MORE:
    return ap_sb_appendf(sb, " [%s ...]", mv);
  case AP_NARGS_ONE_OR_MORE:
    return ap_sb_appendf(sb, " %s [%s ...]", mv, mv);
  case AP_NARGS_FIXED:
    for (i = 0; i < def->opts.nargs_count; i++) {
      if (ap_sb_appendf(sb, " %s", mv) != 0) {
        return -1;
      }
    }
    return 0;
  }
  return 0;
}

static int append_help_line(ap_string_builder *sb, const ap_arg_def *def) {
  const char *mv = metavar_for(def);
  bool has_meta = false;
  int i;

  if (def->opts.action == AP_ACTION_STORE ||
      def->opts.action == AP_ACTION_APPEND) {
    if (def->opts.nargs == AP_NARGS_ONE) {
      has_meta = true;
    } else if (def->opts.nargs == AP_NARGS_OPTIONAL) {
      has_meta = true;
    } else if (def->opts.nargs == AP_NARGS_ZERO_OR_MORE) {
      has_meta = true;
    } else if (def->opts.nargs == AP_NARGS_ONE_OR_MORE) {
      has_meta = true;
    } else if (def->opts.nargs == AP_NARGS_FIXED) {
      has_meta = true;
    }
  }

  if (def->is_optional) {
    if (ap_sb_appendf(sb, "  ") != 0) {
      return -1;
    }
    for (i = 0; i < def->flags_count; i++) {
      if (i > 0 && ap_sb_appendf(sb, ", ") != 0) {
        return -1;
      }
      if (ap_sb_appendf(sb, "%s", def->flags[i]) != 0) {
        return -1;
      }
    }
    if (has_meta) {
      switch (def->opts.nargs) {
      case AP_NARGS_ONE:
        if (ap_sb_appendf(sb, " %s", mv) != 0) {
          return -1;
        }
        break;
      case AP_NARGS_OPTIONAL:
        if (ap_sb_appendf(sb, " [%s]", mv) != 0) {
          return -1;
        }
        break;
      case AP_NARGS_ZERO_OR_MORE:
        if (ap_sb_appendf(sb, " [%s ...]", mv) != 0) {
          return -1;
        }
        break;
      case AP_NARGS_ONE_OR_MORE:
        if (ap_sb_appendf(sb, " %s [%s ...]", mv, mv) != 0) {
          return -1;
        }
        break;
      case AP_NARGS_FIXED: {
        int idx;
        for (idx = 0; idx < def->opts.nargs_count; idx++) {
          if (ap_sb_appendf(sb, " %s", mv) != 0) {
            return -1;
          }
        }
        break;
      }
      }
    }
    if (def->opts.help && ap_sb_appendf(sb, "\n    %s", def->opts.help) != 0) {
      return -1;
    }
    if (def->opts.choices.items && def->opts.choices.count > 0) {
      if (ap_sb_appendf(sb, "\n    choices: ") != 0) {
        return -1;
      }
      for (i = 0; i < def->opts.choices.count; i++) {
        if (i > 0 && ap_sb_appendf(sb, ", ") != 0) {
          return -1;
        }
        if (ap_sb_appendf(sb, "%s", def->opts.choices.items[i]) != 0) {
          return -1;
        }
      }
    }
    if (def->opts.default_value &&
        ap_sb_appendf(sb, "\n    default: %s", def->opts.default_value) != 0) {
      return -1;
    }
    if (def->opts.required && ap_sb_appendf(sb, "\n    required: true") != 0) {
      return -1;
    }
    return ap_sb_appendf(sb, "\n");
  }

  if (ap_sb_appendf(sb, "  %s", mv) != 0) {
    return -1;
  }
  if (has_meta) {
    switch (def->opts.nargs) {
    case AP_NARGS_ONE:
      break;
    case AP_NARGS_OPTIONAL:
      if (ap_sb_appendf(sb, " [%s]", mv) != 0) {
        return -1;
      }
      break;
    case AP_NARGS_ZERO_OR_MORE:
      if (ap_sb_appendf(sb, " [%s ...]", mv) != 0) {
        return -1;
      }
      break;
    case AP_NARGS_ONE_OR_MORE:
      if (ap_sb_appendf(sb, " [%s ...]", mv) != 0) {
        return -1;
      }
      break;
    case AP_NARGS_FIXED: {
      int idx;
      for (idx = 1; idx < def->opts.nargs_count; idx++) {
        if (ap_sb_appendf(sb, " %s", mv) != 0) {
          return -1;
        }
      }
      break;
    }
    }
  }
  if (def->opts.help && ap_sb_appendf(sb, "\n    %s", def->opts.help) != 0) {
    return -1;
  }
  if (def->opts.choices.items && def->opts.choices.count > 0) {
    if (ap_sb_appendf(sb, "\n    choices: ") != 0) {
      return -1;
    }
    for (i = 0; i < def->opts.choices.count; i++) {
      if (i > 0 && ap_sb_appendf(sb, ", ") != 0) {
        return -1;
      }
      if (ap_sb_appendf(sb, "%s", def->opts.choices.items[i]) != 0) {
        return -1;
      }
    }
  }
  if (def->opts.default_value &&
      ap_sb_appendf(sb, "\n    default: %s", def->opts.default_value) != 0) {
    return -1;
  }
  if (def->opts.required && ap_sb_appendf(sb, "\n    required: true") != 0) {
    return -1;
  }
  return ap_sb_appendf(sb, "\n");
}

char *ap_usage_build(const ap_parser *parser) {
  ap_string_builder sb;
  int i;

  if (!parser) {
    return NULL;
  }

  ap_sb_init(&sb);
  if (ap_sb_appendf(&sb, "usage: %s", parser->prog) != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    int rc = def->is_optional ? append_optional_usage(&sb, def)
                              : append_positional_usage(&sb, def);
    if (rc != 0) {
      ap_sb_free(&sb);
      return NULL;
    }
  }

  if (parser->subcommands_count > 0 &&
      ap_sb_appendf(&sb, " <%s>", "subcommand") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (ap_sb_appendf(&sb, "\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  return sb.data;
}

char *ap_help_build(const ap_parser *parser) {
  ap_string_builder sb;
  int i;
  bool has_positionals = false;
  bool has_optionals = false;
  bool has_subcommands = false;

  if (!parser) {
    return NULL;
  }

  ap_sb_init(&sb);

  {
    char *usage = ap_usage_build(parser);
    if (!usage) {
      ap_sb_free(&sb);
      return NULL;
    }
    if (ap_sb_appendf(&sb, "%s", usage) != 0) {
      free(usage);
      ap_sb_free(&sb);
      return NULL;
    }
    free(usage);
  }

  if (parser->description[0] != '\0' &&
      ap_sb_appendf(&sb, "\n%s\n", parser->description) != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  for (i = 0; i < parser->defs_count; i++) {
    if (parser->defs[i].is_optional) {
      has_optionals = true;
    } else {
      has_positionals = true;
    }
  }
  has_subcommands = parser->subcommands_count > 0;

  if (has_positionals) {
    if (ap_sb_appendf(&sb, "\npositional arguments:\n") != 0) {
      ap_sb_free(&sb);
      return NULL;
    }
    for (i = 0; i < parser->defs_count; i++) {
      if (!parser->defs[i].is_optional &&
          append_help_line(&sb, &parser->defs[i]) != 0) {
        ap_sb_free(&sb);
        return NULL;
      }
    }
  }

  if (has_optionals) {
    if (ap_sb_appendf(&sb, "\noptional arguments:\n") != 0) {
      ap_sb_free(&sb);
      return NULL;
    }
    for (i = 0; i < parser->defs_count; i++) {
      if (parser->defs[i].is_optional &&
          append_help_line(&sb, &parser->defs[i]) != 0) {
        ap_sb_free(&sb);
        return NULL;
      }
    }
  }

  if (has_subcommands) {
    if (ap_sb_appendf(&sb, "\nsubcommands:\n") != 0) {
      ap_sb_free(&sb);
      return NULL;
    }
    for (i = 0; i < parser->subcommands_count; i++) {
      if (ap_sb_appendf(&sb, "  %s", parser->subcommands[i].name) != 0) {
        ap_sb_free(&sb);
        return NULL;
      }
      if (parser->subcommands[i].help[0] != '\0' &&
          ap_sb_appendf(&sb, "\n    %s", parser->subcommands[i].help) != 0) {
        ap_sb_free(&sb);
        return NULL;
      }
      if (ap_sb_appendf(&sb, "\n") != 0) {
        ap_sb_free(&sb);
        return NULL;
      }
    }
  }

  return sb.data;
}
