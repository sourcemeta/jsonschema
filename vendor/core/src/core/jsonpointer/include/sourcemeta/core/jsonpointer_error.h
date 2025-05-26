#ifndef SOURCEMETA_CORE_JSONPOINTER_ERROR_H_
#define SOURCEMETA_CORE_JSONPOINTER_ERROR_H_

#ifndef SOURCEMETA_CORE_JSONPOINTER_EXPORT
#include <sourcemeta/core/jsonpointer_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <cstdint> // std::uint64_t

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// This class represents a parsing error.
class SOURCEMETA_CORE_JSONPOINTER_EXPORT PointerParseError
    // TODO: It makes no sense for a JSON Pointer error to inherit from a JSON
    // error. Make them independent
    : public JSONParseError {
public:
  /// Create a parsing error
  PointerParseError(const std::uint64_t column) : JSONParseError{1, column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid JSON Pointer";
  }
};

} // namespace sourcemeta::core

#endif
