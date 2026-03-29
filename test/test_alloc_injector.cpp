#include "test_framework.h"

#include <dlfcn.h>

#include <atomic>
#include <cstdlib>
#include <cstring>

namespace {

std::atomic<int> g_fail_on_nth{0};
std::atomic<int> g_attempts{0};
thread_local int g_hook_depth = 0;

using malloc_fn = void *(*)(size_t);
using realloc_fn = void *(*)(void *, size_t);

malloc_fn resolve_real_malloc() {
  static malloc_fn fn = nullptr;
  if (!fn) {
    fn = reinterpret_cast<malloc_fn>(dlsym(RTLD_NEXT, "malloc"));
  }
  return fn;
}

realloc_fn resolve_real_realloc() {
  static realloc_fn fn = nullptr;
  if (!fn) {
    fn = reinterpret_cast<realloc_fn>(dlsym(RTLD_NEXT, "realloc"));
  }
  return fn;
}

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

extern "C" void *malloc(size_t size) {
  malloc_fn real_malloc;
  void *ptr;

  if (g_hook_depth > 0) {
    real_malloc = resolve_real_malloc();
    return real_malloc ? real_malloc(size) : nullptr;
  }
  g_hook_depth++;
  if (should_fail_this_attempt()) {
    g_hook_depth--;
    return nullptr;
  }
  real_malloc = resolve_real_malloc();
  ptr = real_malloc ? real_malloc(size) : nullptr;
  g_hook_depth--;
  return ptr;
}

extern "C" void *realloc(void *ptr, size_t size) {
  realloc_fn real_realloc;
  void *next;

  if (g_hook_depth > 0) {
    real_realloc = resolve_real_realloc();
    return real_realloc ? real_realloc(ptr, size) : nullptr;
  }
  g_hook_depth++;
  if (should_fail_this_attempt()) {
    g_hook_depth--;
    return nullptr;
  }
  real_realloc = resolve_real_realloc();
  next = real_realloc ? real_realloc(ptr, size) : nullptr;
  g_hook_depth--;
  return next;
}

extern "C" char *strdup(const char *s) {
  malloc_fn real_malloc;
  size_t len;
  char *copy;

  if (g_hook_depth > 0) {
    real_malloc = resolve_real_malloc();
    if (!real_malloc) {
      return nullptr;
    }
    len = std::strlen(s);
    copy = static_cast<char *>(real_malloc(len + 1));
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
  real_malloc = resolve_real_malloc();
  if (!real_malloc) {
    g_hook_depth--;
    return nullptr;
  }
  len = std::strlen(s);
  copy = static_cast<char *>(real_malloc(len + 1));
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
