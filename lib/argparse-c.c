#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "argparse-int.h"

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
