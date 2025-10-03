#ifndef SOURCEMETA_CORE_GZIP_ERROR_H_
#define SOURCEMETA_CORE_GZIP_ERROR_H_

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

#include <exception> // std::exception
#include <string>    // std::string
#include <utility>   // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup gzip
/// An error that represents a general GZIP error event
class SOURCEMETA_CORE_GZIP_EXPORT GZIPError : public std::exception {
public:
  GZIPError(std::string message) : message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  std::string message_;
};

/// @ingroup gzip
/// An error that represents a GZIP archive error event
class SOURCEMETA_CORE_GZIP_EXPORT GZIPArchiveError : public std::exception {
public:
  GZIPArchiveError(std::string message) : message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  std::string message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
