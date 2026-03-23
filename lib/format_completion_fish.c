#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "ap_internal.h"

static bool action_takes_no_value(ap_action action) {
  return action == AP_ACTION_STORE_TRUE || action == AP_ACTION_STORE_FALSE ||
         action == AP_ACTION_COUNT || action == AP_ACTION_STORE_CONST;
}

static bool option_takes_value(const ap_arg_def *def) {
  return def && def->is_optional && !action_takes_no_value(def->opts.action);
}

static ap_completion_kind option_completion_kind(const ap_arg_def *def) {
  if (!option_takes_value(def)) {
    return AP_COMPLETION_KIND_NONE;
  }
  if (def->opts.completion_kind != AP_COMPLETION_KIND_NONE) {
    return def->opts.completion_kind;
  }
  if (def->opts.choices.items && def->opts.choices.count > 0) {
    return AP_COMPLETION_KIND_CHOICES;
  }
  return AP_COMPLETION_KIND_NONE;
}

static bool option_has_dynamic_completion(const ap_arg_def *def) {
  return option_takes_value(def) && def->opts.completion_callback != NULL;
}

static const char *metavar_for(const ap_arg_def *def) {
  static char fallback[64];
  size_t i;

  if (def->opts.metavar && def->opts.metavar[0] != '\0') {
    return def->opts.metavar;
  }
  for (i = 0; i < sizeof(fallback) - 1 && def->dest && def->dest[i] != '\0';
       i++) {
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

static int append_fish_double_quoted(ap_string_builder *sb, const char *text) {
  const unsigned char *p = (const unsigned char *)(text ? text : "");

  if (ap_sb_appendf(sb, "\"") != 0) {
    return -1;
  }
  while (*p != '\0') {
    if (*p == '\\' || *p == '"' || *p == '$') {
      if (ap_sb_appendf(sb, "\\%c", (char)*p) != 0) {
        return -1;
      }
    } else if (*p == '\n' || *p == '\r') {
      if (ap_sb_appendf(sb, " ") != 0) {
        return -1;
      }
    } else if (ap_sb_appendf(sb, "%c", (char)*p) != 0) {
      return -1;
    }
    p++;
  }
  return ap_sb_appendf(sb, "\"");
}

static int append_identifier(ap_string_builder *sb, const char *text) {
  size_t i;
  const char *value = text ? text : "argparse_c";

  for (i = 0; value[i] != '\0'; i++) {
    unsigned char c = (unsigned char)value[i];
    if (isalnum(c) || c == '_') {
      if (ap_sb_appendf(sb, "%c", (char)c) != 0) {
        return -1;
      }
    } else if (ap_sb_appendf(sb, "_") != 0) {
      return -1;
    }
  }
  return 0;
}

static int append_parser_key(ap_string_builder *sb, const ap_parser *parser) {
  if (!parser || !parser->parent) {
    return ap_sb_appendf(sb, "root");
  }
  if (append_parser_key(sb, parser->parent) != 0) {
    return -1;
  }
  return ap_sb_appendf(sb, "/%s", parser->command_name);
}

static int append_description(ap_string_builder *sb, const ap_arg_def *def) {
  const char *help = def->opts.help;

  if (help && help[0] != '\0') {
    if (ap_sb_appendf(sb, " -d ") != 0 ||
        append_fish_double_quoted(sb, help) != 0) {
      return -1;
    }
  } else if (option_takes_value(def)) {
    if (ap_sb_appendf(sb, " -d ") != 0 ||
        append_fish_double_quoted(sb, metavar_for(def)) != 0) {
      return -1;
    }
  }
  return 0;
}

static int append_transition_cases(ap_string_builder *sb,
                                   const ap_parser *parser) {
  int i;

  for (i = 0; i < parser->subcommands_count; i++) {
    if (ap_sb_appendf(sb, "    case \"") != 0) {
      return -1;
    }
    if (append_parser_key(sb, parser) != 0 ||
        ap_sb_appendf(sb, ":%s\"\n", parser->subcommands[i].name) != 0 ||
        ap_sb_appendf(sb, "      set key \"") != 0) {
      return -1;
    }
    if (append_parser_key(sb, parser->subcommands[i].parser) != 0 ||
        ap_sb_appendf(sb, "\"\n") != 0) {
      return -1;
    }
    if (append_transition_cases(sb, parser->subcommands[i].parser) != 0) {
      return -1;
    }
  }
  return 0;
}

static int append_choice_cases(ap_string_builder *sb, const ap_parser *parser) {
  int i;
  int j;
  int k;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (option_completion_kind(def) != AP_COMPLETION_KIND_CHOICES ||
        !def->opts.choices.items || def->opts.choices.count <= 0) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (ap_sb_appendf(sb, "    case \"") != 0) {
        return -1;
      }
      if (append_parser_key(sb, parser) != 0 ||
          ap_sb_appendf(sb, ":%s\"\n      printf '%%s\\n'", def->flags[j]) !=
              0) {
        return -1;
      }
      for (k = 0; k < def->opts.choices.count; k++) {
        if (ap_sb_appendf(sb, " ") != 0 ||
            append_fish_double_quoted(sb, def->opts.choices.items[k]) != 0) {
          return -1;
        }
      }
      if (ap_sb_appendf(sb, "\n") != 0) {
        return -1;
      }
    }
  }

  for (i = 0; i < parser->subcommands_count; i++) {
    if (append_choice_cases(sb, parser->subcommands[i].parser) != 0) {
      return -1;
    }
  }
  return 0;
}

