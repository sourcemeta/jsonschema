#ifndef SOURCEMETA_CORE_CRYPTO_ERROR_H_
#define SOURCEMETA_CORE_CRYPTO_ERROR_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup crypto
/// An error that represents a cryptographic operation that the backend could
/// not carry out
class SOURCEMETA_CORE_CRYPTO_EXPORT CryptoError : public Error {
public:
  /// Construct an error with the given message
  CryptoError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
