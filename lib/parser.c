#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "argparse-int.h"

static bool argv_is_option_key(const char *argv) { return *argv == '-'; }

static argparse_arg_t *argv_search_args(argparse_t *ap, const char *argv) {
  struct argparse_arg_list_t *p = ap->arg_list;
  while (p != NULL) {
    struct argparse_key_list_t *pk = p->data->key_list;
    while (pk != NULL) {
        if (!strcmp(pk->key,argv)) {
            return p->data;
        }
        pk = pk->next;
    }
    p = p->next;
  }
  return NULL;
}

static argparse_arg_t *argv_pick_unset_positional(argparse_t *ap) {
  struct argparse_arg_list_t *p = ap->arg_list;
  while (p != NULL) {
    struct argparse_key_list_t *pk = p->data->key_list;
    if (!argv_is_option_key(pk->key) && p->data->parsed.val == NULL) {
      return p->data;
    }
    p = p->next;
  }
  return NULL;
}


static void parse_error(argparse_t *ap, int argc, char *argv[],char *format, ...) {
  argparse_usage(ap, argc, argv);
  va_list va_l;
  va_start(va_l, format);
  vfprintf(stderr, format, va_l);
  va_end(va_l);
  exit(-1);
}

static int arg_has_invalid_option(argparse_t *ap, int argc, char *argv[]) {
  char **p = NULL;
  if (argc == 1){
      parse_error(ap,argc,argv,"error: no arg\n");
      return 0;
  }
  for (p = argv + 1; *p != NULL; p++) {
    if (argv_is_option_key(*p)) {
      argparse_arg_t *arg = argv_search_args(ap, *p);
      if (arg) {
        if (arg->parsed.key) {
          parse_error(ap, argc, argv, "error: %s has duplicated.\n", *p);
        }
        arg->parsed.key = *p;
        if (arg->type != ARGPARSE_ARGTYPE_NONE) {
          char *next = *(p + 1);
          if (next == NULL || argv_is_option_key(next)) {
            parse_error(ap, argc, argv, "error: %s needs to have val.\n", *p);
          } else {
            arg->parsed.val = *(p + 1);
            p++;
          }
        }
      } else {
        parse_error(ap, argc, argv, "error: invalid option. (%s).\n", *p);
      }
    } else {
      break;
    }
  }
  for (; *p != NULL; p++) {
    argparse_arg_t *arg = argv_pick_unset_positional(ap);
    if (arg) {
      arg->parsed.val = *p;
    } else {
      break;
    }
  }
  return 0;
}

static int arg_is_lacked_required(argparse_t *ap, int argc, char *argv[]) {
  struct argparse_arg_list_t *p = ap->arg_list;
  while (p != NULL) {
    if (p->data->required && !(p->data->parsed.key || p->data->parsed.val)) {
      parse_error(ap, argc, argv, "error: missed required option. (%s).\n",
                  p->data->key_list->key);
    }
    p = p->next;
  }
  return 0;
}

static int arg_validate_vals_int32(argparse_t *ap, int argc, char *argv[],
                                   argparse_arg_list_t *p_arg) {
  char *end = NULL;
  *((int32_t *)p_arg->data->valptr) =
      (int32_t)strtol(p_arg->data->parsed.val, &end, 10);
  if ('\0' != *end) {
    parse_error(ap, argc, argv,
                "error: %s is required integer val. (val=%s).\n",
                p_arg->data->parsed.key, p_arg->data->parsed.val);
  }
  return 0;
}

static int arg_validate_vals(argparse_t *ap, int argc, char *argv[]) {
  struct argparse_arg_list_t *p = ap->arg_list;
  while (p != NULL) {
    if (p->data->parsed.val) {
      switch (p->data->type) {
      case ARGPARSE_ARGTYPE_INT32:
        arg_validate_vals_int32(ap,argc,argv,p);
        break;
      case ARGPARSE_ARGTYPE_CHAR:
        *((char **)p->data->valptr) = p->data->parsed.val;
      default:
        break;
      }
    }
    p = p->next;
  }
  return 0;
}

static bool arg_is_help(argparse_t *ap, int argc, char *argv[]) {
  if(argc < 2){
    return 0;
  }
  if (!strcmp(argv[1], "-h")) {
    return 1;
  }
  if (!strcmp(argv[1], "--help")) {
    return 1;
  }
  return 0;
}

int argparse_parse(argparse_t *ap, int argc, char *argv[]) {
  if (arg_is_help(ap, argc, argv)) {
    argparse_help(ap, argc, argv);
    exit(1);
  }
  arg_has_invalid_option(ap, argc, argv);
  arg_is_lacked_required(ap, argc, argv);
  arg_validate_vals(ap, argc, argv);
  return 0;
}
