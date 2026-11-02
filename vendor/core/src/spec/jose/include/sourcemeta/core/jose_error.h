#ifndef SOURCEMETA_CORE_JOSE_ERROR_H_
#define SOURCEMETA_CORE_JOSE_ERROR_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Key.
class SOURCEMETA_CORE_JOSE_EXPORT JWKParseError : public Error {
public:
  JWKParseError() noexcept : Error{"The input is not a valid JSON Web Key"} {}
};

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Key Set.
class SOURCEMETA_CORE_JOSE_EXPORT JWKSParseError : public Error {
public:
  JWKSParseError() noexcept
      : Error{"The input is not a valid JSON Web Key Set"} {}
};

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Token.
class SOURCEMETA_CORE_JOSE_EXPORT JWTParseError : public Error {
public:
  JWTParseError() noexcept : Error{"The input is not a valid JSON Web Token"} {}
};

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Encryption object.
class SOURCEMETA_CORE_JOSE_EXPORT JWEParseError : public Error {
public:
  JWEParseError() noexcept
      : Error{"The input is not a valid JSON Web Encryption object"} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
