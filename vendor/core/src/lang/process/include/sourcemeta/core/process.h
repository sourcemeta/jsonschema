#ifndef SOURCEMETA_CORE_PROCESS_H_
#define SOURCEMETA_CORE_PROCESS_H_

#ifndef SOURCEMETA_CORE_PROCESS_EXPORT
#include <sourcemeta/core/process_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/process_error.h>
// NOLINTEND(misc-include-cleaner)

#include <filesystem>       // std::filesystem
#include <initializer_list> // std::initializer_list
#include <span>             // std::span
#include <string_view>      // std::string_view

/// @defgroup process Process
/// @brief Process related utilities
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// ```

namespace sourcemeta::core {

/// @ingroup process
///
/// Spawn a program piping its output to the current stdio configuration.
/// The directory parameter specifies the working directory for the spawned
/// process. It must be an absolute path to an existing directory.
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <cassert>
///
/// const auto exit_code{sourcemeta::core::spawn("echo", {"foo"})};
/// assert(exit_code == 0);
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto spawn(const std::string &program,
           std::initializer_list<std::string_view> arguments,
           const std::filesystem::path &directory =
               std::filesystem::current_path()) -> int;

/// @ingroup process
///
/// Spawn a program piping its output to the current stdio configuration.
/// This overload accepts a span for dynamic argument lists.
/// The directory parameter specifies the working directory for the spawned
/// process. It must be an absolute path to an existing directory.
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <vector>
/// #include <string_view>
/// #include <cassert>
///
/// std::vector<std::string_view> arguments{"foo", "bar"};
/// const auto exit_code{sourcemeta::core::spawn("echo", arguments)};
/// assert(exit_code == 0);
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto spawn(
    const std::string &program, std::span<const std::string_view> arguments,
    const std::filesystem::path &directory = std::filesystem::current_path())
    -> int;

} // namespace sourcemeta::core

#endif
