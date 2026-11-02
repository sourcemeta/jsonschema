#ifndef SOURCEMETA_CORE_CRYPTO_RSA_OAEP_H_
#define SOURCEMETA_CORE_CRYPTO_RSA_OAEP_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/crypto_verify.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// The hash function that RSA-OAEP uses for both the label digest and the mask
/// generation function.
enum class RSAOAEPHash : std::uint8_t {
  /// The SHA-1 hash function, selecting RSA-OAEP.
  SHA1,
  /// The SHA-256 hash function, selecting RSA-OAEP-256.
  SHA256
};

/// @ingroup crypto
/// Encrypt a short key or message with RSA-OAEP (RFC 8017) under an RSA public
/// key, where SHA1 selects RSA-OAEP and SHA256 selects RSA-OAEP-256. Returns no
/// value when the key is not RSA, the plaintext is too long for the modulus, or
/// the operation fails. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto wrapped{sourcemeta::core::rsa_oaep_encrypt(
///     public_key, sourcemeta::core::RSAOAEPHash::SHA256, key)};
/// assert(wrapped.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsa_oaep_encrypt(
    const PublicKey &key, const RSAOAEPHash hash,
    const std::string_view plaintext) -> std::optional<std::string>;

/// @ingroup crypto
/// Decrypt a message produced by rsa_oaep_encrypt under the matching RSA
/// private key and hash, returning the original plaintext. Returns no value
/// when the key is not RSA, the ciphertext is malformed, or the padding check
/// fails. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::rsa_oaep_decrypt(
///     private_key, sourcemeta::core::RSAOAEPHash::SHA256, wrapped.value())};
/// assert(key.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsa_oaep_decrypt(
    const PrivateKey &key, const RSAOAEPHash hash,
    const std::string_view ciphertext) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