static int append_option_complete(ap_string_builder *sb, const char *prog,
                                  const ap_parser *parser,
                                  const ap_arg_def *def) {
  int i;
  ap_completion_kind completion_kind = option_completion_kind(def);

  if (ap_sb_appendf(sb, "complete -c ") != 0 ||
      append_fish_double_quoted(sb, prog) != 0 ||
      ap_sb_appendf(sb, " -n '") != 0) {
    return -1;
  }
  if (ap_sb_appendf(sb, "__ap_") != 0 || append_identifier(sb, prog) != 0 ||
      ap_sb_appendf(sb, "_parser_is ") != 0 ||
      append_parser_key(sb, parser) != 0 || ap_sb_appendf(sb, "'") != 0) {
    return -1;
  }

  for (i = 0; i < def->flags_count; i++) {
    const char *flag = def->flags[i];
    if (ap_is_short_flag(flag)) {
      if (ap_sb_appendf(sb, " -s %c", flag[1]) != 0) {
        return -1;
      }
    } else if (ap_is_long_flag(flag)) {
      if (ap_sb_appendf(sb, " -l %s", flag + 2) != 0) {
        return -1;
      }
    }
  }

  if (append_description(sb, def) != 0) {
    return -1;
  }

  if (option_takes_value(def)) {
    if (ap_sb_appendf(sb, " -r") != 0) {
      return -1;
    }
    if (option_has_dynamic_completion(def)) {
      if (ap_sb_appendf(sb, " -a '(__ap_") != 0 ||
          append_identifier(sb, prog) != 0 ||
          ap_sb_appendf(sb, "_dynamic_complete)'") != 0) {
        return -1;
      }
      return ap_sb_appendf(sb, "\n");
    }
    switch (completion_kind) {
    case AP_COMPLETION_KIND_CHOICES:
      if (ap_sb_appendf(sb, " -a '(__ap_") != 0 ||
          append_identifier(sb, prog) != 0 ||
          ap_sb_appendf(sb, "_value_choices ") != 0) {
        return -1;
      }
      if (append_parser_key(sb, parser) != 0 ||
          ap_sb_appendf(sb, ":%s)'", def->flags[def->flags_count - 1]) != 0) {
        return -1;
      }
      break;
    case AP_COMPLETION_KIND_FILE:
      if (ap_sb_appendf(sb, " -F") != 0) {
        return -1;
      }
      break;
    case AP_COMPLETION_KIND_DIRECTORY:
      if (ap_sb_appendf(sb, " -a '(__fish_complete_directories)'") != 0) {
        return -1;
      }
      break;
    case AP_COMPLETION_KIND_COMMAND:
      if (ap_sb_appendf(sb, " -a '(__fish_complete_command)'") != 0) {
        return -1;
      }
      break;
    case AP_COMPLETION_KIND_NONE:
    default:
      break;
    }
  }

  return ap_sb_appendf(sb, "\n");
}

