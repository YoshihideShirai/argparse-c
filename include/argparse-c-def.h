#include <stdbool.h>

#ifndef __ARGPARSE_C_DEF_H__
#define __ARGPARSE_C_DEF_H__

typedef enum {
  ARGPARSE_ARGTYPE_NONE = 0,
  ARGPARSE_ARGTYPE_INT32,
  ARGPARSE_ARGTYPE_CHAR,
} argparse_argtype_t;

typedef struct argparse_key_list_t {
  const char *key;
  struct argparse_key_list_t *next;
} argparse_key_list_t;

typedef struct argparse_arg_t {
  argparse_argtype_t type;
  void *valptr;
  bool required;
  const char *help;
  bool print_usage;
  bool print_help;
  struct argparse_key_list_t *key_list;
  struct {
    char *key;
    char *val;
  } parsed;
} argparse_arg_t;

typedef struct argparse_arg_list_t {
  struct argparse_arg_t *data;
  struct argparse_arg_list_t *next;
} argparse_arg_list_t;

typedef struct argparse_t {
  const char *desc;
  bool is_help_set;
  struct argparse_arg_list_t *arg_list;
} argparse_t;

#endif
