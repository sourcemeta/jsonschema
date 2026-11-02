#ifndef SOURCEMETA_CORE_PROCESS_ERROR_H_
#define SOURCEMETA_CORE_PROCESS_ERROR_H_

#ifndef SOURCEMETA_CORE_PROCESS_EXPORT
#include <sourcemeta/core/process_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <initializer_list> // std::initializer_list
#include <span>             // std::span
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <vector>           // std::vector

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup process
/// An executable program could not be found
class SOURCEMETA_CORE_PROCESS_EXPORT ProcessProgramNotFoundError
    : public Error {
public:
  /// Construct the error given the offending program.
  ProcessProgramNotFoundError(const std::string_view program)
      : Error{"Could not locate the requested program"}, program_{program} {}

  /// The offending program.
  [[nodiscard]] auto program() const noexcept -> std::string_view {
    return this->program_;
  }

private:
  std::string program_;
};

/// @ingroup process
/// A spawned process terminated abnormally
class SOURCEMETA_CORE_PROCESS_EXPORT ProcessSpawnError : public Error {
public:
  /// Construct the error given the offending program and its arguments.
  ProcessSpawnError(const std::string_view program,
                    std::initializer_list<std::string_view> arguments)
      : Error{"Process terminated abnormally"}, program_{program},
        arguments_{arguments.begin(), arguments.end()} {}

  /// Construct the error given the offending program and its arguments.
  ProcessSpawnError(const std::string_view program,
                    std::span<const std::string_view> arguments)
      : Error{"Process terminated abnormally"}, program_{program},
        arguments_{arguments.begin(), arguments.end()} {}

  /// The offending program.
  [[nodiscard]] auto program() const noexcept -> std::string_view {
    return this->program_;
  }

  /// The arguments the program was spawned with.
  [[nodiscard]] auto arguments() const noexcept
      -> const std::vector<std::string> & {
    return this->arguments_;
  }

private:
  std::string program_;
  std::vector<std::string> arguments_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