static int append_subcommand_complete(ap_string_builder *sb, const char *prog,
                                      const ap_parser *parser,
                                      const ap_subcommand_def *sub) {
  if (ap_sb_appendf(sb, "complete -c ") != 0 ||
      append_fish_double_quoted(sb, prog) != 0 ||
      ap_sb_appendf(sb, " -n '") != 0) {
    return -1;
  }
  if (ap_sb_appendf(sb, "__ap_") != 0 || append_identifier(sb, prog) != 0 ||
      ap_sb_appendf(sb, "_parser_is ") != 0 ||
      append_parser_key(sb, parser) != 0 ||
      ap_sb_appendf(sb, "' -f -a ") != 0 ||
      append_fish_double_quoted(sb, sub->name) != 0) {
    return -1;
  }
  if (sub->help && sub->help[0] != '\0') {
    if (ap_sb_appendf(sb, " -d ") != 0 ||
        append_fish_double_quoted(sb, sub->help) != 0) {
      return -1;
    }
  }
  return ap_sb_appendf(sb, "\n");
}

static int append_complete_commands(ap_string_builder *sb, const char *prog,
                                    const ap_parser *parser) {
  int i;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!def->is_optional) {
      continue;
    }
    if (append_option_complete(sb, prog, parser, def) != 0) {
      return -1;
    }
  }

  for (i = 0; i < parser->subcommands_count; i++) {
    if (append_subcommand_complete(sb, prog, parser, &parser->subcommands[i]) !=
        0) {
      return -1;
    }
    if (append_complete_commands(sb, prog, parser->subcommands[i].parser) !=
        0) {
      return -1;
    }
  }
  return 0;
}

char *ap_fish_completion_build(const ap_parser *parser) {
  ap_string_builder sb;
  const char *prog;

  if (!parser || !parser->prog || parser->prog[0] == '\0') {
    return NULL;
  }
  prog = parser->command_name && parser->command_name[0] != '\0'
             ? parser->command_name
             : parser->prog;

  ap_sb_init(&sb);
  if (ap_sb_appendf(&sb, "# fish completion for %s\n", parser->prog) != 0 ||
      ap_sb_appendf(&sb, "complete -c ") != 0 ||
      append_fish_double_quoted(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, " -f\n\nfunction __ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_parser_key\n"
                         "  set -l tokens (commandline -opc)\n"
                         "  set -e tokens[1]\n"
                         "  set -l key root\n"
                         "  for token in $tokens\n"
                         "    switch \"$key:$token\"\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (append_transition_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb,
                    "    end\n"
                    "  end\n"
                    "  printf '%s\\n' \"$key\"\n"
                    "end\n\n"
                    "function __ap_",
                    "%s") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_parser_is\n"
                         "  test (__ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_parser_key) = \"$argv[1]\"\n"
                         "end\n\n"
                         "function __ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_value_choices\n"
                         "  switch \"$argv[1]\"\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  if (append_choice_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb, "  end\n"
                         "end\n\nfunction __ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_dynamic_complete\n"
                         "  set -l tokens (commandline -opc)\n"
                         "  set -e tokens[1]\n"
                         "  set -l current (commandline -ct)\n"
                         "  ") != 0 ||
      append_fish_double_quoted(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, " ") != 0 ||
      append_fish_double_quoted(&sb, ap_parser_completion_entrypoint(parser)) !=
          0 ||
      ap_sb_appendf(&sb, " --shell fish -- $tokens $current\n"
                         "end\n\nfunction __ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_positional_complete\n"
                         "  set -l current (commandline -ct)\n"
                         "  if string match -qr '^-' -- \"$current\"\n"
                         "    return 1\n"
                         "  end\n"
                         "  __ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_dynamic_complete\n"
                         "end\n\n") != 0 ||
      append_complete_commands(&sb, prog, parser) != 0 ||
      ap_sb_appendf(&sb, "complete -c ") != 0 ||
      append_fish_double_quoted(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, " -f -a '(__ap_") != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(&sb, "_positional_complete)'\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  return sb.data;
}
