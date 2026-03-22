#ifndef MINICPPUTEST_TESTHARNESS_H
#define MINICPPUTEST_TESTHARNESS_H

#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace MiniCppUTest {

struct TestCase {
  const char *group_name;
  const char *test_name;
  std::function<void()> run;
};

inline std::vector<TestCase> &registry() {
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar {
  Registrar(const char *group_name, const char *test_name,
            std::function<void()> run) {
    registry().push_back({group_name, test_name, std::move(run)});
  }
};

class AssertFailure : public std::runtime_error {
public:
  explicit AssertFailure(const std::string &message)
      : std::runtime_error(message) {}
};

inline void fail(const char *expr, const char *file, int line,
                 const std::string &detail = std::string()) {
  std::ostringstream oss;
  oss << file << ":" << line << ": assertion failed: " << expr;
  if (!detail.empty()) {
    oss << " (" << detail << ")";
  }
  throw AssertFailure(oss.str());
}

} // namespace MiniCppUTest

#define TEST_GROUP(group_name) struct group_name

#define TEST(group_name, test_name)                                             \
  static void group_name##_##test_name##_impl(group_name &);                    \
  static MiniCppUTest::Registrar group_name##_##test_name##_registrar(          \
      #group_name, #test_name, []() {                                           \
        group_name group_instance;                                              \
        group_name##_##test_name##_impl(group_instance);                        \
        group_instance.teardown();                                              \
      });                                                                       \
  static void group_name##_##test_name##_impl(group_name &)

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      MiniCppUTest::fail(#condition, __FILE__, __LINE__);                       \
    }                                                                           \
  } while (0)

#define CHECK_TRUE(condition) CHECK(condition)

#define LONGS_EQUAL(expected, actual)                                           \
  do {                                                                          \
    long long expected_value__ = static_cast<long long>(expected);              \
    long long actual_value__ = static_cast<long long>(actual);                  \
    if (expected_value__ != actual_value__) {                                   \
      std::ostringstream detail__;                                              \
      detail__ << "expected " << expected_value__ << ", actual "                \
               << actual_value__;                                               \
      MiniCppUTest::fail(#actual, __FILE__, __LINE__, detail__.str());          \
    }                                                                           \
  } while (0)

#define STRCMP_EQUAL(expected, actual)                                          \
  do {                                                                          \
    const char *expected_value__ = (expected);                                  \
    const char *actual_value__ = (actual);                                      \
    if (((expected_value__ == nullptr) != (actual_value__ == nullptr)) ||       \
        (expected_value__ != nullptr &&                                          \
         std::strcmp(expected_value__, actual_value__) != 0)) {                 \
      std::ostringstream detail__;                                              \
      detail__ << "expected \""                                                \
               << (expected_value__ ? expected_value__ : "(null)")              \
               << "\", actual \""                                             \
               << (actual_value__ ? actual_value__ : "(null)") << "\"";       \
      MiniCppUTest::fail(#actual, __FILE__, __LINE__, detail__.str());          \
    }                                                                           \
  } while (0)

#endif
