#ifndef SOURCEMETA_CORE_JSONPATH_ERROR_H_
#define SOURCEMETA_CORE_JSONPATH_ERROR_H_

#ifndef SOURCEMETA_CORE_JSONPATH_EXPORT
#include <sourcemeta/core/jsonpath_export.h>
#endif

#include <cstdint>   // std::uint64_t
#include <exception> // std::exception

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup jsonpath
/// An error that represents a JSONPath parsing failure
class SOURCEMETA_CORE_JSONPATH_EXPORT JSONPathParseError
    : public std::exception {
public:
  /// Construct an error given the column number where parsing failed
  JSONPathParseError(const std::uint64_t column) : column_{column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid JSON Path query";
  }

  /// Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
