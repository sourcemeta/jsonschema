#ifndef SOURCEMETA_CORE_JSONPATH_ERROR_H_
#define SOURCEMETA_CORE_JSONPATH_ERROR_H_

#ifndef SOURCEMETA_CORE_JSONPATH_EXPORT
#include <sourcemeta/core/jsonpath_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint> // std::uint64_t

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup jsonpath
/// An error that represents a JSONPath parsing failure
class SOURCEMETA_CORE_JSONPATH_EXPORT JSONPathParseError : public Error {
public:
  /// Construct an error given the column number where parsing failed
  JSONPathParseError(const std::uint64_t column) noexcept
      : Error{"The input is not a valid JSON Path query"}, column_{column} {}

  /// Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
