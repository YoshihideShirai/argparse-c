#include <stdlib.h>

#include "ap_internal.h"

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

static int ap_sb_append_roff_text(ap_string_builder *sb, const char *text) {
  size_t i;

  if (!text || text[0] == '\0') {
    return 0;
  }

  for (i = 0; text[i] != '\0'; i++) {
    char c = text[i];

    if ((i == 0 || text[i - 1] == '\n') && (c == '.' || c == '\'')) {
      if (ap_sb_appendf(sb, "\\&") != 0) {
        return -1;
      }
    }

    if (c == '\\') {
      if (ap_sb_appendf(sb, "\\\\") != 0) {
        return -1;
      }
    } else if (c == '-') {
      if (ap_sb_appendf(sb, "\\-") != 0) {
        return -1;
      }
    } else if (c == '\n') {
      if (ap_sb_appendf(sb, "\n") != 0) {
        return -1;
      }
    } else if (ap_sb_appendf(sb, "%c", c) != 0) {
      return -1;
    }
  }

  return 0;
}

static int append_nargs_suffix(ap_string_builder *sb, const ap_arg_def *def) {
  const char *mv = metavar_for(def);
  int i;

  if (def->opts.action != AP_ACTION_STORE &&
      def->opts.action != AP_ACTION_APPEND) {
    return 0;
  }

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

static int append_usage_synopsis(ap_string_builder *sb,
                                 const ap_parser *parser) {
  int i;

  if (ap_sb_appendf(sb, ".B ") != 0 ||
      ap_sb_append_roff_text(sb, parser->prog) != 0) {
    return -1;
  }

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];

    if (def->is_optional) {
      if (def->opts.required) {
        if (ap_sb_appendf(sb, " ") != 0) {
          return -1;
        }
      } else if (ap_sb_appendf(sb, " [") != 0) {
        return -1;
      }

      if (ap_sb_append_roff_text(sb, def->flags[0]) != 0 ||
          append_nargs_suffix(sb, def) != 0) {
        return -1;
      }

      if (!def->opts.required && ap_sb_appendf(sb, "]") != 0) {
        return -1;
      }
    } else {
      if (ap_sb_appendf(sb, " ") != 0 ||
          ap_sb_append_roff_text(sb, metavar_for(def)) != 0 ||
          append_nargs_suffix(sb, def) != 0) {
        return -1;
      }
    }
  }

  if (parser->subcommands_count > 0 &&
      ap_sb_appendf(sb, " [SUBCOMMAND]") != 0) {
    return -1;
  }

  return ap_sb_appendf(sb, "\n");
}

static int append_option_signature(ap_string_builder *sb,
                                   const ap_arg_def *def) {
  int i;

  for (i = 0; i < def->flags_count; i++) {
    if (i > 0 && ap_sb_appendf(sb, ", ") != 0) {
      return -1;
    }
    if (ap_sb_append_roff_text(sb, def->flags[i]) != 0) {
      return -1;
    }
  }

  return append_nargs_suffix(sb, def);
}

static int append_field_line(ap_string_builder *sb, const char *label,
                             const char *value) {
  if (!value || value[0] == '\0') {
    return 0;
  }

  if (ap_sb_appendf(sb, "\n") != 0 || ap_sb_append_roff_text(sb, label) != 0 ||
      ap_sb_appendf(sb, ": ") != 0 || ap_sb_append_roff_text(sb, value) != 0) {
    return -1;
  }

  return 0;
}

static int append_choices_line(ap_string_builder *sb,
                               const ap_choices *choices) {
  int i;

  if (!choices || !choices->items || choices->count <= 0) {
    return 0;
  }

  if (ap_sb_appendf(sb, "\nchoices: ") != 0) {
    return -1;
  }

  for (i = 0; i < choices->count; i++) {
    if (i > 0 && ap_sb_appendf(sb, ", ") != 0) {
      return -1;
    }
    if (ap_sb_append_roff_text(sb, choices->items[i]) != 0) {
      return -1;
    }
  }

  return 0;
}

