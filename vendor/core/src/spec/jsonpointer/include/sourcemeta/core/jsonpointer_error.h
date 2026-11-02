#ifndef SOURCEMETA_CORE_JSONPOINTER_ERROR_H_
#define SOURCEMETA_CORE_JSONPOINTER_ERROR_H_

#ifndef SOURCEMETA_CORE_JSONPOINTER_EXPORT
#include <sourcemeta/core/jsonpointer_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint> // std::uint64_t

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup jsonpointer
/// This class represents a parsing error.
class SOURCEMETA_CORE_JSONPOINTER_EXPORT PointerParseError : public Error {
public:
  /// Create a parsing error
  PointerParseError(const std::uint64_t column) noexcept
      : Error{"The input is not a valid JSON Pointer"}, column_{column} {}

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
