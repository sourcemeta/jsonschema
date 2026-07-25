#ifndef SOURCEMETA_CORE_JOSE_ENCRYPT_H_
#define SOURCEMETA_CORE_JOSE_ENCRYPT_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <sourcemeta/core/jose_jwk.h>

#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// Encrypt a plaintext into a JSON Web Encryption object in compact
/// serialization (RFC 7516). The key-management algorithm and content
/// encryption algorithm are taken from the protected header `alg` and `enc`
/// parameters (RFC 7516 Sections 4.1.1 and 4.1.2). A fresh initialization
/// vector is always generated. The content encryption key is generated as the
/// algorithm requires: freshly random for the key-wrapping and RSA algorithms,
/// derived from the agreement for direct `ECDH-ES`, and the shared secret
/// itself for `dir`. For the `ECDH-ES` algorithms an ephemeral key is minted
/// and its public part added to the emitted protected header as the `epk`
/// parameter. The key is the recipient public key for the asymmetric algorithms
/// and the shared octet secret for `A*KW` and `dir`. Returns no value when the
/// header is not an object, has duplicate members, carries a critical extension
/// or a compression parameter, names an unsupported or key-incompatible
/// algorithm, or the key cannot serve it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto object{sourcemeta::core::jwe_encrypt(
///     sourcemeta::core::parse_json(R"({ "alg": "RSA-OAEP-256",
///                                       "enc": "A128GCM" })"),
///     "secret message", recipient)};
/// assert(object.has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwe_encrypt(const JSON &header, const std::string_view plaintext,
                 const JWK &key) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
