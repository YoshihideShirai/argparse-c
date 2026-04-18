#include <ctype.h>
#include <stdlib.h>

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

static int option_value_count(const ap_arg_def *def) {
  if (!option_takes_value(def)) {
    return 0;
  }
  if (def->opts.nargs == AP_NARGS_FIXED) {
    return def->opts.nargs_count;
  }
  return 1;
}

static const char *option_value_mode(const ap_arg_def *def) {
  if (!option_takes_value(def)) {
    return "none";
  }
  switch (def->opts.nargs) {
  case AP_NARGS_OPTIONAL:
    return "optional";
  case AP_NARGS_ZERO_OR_MORE:
  case AP_NARGS_ONE_OR_MORE:
    return "multi";
  case AP_NARGS_FIXED:
    return def->opts.nargs_count > 1 ? "fixed" : "single";
  case AP_NARGS_ONE:
  default:
    return "single";
  }
}

static const char *completion_dispatch_kind_name(const ap_arg_def *def) {
  if (option_has_dynamic_completion(def)) {
    return "dynamic";
  }
  switch (option_completion_kind(def)) {
  case AP_COMPLETION_KIND_CHOICES:
    return "choices";
  case AP_COMPLETION_KIND_FILE:
    return "file";
  case AP_COMPLETION_KIND_DIRECTORY:
    return "directory";
  case AP_COMPLETION_KIND_COMMAND:
    return "command";
  case AP_COMPLETION_KIND_NONE:
  default:
    return "none";
  }
}

static int append_single_quoted(ap_string_builder *sb, const char *text) {
  const unsigned char *p = (const unsigned char *)(text ? text : "");

  if (ap_sb_appendf(sb, "'") != 0) {
    return -1;
  }
  while (*p != '\0') {
    if (*p == '\'') {
      if (ap_sb_appendf(sb, "'\\''") != 0) {
        return -1;
      }
    } else if (ap_sb_appendf(sb, "%c", (char)*p) != 0) {
      return -1;
    }
    p++;
  }
  return ap_sb_appendf(sb, "'");
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

static int append_load_parser_cases(ap_string_builder *sb,
                                    const ap_parser *parser) {
  int i;
  int j;
  bool first;

  if (ap_sb_appendf(sb, "      ") != 0 || append_parser_key(sb, parser) != 0 ||
      ap_sb_appendf(sb, ")\n        parser_subcommands=") != 0) {
    return -1;
  }
  if (ap_sb_appendf(sb, "'") != 0) {
    return -1;
  }
  for (i = 0; i < parser->subcommands_count; i++) {
    if (i > 0 && ap_sb_appendf(sb, " ") != 0) {
      return -1;
    }
    if (ap_sb_appendf(sb, "%s", parser->subcommands[i].name) != 0) {
      return -1;
    }
  }
  if (ap_sb_appendf(sb, "'\n        parser_options='") != 0) {
    return -1;
  }
  first = true;
  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!def->is_optional) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (!first && ap_sb_appendf(sb, " ") != 0) {
        return -1;
      }
      first = false;
      if (ap_sb_appendf(sb, "%s", def->flags[j]) != 0) {
        return -1;
      }
    }
  }
  if (ap_sb_appendf(sb, "'\n        parser_value_options='") != 0) {
    return -1;
  }
  first = true;
  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!option_takes_value(def)) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (!first && ap_sb_appendf(sb, " ") != 0) {
        return -1;
      }
      first = false;
      if (ap_sb_appendf(sb, "%s", def->flags[j]) != 0) {
        return -1;
      }
    }
  }
  if (ap_sb_appendf(sb, "'\n        parser_flag_only_options='") != 0) {
    return -1;
  }
  first = true;
  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!def->is_optional || option_takes_value(def)) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (!first && ap_sb_appendf(sb, " ") != 0) {
        return -1;
      }
      first = false;
      if (ap_sb_appendf(sb, "%s", def->flags[j]) != 0) {
        return -1;
      }
    }
  }
  if (ap_sb_appendf(sb, "'\n        ;;\n") != 0) {
    return -1;
  }

  for (i = 0; i < parser->subcommands_count; i++) {
    if (append_load_parser_cases(sb, parser->subcommands[i].parser) != 0) {
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
      if (ap_sb_appendf(sb, "      ") != 0 ||
          append_parser_key(sb, parser) != 0 ||
          ap_sb_appendf(sb, ":%s) printf '%%s\\n' ", def->flags[j]) != 0) {
        return -1;
      }
      for (k = 0; k < def->opts.choices.count; k++) {
        if (k > 0 && ap_sb_appendf(sb, " ") != 0) {
          return -1;
        }
        if (append_single_quoted(sb, def->opts.choices.items[k]) != 0) {
          return -1;
        }
      }
      if (ap_sb_appendf(sb, ";;\n") != 0) {
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

static int append_value_mode_cases(ap_string_builder *sb,
                                   const ap_parser *parser) {
  int i;
  int j;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!option_takes_value(def)) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (ap_sb_appendf(sb, "      ") != 0 ||
          append_parser_key(sb, parser) != 0 ||
          ap_sb_appendf(sb, ":%s) printf '%%s\\n' '%s' ;;\n", def->flags[j],
                        option_value_mode(def)) != 0) {
        return -1;
      }
    }
  }
  for (i = 0; i < parser->subcommands_count; i++) {
    if (append_value_mode_cases(sb, parser->subcommands[i].parser) != 0) {
      return -1;
    }
  }
  return 0;
}

