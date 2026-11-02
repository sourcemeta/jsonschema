#ifndef SOURCEMETA_CORE_URI_ERROR_H_
#define SOURCEMETA_CORE_URI_ERROR_H_

#ifndef SOURCEMETA_CORE_URI_EXPORT
#include <sourcemeta/core/uri_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint> // std::uint64_t

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup uri
/// An error that represents a URI parsing failure
class SOURCEMETA_CORE_URI_EXPORT URIParseError : public Error {
public:
  /// Construct an error from the column at which parsing failed
  URIParseError(const std::uint64_t column) noexcept
      : Error{"The input is not a valid URI"}, column_{column} {}

  /// Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

/// @ingroup uri
/// An error that represents a general URI error event
class SOURCEMETA_CORE_URI_EXPORT URIError : public Error {
public:
  /// Construct an error with the given message
  URIError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
