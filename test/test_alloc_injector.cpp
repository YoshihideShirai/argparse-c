#include "test_framework.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define AP_TEST_ALLOC_INJECTION_DISABLED 1
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#define AP_TEST_ALLOC_INJECTION_DISABLED 1
#endif

#if !defined(AP_TEST_ALLOC_INJECTION_DISABLED)

namespace {

std::atomic<int> g_fail_on_nth{0};
std::atomic<int> g_attempts{0};
thread_local int g_hook_depth = 0;

bool should_fail_this_attempt() {
  int nth = g_fail_on_nth.load(std::memory_order_relaxed);
  int attempt;
  if (nth <= 0) {
    return false;
  }
  attempt = g_attempts.fetch_add(1, std::memory_order_relaxed) + 1;
  return attempt == nth;
}

} // namespace

extern "C" {
void *__libc_malloc(size_t size);
void *__libc_realloc(void *ptr, size_t size);
}

extern "C" void *malloc(size_t size) {
  if (g_hook_depth > 0) {
    return __libc_malloc(size);
  }
  g_hook_depth++;
  if (should_fail_this_attempt()) {
    g_hook_depth--;
    return nullptr;
  }
  void *ptr = __libc_malloc(size);
  g_hook_depth--;
  return ptr;
}

extern "C" void *realloc(void *ptr, size_t size) {
  if (g_hook_depth > 0) {
    return __libc_realloc(ptr, size);
  }
  g_hook_depth++;
  if (should_fail_this_attempt()) {
    g_hook_depth--;
    return nullptr;
  }
  void *next = __libc_realloc(ptr, size);
  g_hook_depth--;
  return next;
}

extern "C" char *strdup(const char *s) {
  size_t len;
  char *copy;

  if (g_hook_depth > 0) {
    len = std::strlen(s);
    copy = static_cast<char *>(__libc_malloc(len + 1));
    if (!copy) {
      return nullptr;
    }
    std::memcpy(copy, s, len + 1);
    return copy;
  }

  g_hook_depth++;
  if (should_fail_this_attempt()) {
    g_hook_depth--;
    return nullptr;
  }
  len = std::strlen(s);
  copy = static_cast<char *>(__libc_malloc(len + 1));
  if (copy) {
    std::memcpy(copy, s, len + 1);
  }
  g_hook_depth--;
  return copy;
}

void test_alloc_fail_disable(void) {
  g_fail_on_nth.store(0, std::memory_order_relaxed);
  g_attempts.store(0, std::memory_order_relaxed);
}

void test_alloc_fail_on_nth(int nth_alloc) {
  g_fail_on_nth.store(nth_alloc, std::memory_order_relaxed);
  g_attempts.store(0, std::memory_order_relaxed);
}

bool test_alloc_injection_available(void) { return true; }

#else

void test_alloc_fail_disable(void) {}

void test_alloc_fail_on_nth(int nth_alloc) { (void)nth_alloc; }

bool test_alloc_injection_available(void) { return false; }

#endif
