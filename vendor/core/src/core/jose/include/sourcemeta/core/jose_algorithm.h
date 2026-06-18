#ifndef SOURCEMETA_CORE_JOSE_ALGORITHM_H_
#define SOURCEMETA_CORE_JOSE_ALGORITHM_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// The asymmetric JSON Web Signature algorithms from RFC 7518 Section 3.1 and
/// the Edwards-curve algorithm from RFC 8037 Section 3.1. The symmetric HMAC
/// family and the null algorithm are intentionally absent, which makes
/// algorithm confusion attacks unrepresentable in the type system.
enum class JWSAlgorithm : std::uint8_t {
  RS256,
  RS384,
  RS512,
  PS256,
  PS384,
  PS512,
  ES256,
  ES384,
  ES512,
  EdDSA
};

/// @ingroup jose
/// Map a JSON Web Signature `alg` value to its algorithm, returning no value
/// for any unrecognized name. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_jws_algorithm("RS256").has_value());
/// assert(!sourcemeta::core::to_jws_algorithm("none").has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto to_jws_algorithm(const std::string_view value) noexcept
    -> std::optional<JWSAlgorithm>;

} // namespace sourcemeta::core

#endif
