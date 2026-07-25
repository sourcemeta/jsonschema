#ifndef SOURCEMETA_CORE_JOSE_DECRYPT_H_
#define SOURCEMETA_CORE_JOSE_DECRYPT_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <sourcemeta/core/jose_jwe.h>
#include <sourcemeta/core/jose_jwk_private.h>

#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::core {

/// @ingroup jose
/// Decrypt a JSON Web Encryption object with the recipient key, returning the
/// plaintext (RFC 7516). The key is the recipient private key for the
/// asymmetric algorithms and the shared octet secret for `A*KW` and `dir`.
/// Returns no value on any failure, uniformly, so that a key-unwrap failure, an
/// authentication tag mismatch, and a structural error are indistinguishable
/// and the recipient cannot be used as a decryption oracle (RFC 7516 Sections
/// 11.4 and 11.5). For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// const auto object{sourcemeta::core::JWE::from(compact)};
/// assert(object.has_value());
/// const auto plaintext{sourcemeta::core::jwe_decrypt(object.value(), key)};
/// assert(plaintext.has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwe_decrypt(const JWE &jwe, const JWKPrivate &key)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
