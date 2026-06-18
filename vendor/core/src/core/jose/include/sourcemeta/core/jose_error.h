#ifndef SOURCEMETA_CORE_JOSE_ERROR_H_
#define SOURCEMETA_CORE_JOSE_ERROR_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <exception> // std::exception

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Key.
class SOURCEMETA_CORE_JOSE_EXPORT JWKParseError : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid JSON Web Key";
  }
};

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Key Set.
class SOURCEMETA_CORE_JOSE_EXPORT JWKSParseError : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid JSON Web Key Set";
  }
};

/// @ingroup jose
/// An error that occurs when parsing an invalid JSON Web Token.
class SOURCEMETA_CORE_JOSE_EXPORT JWTParseError : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid JSON Web Token";
  }
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
