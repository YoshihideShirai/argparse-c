#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "argparse-c.h"
}

struct TestCase {
  const char *name;
  void (*run)();
};

std::vector<TestCase> &registry();

struct Registrar {
  Registrar(const char *name, void (*run)());
};

[[noreturn]] void fail(const char *expr, const char *file, int line,
                       const std::string &detail = "");

ap_parser *new_base_parser(void);
void test_alloc_fail_disable(void);
void test_alloc_fail_on_nth(int nth_alloc);
bool test_alloc_injection_available(void);

#define TEST(name)                                                             \
  static void name();                                                          \
  static Registrar name##_registrar(#name, &name);                             \
  static void name()

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fail(#condition, __FILE__, __LINE__);                                    \
    }                                                                          \
  } while (0)

#define CHECK_TRUE(condition) CHECK(condition)

#define LONGS_EQUAL(expected, actual)                                          \
  do {                                                                         \
    auto expected_value = (expected);                                          \
    auto actual_value = (actual);                                              \
    if (expected_value != actual_value) {                                      \
      std::ostringstream detail;                                               \
      detail << "expected " << expected_value << ", got " << actual_value;     \
      fail(#actual, __FILE__, __LINE__, detail.str());                         \
    }                                                                          \
  } while (0)

#define STRCMP_EQUAL(expected, actual)                                         \
  do {                                                                         \
    const char *expected_value = (expected);                                   \
    const char *actual_value = (actual);                                       \
    if (((expected_value) == NULL) != ((actual_value) == NULL) ||              \
        ((expected_value) != NULL &&                                           \
         strcmp(expected_value, actual_value) != 0)) {                         \
      std::ostringstream detail;                                               \
      detail << "expected '" << (expected_value ? expected_value : "(null)")   \
             << "', got '" << (actual_value ? actual_value : "(null)") << "'"; \
      fail(#actual, __FILE__, __LINE__, detail.str());                         \
    }                                                                          \
  } while (0)
