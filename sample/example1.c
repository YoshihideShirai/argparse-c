#include "argparse-c.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int example1_main(int argc, char *argv[]) {
  struct {
    char *text;
    int32_t integer;
    char *arg1;
    char *arg2;
  } args = { NULL };
  argparse_t *argparse = argparse_init("example1 command.");

  argparse_arg_t *text_option = argparse_arg_new_char(&args.text);
  argparse_arg_set_required(text_option, true);
  argparse_arg_set_key(text_option, "-t");
  argparse_arg_set_key(text_option, "--text");
  argparse_arg_set_help(text_option, "text field.");
  argparse_add_arg(argparse, text_option);

  argparse_arg_t *integer_option = argparse_arg_new_int32(&args.integer);
  argparse_arg_set_key(integer_option, "-i");
  argparse_arg_set_key(integer_option, "--integer");
  argparse_arg_set_help(integer_option, "integer field.");
  argparse_add_arg(argparse, integer_option);

  argparse_arg_t *arg1 = argparse_arg_new_char(&args.arg1);
  argparse_arg_set_required(arg1, true);
  argparse_arg_set_key(arg1, "arg1");
  argparse_arg_set_help(arg1, "arg1 field.");
  argparse_add_arg(argparse, arg1);

  argparse_arg_t *arg2 = argparse_arg_new_char(&args.arg2);
  argparse_arg_set_key(arg2, "arg2");
  argparse_arg_set_help(arg2, "arg2 field.");
  argparse_add_arg(argparse, arg2);

  argparse_exec(argparse, argc, argv);

  argparse_destroy(argparse);

  printf("args.text = %s\n",args.text);
  printf("args.integer = %d\n",args.integer);
  printf("args.arg1 = %s\n",args.arg1);
  printf("args.arg2 = %s\n",args.arg2);

  return 0;
}

int main(int argc, char *argv[]) { return example1_main(argc, argv); }
