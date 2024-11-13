#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_UNREACHABLE_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_UNREACHABLE_H_

#include <cassert> // assert

// Until we are on C++23 and can use std::unreachable
// See https://en.cppreference.com/w/cpp/utility/unreachable
[[noreturn]] inline void unreachable() {
  assert(false);
#if defined(_MSC_VER) && !defined(__clang__)
  __assume(false);
#else
  __builtin_unreachable();
#endif
}

#endif