static int append_options_section(ap_string_builder *sb,
                                  const ap_parser *parser, bool *has_options) {
  int i;
  bool emitted = false;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];

    if (!def->is_optional) {
      continue;
    }

    if (!emitted) {
      if (ap_sb_appendf(sb, ".SH OPTIONS\n") != 0) {
        return -1;
      }
      emitted = true;
    }

    if (ap_sb_appendf(sb, ".TP\n.B ") != 0 ||
        append_option_signature(sb, def) != 0 || ap_sb_appendf(sb, "\n") != 0) {
      return -1;
    }

    if (def->opts.help && def->opts.help[0] != '\0' &&
        ap_sb_append_roff_text(sb, def->opts.help) != 0) {
      return -1;
    }
    if (append_choices_line(sb, &def->opts.choices) != 0 ||
        append_field_line(sb, "default", def->opts.default_value) != 0 ||
        (def->opts.required && append_field_line(sb, "required", "yes") != 0)) {
      return -1;
    }
    if (ap_sb_appendf(sb, "\n") != 0) {
      return -1;
    }
  }

  *has_options = emitted;
  return 0;
}

static int append_subcommand_entry(ap_string_builder *sb,
                                   const ap_subcommand_def *subcommand,
                                   int depth) {
  int i;

  for (i = 0; i < depth; i++) {
    if (ap_sb_appendf(sb, ".RS\n") != 0) {
      return -1;
    }
  }

  if (ap_sb_appendf(sb, ".TP\n.B ") != 0 ||
      ap_sb_append_roff_text(sb, subcommand->name) != 0 ||
      ap_sb_appendf(sb, "\n") != 0) {
    return -1;
  }

  if (subcommand->help && subcommand->help[0] != '\0' &&
      ap_sb_append_roff_text(sb, subcommand->help) != 0) {
    return -1;
  }

  if (subcommand->parser) {
    if (ap_sb_appendf(sb, "\n") != 0 ||
        append_usage_synopsis(sb, subcommand->parser) != 0) {
      return -1;
    }

    for (i = 0; i < subcommand->parser->subcommands_count; i++) {
      if (append_subcommand_entry(sb, &subcommand->parser->subcommands[i],
                                  depth + 1) != 0) {
        return -1;
      }
    }
  }

  for (i = 0; i < depth; i++) {
    if (ap_sb_appendf(sb, ".RE\n") != 0) {
      return -1;
    }
  }

  return 0;
}

char *ap_manpage_build(const ap_parser *parser) {
  ap_string_builder sb;
  bool has_options = false;
  int i;

  if (!parser) {
    return NULL;
  }

  ap_sb_init(&sb);

  if (ap_sb_appendf(&sb, ".TH ") != 0 ||
      ap_sb_append_roff_text(&sb, parser->prog) != 0 ||
      ap_sb_appendf(&sb, " 1\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (ap_sb_appendf(&sb, ".SH NAME\n") != 0 ||
      ap_sb_append_roff_text(&sb, parser->prog) != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (parser->description[0] != '\0') {
    if (ap_sb_appendf(&sb, " \\- ") != 0 ||
        ap_sb_append_roff_text(&sb, parser->description) != 0) {
      ap_sb_free(&sb);
      return NULL;
    }
  }
  if (ap_sb_appendf(&sb, "\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (ap_sb_appendf(&sb, ".SH SYNOPSIS\n") != 0 ||
      append_usage_synopsis(&sb, parser) != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (ap_sb_appendf(&sb, ".SH DESCRIPTION\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (parser->description[0] != '\0') {
    if (ap_sb_append_roff_text(&sb, parser->description) != 0) {
      ap_sb_free(&sb);
      return NULL;
    }
  } else if (ap_sb_appendf(
                 &sb, "Command line interface generated by argparse\\-c.") !=
             0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (ap_sb_appendf(&sb, "\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (append_options_section(&sb, parser, &has_options) != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (!has_options && ap_sb_appendf(&sb, ".SH OPTIONS\nNone.\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (ap_sb_appendf(&sb, ".SH SUBCOMMANDS\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (parser->subcommands_count == 0) {
    if (ap_sb_appendf(&sb, "This command does not define subcommands.\n") !=
        0) {
      ap_sb_free(&sb);
      return NULL;
    }
  } else {
    for (i = 0; i < parser->subcommands_count; i++) {
      if (append_subcommand_entry(&sb, &parser->subcommands[i], 0) != 0) {
        ap_sb_free(&sb);
        return NULL;
      }
    }
  }

  if (ap_sb_appendf(
          &sb, ".SH EXIT STATUS\n"
               ".TP\n.B 0\n"
               "The command completed successfully.\n"
               ".TP\n.B non\\-zero\n"
               "Argument parsing, validation, or command dispatch failed.\n"
               ".SH ERRORS\n"
               "Typical failures include unknown options, missing values, "
               "invalid choices, required argument violations, and unexpected "
               "positional arguments.\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  return sb.data;
}
