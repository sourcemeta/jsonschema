#ifndef SOURCEMETA_CORE_PROCESS_H_
#define SOURCEMETA_CORE_PROCESS_H_

#ifndef SOURCEMETA_CORE_PROCESS_EXPORT
#include <sourcemeta/core/process_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/process_error.h>
#include <sourcemeta/core/process_usage.h>
// NOLINTEND(misc-include-cleaner)

#include <filesystem>       // std::filesystem
#include <initializer_list> // std::initializer_list
#include <map>              // std::map
#include <optional>         // std::optional
#include <span>             // std::span
#include <string>           // std::string
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
/// The settings for running a program.
///
/// Every member carries a default so that naming only the ones that matter
/// stays free of missing-initializer warnings.
struct ProcessInput {
  /// The working directory of the program. It must be an absolute path to an
  /// existing directory
  std::filesystem::path directory{std::filesystem::current_path()};
  /// The entire environment of the program, replacing rather than extending the
  /// environment of the caller. Without a value, the caller's environment is
  /// inherited as it stands. The referenced names and values must outlive the
  /// call
  std::optional<std::map<std::string_view, std::string_view>> environment{
      std::nullopt};
  /// The bytes to feed the program on its standard input. The referenced buffer
  /// must outlive the call
  std::string_view standard_input{};
};

/// @ingroup process
///
/// Spawn a program piping its output to the current stdio configuration.
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
           const ProcessInput &input = {}) -> int;

/// @ingroup process
///
/// Spawn a program piping its output to the current stdio configuration.
/// This overload accepts a span for dynamic argument lists.
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
auto spawn(const std::string &program,
           std::span<const std::string_view> arguments,
           const ProcessInput &input = {}) -> int;

/// @ingroup process
/// The result of running a program while capturing what it writes.
struct ProcessOutput {
  /// The code the program exited with, or no value if it terminated abnormally,
  /// such as by a signal
  std::optional<int> exit_code{std::nullopt};
  /// Everything the program wrote to its standard output
  std::string standard_output{};
  /// Everything the program wrote to its standard error
  std::string standard_error{};
};

/// @ingroup process
///
/// Spawn a program, feeding it the given input and capturing both of its output
/// streams in full.
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::spawn_and_capture("echo", {"foo"})};
/// assert(result.exit_code.value() == 0);
/// assert(result.standard_output == "foo\n");
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto spawn_and_capture(const std::string &program,
                       std::initializer_list<std::string_view> arguments,
                       const ProcessInput &input = {}) -> ProcessOutput;

/// @ingroup process
///
/// Spawn a program, feeding it the given input and capturing both of its output
/// streams in full. This overload accepts a span for dynamic argument lists.
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <vector>
/// #include <string_view>
/// #include <cassert>
///
/// std::vector<std::string_view> arguments{"foo", "bar"};
/// const auto result{sourcemeta::core::spawn_and_capture("echo", arguments)};
/// assert(result.exit_code.value() == 0);
/// assert(result.standard_output == "foo bar\n");
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto spawn_and_capture(const std::string &program,
                       std::span<const std::string_view> arguments,
                       const ProcessInput &input = {}) -> ProcessOutput;

} // namespace sourcemeta::core

#endif
