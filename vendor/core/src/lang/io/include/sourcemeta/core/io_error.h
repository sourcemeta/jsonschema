#ifndef SOURCEMETA_CORE_IO_ERROR_H_
#define SOURCEMETA_CORE_IO_ERROR_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <exception>   // std::exception
#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup io
/// An error that represents a failure to memory-map a file
class SOURCEMETA_CORE_IO_EXPORT FileViewError : public std::exception {
public:
  FileViewError(std::filesystem::path path, const char *message)
      : path_{std::move(path)}, message_{message} {}
  FileViewError(std::filesystem::path path, std::string message) = delete;
  FileViewError(std::filesystem::path path, std::string &&message) = delete;
  FileViewError(std::filesystem::path path, std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
  const char *message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