static int append_completion_kind_cases(ap_string_builder *sb,
                                        const ap_parser *parser) {
  int i;
  int j;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!option_takes_value(def)) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (ap_sb_appendf(sb, "      ") != 0 ||
          append_parser_key(sb, parser) != 0 ||
          ap_sb_appendf(sb, ":%s) printf '%%s\\n' '%s' ;;\n", def->flags[j],
                        completion_dispatch_kind_name(def)) != 0) {
        return -1;
      }
    }
  }
  for (i = 0; i < parser->subcommands_count; i++) {
    if (append_completion_kind_cases(sb, parser->subcommands[i].parser) != 0) {
      return -1;
    }
  }
  return 0;
}

static int append_value_count_cases(ap_string_builder *sb,
                                    const ap_parser *parser) {
  int i;
  int j;

  for (i = 0; i < parser->defs_count; i++) {
    const ap_arg_def *def = &parser->defs[i];
    if (!option_takes_value(def)) {
      continue;
    }
    for (j = 0; j < def->flags_count; j++) {
      if (ap_sb_appendf(sb, "      ") != 0 ||
          append_parser_key(sb, parser) != 0 ||
          ap_sb_appendf(sb, ":%s) printf '%%d\\n' %d ;;\n", def->flags[j],
                        option_value_count(def)) != 0) {
        return -1;
      }
    }
  }
  for (i = 0; i < parser->subcommands_count; i++) {
    if (append_value_count_cases(sb, parser->subcommands[i].parser) != 0) {
      return -1;
    }
  }
  return 0;
}

