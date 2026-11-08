#pragma once

// A tiny test runner: TEST() registers a case, CHECK() records a failure.
// Each test file builds into its own executable that CTest runs.

#include <cstdio>
#include <vector>

namespace check {

struct Case {
  const char *name;
  void (*fn)();
};

inline std::vector<Case> &cases() {
  static std::vector<Case> all;
  return all;
}

inline int &failures() {
  static int count = 0;
  return count;
}

struct Register {
  Register(const char *name, void (*fn)()) { cases().push_back({name, fn}); }
};

}

#define TEST(name)                                          \
  static void name();                                       \
  static check::Register register_##name(#name, name);     \
  static void name()

#define CHECK(cond)                                                          \
  do {                                                                       \
    if(!(cond)) {                                                            \
      std::printf("%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);   \
      check::failures()++;                                                   \
    }                                                                        \
  } while(0)

#define CHECK_EQ(a, b)                                                       \
  do {                                                                       \
    auto check_a_ = (a);                                                     \
    auto check_b_ = (b);                                                     \
    if(!(check_a_ == check_b_)) {                                            \
      std::printf("%s:%d: CHECK_EQ(%s, %s) failed: %lld != %lld\n", __FILE__, \
                  __LINE__, #a, #b, (long long)check_a_, (long long)check_b_); \
      check::failures()++;                                                   \
    }                                                                        \
  } while(0)
