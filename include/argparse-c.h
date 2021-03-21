#include <stdint.h>
#include <stdbool.h>

#include "argparse-c-def.h"

#ifndef __ARGPARSE_C_H__
#define __ARGPARSE_C_H__

extern argparse_t *argparse_init(const char *desc);
extern void argparse_add_arg(argparse_t *ap, argparse_arg_t *apa);
extern void argparse_exec(argparse_t *ap, int argc, char *argv[]);
extern void argparse_destroy(argparse_t *ap);

extern argparse_arg_t *argparse_arg_new_noval(bool *is_set);
extern argparse_arg_t *argparse_arg_new_char(char **str);
extern argparse_arg_t *argparse_arg_new_int32(int32_t *integer);

extern void argparse_arg_set_required(argparse_arg_t *apa, bool required);
extern void argparse_arg_set_key(argparse_arg_t *apa, const char *key);
extern void argparse_arg_set_help(argparse_arg_t *apa, const char *help);
extern void argparse_arg_set_print_usage(argparse_arg_t *apa, bool print_usage);
extern void argparse_arg_set_print_help(argparse_arg_t *apa, bool print_help);

#endif
