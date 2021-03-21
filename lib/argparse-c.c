#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "argparse-c.h"

argparse_t *argparse_init(const char *desc) {
  argparse_t *argparse = calloc(1, sizeof(argparse_t));
  argparse->desc = desc;

  argparse_arg_t *help_option = argparse_arg_new_noval(&argparse->is_help_set);
  argparse_arg_set_key(help_option, "-h");
  argparse_arg_set_key(help_option, "--help");
  argparse_arg_set_help(help_option, "show this help message and exit");
  argparse_add_arg(argparse, help_option);

  return argparse;
}

void argparse_add_arg(argparse_t *ap, struct argparse_arg_t *apa) {
  argparse_arg_list_t *data = calloc(1, sizeof(argparse_arg_list_t));
  struct argparse_arg_list_t *p = ap->arg_list;
  data->data = apa;
  if(!p){
    ap->arg_list = data;
    return;
  }
  while (p->next != NULL) {
    p = p->next;
  }
  p->next = data;
}

static argparse_key_list_t *pick_metaval_key(argparse_arg_t *arg) {
  struct argparse_key_list_t *p = arg->key_list;

  while (p != NULL) {
    if (!strncmp(p->key, "--", 2)) {
      return p;
    }
    p = p->next;
  }
  return arg->key_list;
}

static char *key2metaval(argparse_arg_t *arg) {
  struct argparse_key_list_t *pk = pick_metaval_key(arg);
  char *v = strdup(pk->key);
  int i, j;
  while (v[0] == '-') {
    for (j = 0; v[j]; j++) {
      v[j] = v[j + 1];
    }
  }
  for (i = 0; v[i]; i++) {
    v[i] = toupper((unsigned char)v[i]);
  }
  return v;
}

static bool arg_is_option(argparse_arg_t *arg) {
  struct argparse_key_list_t *pk = arg->key_list;
  return pk->key[0] == '-';
}

static bool arg_has_val(argparse_arg_t *arg) {
  return arg->type != ARGPARSE_ARGTYPE_NONE;
}

static void argparse_print_arg_metaval(argparse_arg_t *arg,
                                       argparse_key_list_t *pk) {
  char *metaval = NULL;
  metaval = key2metaval(arg);
  if (arg_is_option(arg) && arg_has_val(arg)) {
    printf("%s %s", pk->key, metaval);
  } else {
    printf("%s", pk->key);
  }
  free(metaval);
}

static void argparse_print_usage_arg(argparse_arg_t *arg) {
  argparse_key_list_t *pk = arg->key_list;
  char *metaval = NULL;
  if (!arg->print_usage) {
    return;
  }
  metaval = key2metaval(arg);
  printf(" ");
  if (!arg->required) {
    printf("[");
  }
  argparse_print_arg_metaval(arg, pk);
  if (!arg->required) {
    printf("]");
  }
  free(metaval);
}

static void argparse_usage(argparse_t *ap, int argc, char *argv[]) {
  struct argparse_arg_list_t *p = ap->arg_list;
  printf("usage: %s", argv[0]);
  while (p != NULL) {
    argparse_print_usage_arg(p->data);
    p = p->next;
  }
  printf("\n");
}

static void argparse_help_positional(argparse_t *ap, int argc, char *argv[]) {
  struct argparse_arg_list_t *p = ap->arg_list;
  printf("positional arguments:\n");
  while (p != NULL) {
    if (!arg_is_option(p->data)) {
      printf("  ");
      struct argparse_key_list_t *pk = p->data->key_list;
      while (pk != NULL) {
        if (p->data->key_list != pk) {
          printf(", ");
        }
        argparse_print_arg_metaval(p->data, pk);
        pk = pk->next;
      }
      printf("\n    %s\n", p->data->help);
    }
    p = p->next;
  }
}

static void argparse_help_optional(argparse_t *ap, int argc, char *argv[]) {
  struct argparse_arg_list_t *p = ap->arg_list;
  printf("optional arguments:\n");
  while (p != NULL) {
    if (arg_is_option(p->data)) {
      printf("  ");
      struct argparse_key_list_t *pk = p->data->key_list;
      while (pk != NULL) {
        if (p->data->key_list != pk) {
          printf(", ");
        }
        argparse_print_arg_metaval(p->data, pk);
        pk = pk->next;
      }
      printf("\n    %s\n", p->data->help);
    }
    p = p->next;
  }
}

static void argparse_help(argparse_t *ap, int argc, char *argv[]) {
  argparse_usage(ap, argc, argv);
  printf("\n");
  printf("%s\n",ap->desc);
  printf("\n");
  argparse_help_positional(ap, argc, argv);
  printf("\n");
  argparse_help_optional(ap, argc, argv);
  printf("\n");
}

void argparse_exec(argparse_t *ap, int argc, char *argv[]) {
  argparse_help(ap, argc, argv);
}

void argparse_destroy(argparse_t *ap) {}

static argparse_arg_t *argparse_arg_new(void) {
  argparse_arg_t *arg = calloc(1, sizeof(argparse_arg_t));
  arg->print_help = true;
  arg->print_usage = true;
  return arg;
}

argparse_arg_t *argparse_arg_new_noval(bool *is_set) {
  argparse_arg_t *arg = argparse_arg_new();
  arg->type = ARGPARSE_ARGTYPE_NONE;
  return arg;
}

argparse_arg_t *argparse_arg_new_char(char *const *str) {
  argparse_arg_t *arg = argparse_arg_new();
  arg->type = ARGPARSE_ARGTYPE_CHAR;
  return arg;
}

argparse_arg_t *argparse_arg_new_int32(int32_t *integer) {
  argparse_arg_t *arg = argparse_arg_new();
  arg->type = ARGPARSE_ARGTYPE_INT32;
  return arg;
}

void argparse_arg_set_required(argparse_arg_t *apa, bool required) {
  apa->required = required;
}

void argparse_arg_set_key(argparse_arg_t *apa, const char *key) {
  struct argparse_key_list_t *data = calloc(1, sizeof(argparse_key_list_t));
  struct argparse_key_list_t *p = apa->key_list;
  data->key = key;
  if (!p) {
    apa->key_list = data;
    return;
  }
  while (p->next != NULL) {
    p = p->next;
  }
  p->next = data;
}

void argparse_arg_set_help(argparse_arg_t *apa, const char *help) {
  apa->help = help;
}

void argparse_arg_set_print_usage(argparse_arg_t *apa, bool print_usage) {
  apa->print_usage = print_usage;
}

void argparse_arg_set_print_help(argparse_arg_t *apa, bool print_help) {
  apa->print_help = print_help;
}
