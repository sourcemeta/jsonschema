#ifndef SOURCEMETA_CORE_PROCESS_ERROR_H_
#define SOURCEMETA_CORE_PROCESS_ERROR_H_

#ifndef SOURCEMETA_CORE_PROCESS_EXPORT
#include <sourcemeta/core/process_export.h>
#endif

#include <exception>        // std::exception
#include <initializer_list> // std::initializer_list
#include <span>             // std::span
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <utility>          // std::move
#include <vector>           // std::vector

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup process
/// An executable program could not be found
class SOURCEMETA_CORE_PROCESS_EXPORT ProcessProgramNotNotFoundError
    : public std::exception {
public:
  ProcessProgramNotNotFoundError(std::string program)
      : program_{std::move(program)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Could not locate the requested program";
  }

  [[nodiscard]] auto program() const noexcept -> std::string_view {
    return this->program_;
  }

private:
  std::string program_;
};

/// @ingroup process
/// A spawned process terminated abnormally
class SOURCEMETA_CORE_PROCESS_EXPORT ProcessSpawnError : public std::exception {
public:
  ProcessSpawnError(std::string program,
                    std::initializer_list<std::string_view> arguments)
      : program_{std::move(program)},
        arguments_{arguments.begin(), arguments.end()} {}

  ProcessSpawnError(std::string program,
                    std::span<const std::string_view> arguments)
      : program_{std::move(program)},
        arguments_{arguments.begin(), arguments.end()} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Process terminated abnormally";
  }

  [[nodiscard]] auto program() const noexcept -> std::string_view {
    return this->program_;
  }

  [[nodiscard]] auto arguments() const noexcept
      -> const std::vector<std::string> & {
    return this->arguments_;
  }

private:
  std::string program_;
  std::vector<std::string> arguments_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
