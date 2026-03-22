#include "test_framework.h"

std::vector<TestCase> &registry() {
  static std::vector<TestCase> tests;
  return tests;
}

Registrar::Registrar(const char *name, void (*run)()) { registry().push_back({name, run}); }

[[noreturn]] void fail(const char *expr, const char *file, int line,
                      const std::string &detail) {
  std::ostringstream oss;
  oss << file << ':' << line << ": assertion failed: " << expr;
  if (!detail.empty()) {
    oss << " (" << detail << ')';
  }
  throw std::runtime_error(oss.str());
}

ap_parser *new_base_parser(void) {
  ap_error err = {};
  ap_parser *p = ap_parser_new("prog", "desc");

  if (!p) {
    return NULL;
  }

  ap_arg_options text_opts = ap_arg_options_default();
  text_opts.required = true;
  text_opts.help = "text";
  if (ap_add_argument(p, "-t, --text", text_opts, &err) != 0) {
    ap_parser_free(p);
    return NULL;
  }

  ap_arg_options int_opts = ap_arg_options_default();
  int_opts.type = AP_TYPE_INT32;
  if (ap_add_argument(p, "-n, --num", int_opts, &err) != 0) {
    ap_parser_free(p);
    return NULL;
  }

  ap_arg_options pos_opts = ap_arg_options_default();
  pos_opts.help = "input";
  if (ap_add_argument(p, "input", pos_opts, &err) != 0) {
    ap_parser_free(p);
    return NULL;
  }

  return p;
}
