#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "argparse-c.h"

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

void argparse_help(argparse_t *ap, int argc, char *argv[]) {
  argparse_usage(ap, argc, argv);
  printf("\n");
  printf("%s\n",ap->desc);
  printf("\n");
  argparse_help_positional(ap, argc, argv);
  printf("\n");
  argparse_help_optional(ap, argc, argv);
  printf("\n");
}
