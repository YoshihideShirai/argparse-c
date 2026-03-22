#include "test_framework.h"

int main() {
  int failures = 0;

  for (const TestCase &test : registry()) {
    try {
      test.run();
      std::fprintf(stdout, "[PASS] %s\n", test.name);
    } catch (const std::exception &ex) {
      std::fprintf(stderr, "[FAIL] %s: %s\n", test.name, ex.what());
      ++failures;
    } catch (...) {
      std::fprintf(stderr, "[FAIL] %s: unknown exception\n", test.name);
      ++failures;
    }
  }

  if (failures == 0) {
    std::fprintf(stdout, "OK (%zu tests)\n", registry().size());
    return 0;
  }

  std::fprintf(stderr, "%d test(s) failed\n", failures);
  return 1;
}
