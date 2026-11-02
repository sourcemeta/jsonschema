#ifndef SOURCEMETA_CORE_PUNYCODE_ERROR_H_
#define SOURCEMETA_CORE_PUNYCODE_ERROR_H_

#ifndef SOURCEMETA_CORE_PUNYCODE_EXPORT
#include <sourcemeta/core/punycode_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup punycode
/// An error that represents a Punycode encoding or decoding failure
class SOURCEMETA_CORE_PUNYCODE_EXPORT PunycodeError : public Error {
public:
  /// Construct an error with the given message
  PunycodeError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
