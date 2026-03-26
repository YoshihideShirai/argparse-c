#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse-c.h"

static int add_matching_candidates(const ap_completion_request *request,
                                   ap_completion_result *result,
                                   void *user_data, ap_error *err) {
  const char *const *values = (const char *const *)user_data;
  const char *prefix = request && request->current_token ? request->current_token : "";
  int i = 0;

  for (i = 0; values && values[i] != NULL; ++i) {
    if (strncmp(values[i], prefix, strlen(prefix)) == 0 &&
        ap_completion_result_add(result, values[i], "dynamic", err) != 0) {
      return -1;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  static const char *const candidates[] = {
      /* CHANGE_ME_CANDIDATES: replace with your runtime candidates. */
      "build", "bench", "bundle", NULL,
  };
  ap_error err = {0};
  ap_namespace *ns = NULL;
  ap_completion_result completion = {0};
  int handled = 0;
  int i = 0;
  ap_parser *parser = ap_parser_new("template_completion", "completion callback template");

  if (!parser) {
    fprintf(stderr, "failed to initialize parser\n");
    return 1;
  }

  {
    ap_arg_options task = ap_arg_options_default();
    task.completion_kind = AP_COMPLETION_KIND_COMMAND;
    task.completion_callback = add_matching_candidates;
    task.completion_user_data = (void *)candidates;
    task.help = "task name";

    if (ap_add_argument(parser, "--task", task, &err) != 0) {
      fprintf(stderr, "%s\n", err.message);
      ap_parser_free(parser);
      return 1;
    }
  }

  if (ap_try_handle_completion(parser, argc, argv, "bash", &handled, &completion,
                               &err) != 0) {
    fprintf(stderr, "%s\n", err.message);
    ap_completion_result_free(&completion);
    ap_parser_free(parser);
    return 1;
  }
  if (handled) {
    for (i = 0; i < completion.count; ++i) {
      printf("%s\n", completion.items[i].value);
    }
    ap_completion_result_free(&completion);
    ap_parser_free(parser);
    return 0;
  }

  if (ap_parse_args(parser, argc, argv, &ns, &err) != 0) {
    char *formatted = ap_format_error(parser, &err);
    if (formatted) {
      fprintf(stderr, "%s", formatted);
      free(formatted);
    }
    ap_parser_free(parser);
    return 1;
  }

  ap_namespace_free(ns);
  ap_parser_free(parser);
  return 0;
}
