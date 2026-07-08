#ifndef SOURCEMETA_CORE_CRYPTO_AES_GCM_H_
#define SOURCEMETA_CORE_CRYPTO_AES_GCM_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Seal a plaintext under a 256-bit key using AES-256 in Galois/Counter Mode
/// (NIST SP 800-38D) with a fresh random nonce and no associated data. The
/// result is the self-contained sealed message, the nonce followed by the
/// ciphertext and the authentication tag, and its confidentiality and
/// integrity are both protected when the library is built against a system
/// cryptography provider. Returns no value when the key is not 256 bits, the
/// plaintext is too large to process, or the random nonce could not be drawn.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto sealed{sourcemeta::core::aes_256_gcm_seal(key, "hello")};
/// assert(sealed.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
aes_256_gcm_seal(const std::string_view key, const std::string_view plaintext)
    -> std::optional<std::string>;

/// @ingroup crypto
/// Open a message sealed with aes_256_gcm_seal under the same 256-bit key,
/// returning the original plaintext. Returns no value when the key is not 256
/// bits, the input is too short to be a sealed message or too large to process,
/// or the authentication tag does not verify, so a tampered or truncated
/// message is rejected rather than decrypted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto plaintext{sourcemeta::core::aes_256_gcm_unseal(key, sealed)};
/// assert(plaintext.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
aes_256_gcm_unseal(const std::string_view key, const std::string_view sealed)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
