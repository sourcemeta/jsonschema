#ifndef SOURCEMETA_CORE_OIDC_ENCRYPTION_H_
#define SOURCEMETA_CORE_OIDC_ENCRYPTION_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/jose.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Decrypt a nested encrypted token, recovering the inner signed JWT from a
/// compact JWE, returning no value when the input is not a valid JWE or the key
/// cannot decrypt it (OpenID Connect Core 1.0 Section 10.2). OpenID Connect
/// encrypted ID Tokens, UserInfo responses, and request objects are a signed
/// JWT wrapped in a JWE, so the Relying Party decrypts first and then verifies
/// the recovered signature. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto inner{sourcemeta::core::oidc_decrypt_nested_jwt(compact, key)};
/// if (inner.has_value()) {
///   const auto token{sourcemeta::core::JWT::from(inner.value())};
///   assert(token.has_value());
/// }
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_decrypt_nested_jwt(const std::string_view compact,
                             const JWKPrivate &key)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
