#include "argparse-c.h"

#ifndef __ARGPARSE_INT_H__
#define __ARGPARSE_INT_H__

extern void argparse_help(argparse_t *ap, int argc, char *argv[]);
extern void argparse_usage(argparse_t *ap, int argc, char *argv[]);
extern int argparse_parse(argparse_t *ap, int argc, char *argv[]);

#endif
