#ifndef SOURCEMETA_CORE_URI_ERROR_H_
#define SOURCEMETA_CORE_URI_ERROR_H_

#ifndef SOURCEMETA_CORE_URI_EXPORT
#include <sourcemeta/core/uri_export.h>
#endif

#include <cstdint>   // std::uint64_t
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

/// @ingroup uri
/// An error that represents a URI parsing failure
class SOURCEMETA_CORE_URI_EXPORT URIParseError : public std::exception {
public:
  URIParseError(const std::uint64_t column) : column_{column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid URI";
  }

  /// Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return column_;
  }

private:
  std::uint64_t column_;
};

/// @ingroup uri
/// An error that represents a general URI error event
class SOURCEMETA_CORE_URI_EXPORT URIError : public std::exception {
public:
  URIError(std::string message) : message_{std::move(message)} {}
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
