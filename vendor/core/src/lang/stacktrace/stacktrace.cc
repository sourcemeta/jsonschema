#include <sourcemeta/core/stacktrace.h>

#if defined(_WIN32)
#include "stacktrace_windows.h"
#elif defined(__unix__) || defined(__APPLE__)
#include "stacktrace_posix.h"
#else
#error "sourcemeta::core::stacktrace has no implementation for this platform"
#endif
