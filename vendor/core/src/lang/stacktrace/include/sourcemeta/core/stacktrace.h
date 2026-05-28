#ifndef SOURCEMETA_CORE_STACKTRACE_H_
#define SOURCEMETA_CORE_STACKTRACE_H_

#ifndef SOURCEMETA_CORE_STACKTRACE_EXPORT
#include <sourcemeta/core/stacktrace_export.h>
#endif

/// @defgroup stacktrace Stacktrace
/// @brief A collection of utilities for interacting with stack traces.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/stacktrace.h>
/// ```

namespace sourcemeta::core {

/// @ingroup stacktrace
///
/// Install a process-wide handler that prints a stack trace on fatal signals.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/stacktrace.h>
///
/// auto main() -> int {
///   sourcemeta::core::stacktrace_on_crash();
///   // ... rest of the program
/// }
/// ```
SOURCEMETA_CORE_STACKTRACE_EXPORT
auto stacktrace_on_crash() -> void;

/// @ingroup stacktrace
///
/// Print the current backtrace. Safe to call from any thread, but not from a
/// signal handler. For example:
///
/// ```cpp
/// #include <sourcemeta/core/stacktrace.h>
///
/// sourcemeta::core::stacktrace();
/// ```
SOURCEMETA_CORE_STACKTRACE_EXPORT
auto stacktrace() -> void;

} // namespace sourcemeta::core

#endif