char *ap_bash_completion_build(const ap_parser *parser) {
  ap_string_builder sb;
  const char *prog;

  if (!parser || !parser->prog || parser->prog[0] == '\0') {
    return NULL;
  }
  prog = parser->command_name && parser->command_name[0] != '\0'
             ? parser->command_name
             : parser->prog;

  ap_sb_init(&sb);
  if (ap_sb_appendf(&sb, "# bash completion for %s\n_", parser->prog) != 0 ||
      append_identifier(&sb, prog) != 0 ||
      ap_sb_appendf(
          &sb, "() {\n"
               "  local cur parser_key parser_subcommands parser_options "
               "parser_value_options parser_flag_only_options\n"
               "  local pending_option pending_mode pending_fixed_remaining "
               "option_name value_prefix token choices completion_kind\n"
               "  local positional_phase\n"
               "  local -a filtered candidates\n"
               "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
               "  parser_key='root'\n"
               "  pending_option=''\n"
               "  pending_mode=''\n"
               "  pending_fixed_remaining=0\n\n"
               "  positional_phase=0\n\n"
               "  __ap_completion_load_parser() {\n"
               "    case \"$1\" in\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (append_load_parser_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb, "      *) return 1 ;;\n"
                         "    esac\n"
                         "  }\n\n"
                         "  __ap_completion_choice_words() {\n"
                         "    case \"$1\" in\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (append_choice_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb, "      *) return 1 ;;\n"
                         "    esac\n"
                         "  }\n\n"
                         "  __ap_completion_option_value_mode() {\n"
                         "    case \"$1\" in\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (append_value_mode_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb, "      *) return 1 ;;\n"
                         "    esac\n"
                         "  }\n\n"
                         "  __ap_completion_option_completion_kind() {\n"
                         "    case \"$1\" in\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (append_completion_kind_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb, "      *) return 1 ;;\n"
                         "    esac\n"
                         "  }\n\n"
                         "  __ap_completion_option_value_count() {\n"
                         "    case \"$1\" in\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }
  if (append_value_count_cases(&sb, parser) != 0 ||
      ap_sb_appendf(&sb, "      *) return 1 ;;\n"
                         "    esac\n"
                         "  }\n\n"
                         "  __ap_completion_has_word() {\n"
                         "    case \" $2 \" in\n"
                         "      *\" $1 \"*) return 0 ;;\n"
                         "      *) return 1 ;;\n"
                         "    esac\n"
                         "  }\n\n"
                         "  __ap_completion_filter_mutex_candidates() {\n"
                         "    local parser_name=$1\n"
                         "    shift\n"
                         "    printf '%%s\\n' \"$@\"\n"
                         "  }\n\n"
                         "  __ap_completion_dynamic_query() {\n"
                         "    ") != 0 ||
      append_single_quoted(&sb, prog) != 0 || ap_sb_appendf(&sb, " ") != 0 ||
      append_single_quoted(&sb, ap_parser_completion_entrypoint(parser)) != 0 ||
      ap_sb_appendf(
          &sb,
          " --shell bash -- \"${COMP_WORDS[@]:1}\"\n"
          "  }\n\n"
          "  __ap_completion_load_parser \"$parser_key\" || return 0\n"
          "  local i=1\n"
          "  while (( i < COMP_CWORD )); do\n"
          "    token=\"${COMP_WORDS[i]}\"\n"
          "    if [[ -n \"$pending_option\" ]]; then\n"
          "      case \"$pending_mode\" in\n"
          "        single)\n"
          "          pending_option=''\n"
          "          pending_mode=''\n"
          "          ;;&\n"
          "        fixed)\n"
          "          pending_fixed_remaining=$((pending_fixed_remaining - 1))\n"
          "          if (( pending_fixed_remaining <= 0 )); then\n"
          "            pending_option=''\n"
          "            pending_mode=''\n"
          "            pending_fixed_remaining=0\n"
          "          fi\n"
          "          ;;&\n"
          "        optional|multi)\n"
          "          if [[ \"$token\" != -* ]]; then\n"
          "            if [[ \"$pending_mode\" == optional ]]; then\n"
          "              pending_option=''\n"
          "              pending_mode=''\n"
          "            fi\n"
          "          else\n"
          "            pending_option=''\n"
          "            pending_mode=''\n"
          "          fi\n"
          "          ;;\n"
          "      esac\n"
          "    fi\n"
          "    if [[ \"$token\" == --*=* ]]; then\n"
          "      option_name=${token%%=*}\n"
          "      if __ap_completion_has_word \"$option_name\" "
          "\"$parser_value_options\"; then\n"
          "        :\n"
          "      fi\n"
          "    elif [[ \"$token\" == -* ]]; then\n"
          "      if __ap_completion_has_word \"$token\" "
          "\"$parser_value_options\"; then\n"
          "        pending_option=$token\n"
          "        pending_mode=$(__ap_completion_option_value_mode "
          "\"$parser_key:$token\")\n"
          "        "
          "pending_fixed_remaining=$(__ap_completion_option_value_count "
          "\"$parser_key:$token\")\n"
          "      fi\n"
          "    elif __ap_completion_has_word \"$token\" "
          "\"$parser_subcommands\"; then\n"
          "      if [[ \"$parser_key\" == root ]]; then\n"
          "        parser_key=\"root/$token\"\n"
          "      else\n"
          "        parser_key=\"$parser_key/$token\"\n"
          "      fi\n"
          "      positional_phase=0\n"
          "      pending_option=''\n"
          "      pending_mode=''\n"
          "      pending_fixed_remaining=0\n"
          "      __ap_completion_load_parser \"$parser_key\" || return 0\n"
          "    elif [[ \"$token\" != -* ]]; then\n"
          "      positional_phase=1\n"
          "    fi\n"
          "    i=$((i + 1))\n"
          "  done\n\n") != 0 ||
      ap_sb_appendf(
          &sb,
          "  if [[ \"$cur\" == --*=* ]]; then\n"
          "    option_name=${cur%%=*}\n"
          "    value_prefix=${cur#*=}\n"
          "    completion_kind=$(__ap_completion_option_completion_kind "
          "\"$parser_key:$option_name\" 2>/dev/null || printf '%%s' none)\n"
          "    case \"$completion_kind\" in\n"
          "      choices)\n"
          "        if choices=$(__ap_completion_choice_words "
          "\"$parser_key:$option_name\" 2>/dev/null); then\n"
          "          COMPREPLY=( $(compgen -W \"$choices\" -- "
          "\"$value_prefix\") )\n"
          "        fi\n"
          "        ;;\n"
          "      dynamic)\n"
          "        mapfile -t COMPREPLY < <(__ap_completion_dynamic_query)\n"
          "        ;;\n"
          "      file)\n"
          "        COMPREPLY=( $(compgen -f -- \"$value_prefix\") )\n"
          "        ;;\n"
          "      directory)\n"
          "        COMPREPLY=( $(compgen -d -- \"$value_prefix\") )\n"
          "        ;;\n"
          "      command)\n"
          "        COMPREPLY=( $(compgen -c -- \"$value_prefix\") )\n"
          "        ;;\n"
          "    esac\n"
          "    if (( ${#COMPREPLY[@]} > 0 )); then\n"
          "      local idx\n"
          "      for idx in \"${!COMPREPLY[@]}\"; do\n"
          "        COMPREPLY[idx]=\"$option_name=${COMPREPLY[idx]}\"\n"
          "      done\n"
          "      return 0\n"
          "    fi\n"
          "  fi\n\n"
          "  if [[ -n \"$pending_option\" ]]; then\n"
          "    completion_kind=$(__ap_completion_option_completion_kind "
          "\"$parser_key:$pending_option\" 2>/dev/null || printf '%%s' none)\n"
          "    case \"$completion_kind\" in\n"
          "      choices)\n"
          "        if choices=$(__ap_completion_choice_words "
          "\"$parser_key:$pending_option\" 2>/dev/null); then\n"
          "          COMPREPLY=( $(compgen -W \"$choices\" -- \"$cur\") )\n"
          "        fi\n"
          "        ;;\n"
          "      dynamic)\n"
          "        mapfile -t COMPREPLY < <(__ap_completion_dynamic_query)\n"
          "        ;;\n"
          "      file)\n"
          "        COMPREPLY=( $(compgen -f -- \"$cur\") )\n"
          "        ;;\n"
          "      directory)\n"
          "        COMPREPLY=( $(compgen -d -- \"$cur\") )\n"
          "        ;;\n"
          "      command)\n"
          "        COMPREPLY=( $(compgen -c -- \"$cur\") )\n"
          "        ;;\n"
          "    esac\n"
          "    if (( ${#COMPREPLY[@]} > 0 )); then\n"
          "      return 0\n"
          "    fi\n"
          "  fi\n\n") != 0 ||
      ap_sb_appendf(
          &sb,
          "  if [[ \"$cur\" == -* && $positional_phase -eq 0 ]]; then\n"
          "    filtered=()\n"
          "    while IFS= read -r token; do\n"
          "      [[ -n \"$token\" ]] && filtered+=(\"$token\")\n"
          "    done < <(__ap_completion_filter_mutex_candidates "
          "\"$parser_key\" $parser_options)\n"
          "    COMPREPLY=( $(compgen -W \"${filtered[*]}\" -- \"$cur\") )\n"
          "    return 0\n"
          "  fi\n\n"
          "  mapfile -t COMPREPLY < <(__ap_completion_dynamic_query)\n"
          "  if (( ${#COMPREPLY[@]} > 0 )); then\n"
          "    return 0\n"
          "  fi\n\n"
          "  if (( positional_phase == 0 )); then\n"
          "    candidates=()\n"
          "    while IFS= read -r token; do\n"
          "      [[ -n \"$token\" ]] && candidates+=(\"$token\")\n"
          "    done < <(printf '%%s\\n' $parser_subcommands)\n"
          "    COMPREPLY=( $(compgen -W \"${candidates[*]}\" -- \"$cur\") )\n"
          "  fi\n"
          "  return 0\n"
          "}\n\n"
          "complete -F _") != 0 ||
      append_identifier(&sb, prog) != 0 || ap_sb_appendf(&sb, " ") != 0 ||
      append_single_quoted(&sb, prog) != 0 || ap_sb_appendf(&sb, "\n") != 0) {
    ap_sb_free(&sb);
    return NULL;
  }

  return sb.data;
}
