#ifndef MINICPPUTEST_COMMANDLINETESTRUNNER_H
#define MINICPPUTEST_COMMANDLINETESTRUNNER_H

#include <cstdio>

#include "CppUTest/TestHarness.h"

class CommandLineTestRunner {
public:
  static int RunAllTests(int, char **) {
    int failures = 0;
    for (const auto &test : MiniCppUTest::registry()) {
      try {
        test.run();
      } catch (const MiniCppUTest::AssertFailure &failure) {
        std::fprintf(stderr, "[FAIL] %s.%s: %s\n", test.group_name,
                     test.test_name, failure.what());
        failures++;
      } catch (const std::exception &ex) {
        std::fprintf(stderr, "[FAIL] %s.%s: unexpected exception: %s\n",
                     test.group_name, test.test_name, ex.what());
        failures++;
      } catch (...) {
        std::fprintf(stderr, "[FAIL] %s.%s: unknown exception\n",
                     test.group_name, test.test_name);
        failures++;
      }
    }

    if (failures == 0) {
      std::fprintf(stdout, "OK (%zu tests)\n", MiniCppUTest::registry().size());
      return 0;
    }

    std::fprintf(stderr, "%d test(s) failed\n", failures);
    return 1;
  }
};

#endif
