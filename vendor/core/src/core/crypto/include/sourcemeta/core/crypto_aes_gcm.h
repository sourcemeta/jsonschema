#ifndef SOURCEMETA_CORE_CRYPTO_AES_GCM_H_
#define SOURCEMETA_CORE_CRYPTO_AES_GCM_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <cassert>     // assert
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// The output of an AES-GCM encryption, a single owned buffer holding the
/// ciphertext followed by its 16-byte authentication tag, exposed as views.
struct AESGCMCiphertext {
  /// The ciphertext followed by its 16-byte authentication tag.
  std::string data;

  /// The ciphertext, the same length as the plaintext.
  [[nodiscard]] auto ciphertext() const -> std::string_view {
    assert(this->data.size() >= 16);
    return std::string_view{this->data}.substr(0, this->data.size() - 16);
  }

  /// The 16-byte authentication tag.
  [[nodiscard]] auto tag() const -> std::string_view {
    assert(this->data.size() >= 16);
    return std::string_view{this->data}.substr(this->data.size() - 16);
  }
};

/// @ingroup crypto
/// Encrypt a plaintext with AES in Galois/Counter Mode (NIST SP 800-38D) under
/// a 128, 192, or 256-bit key, a 96-bit initialization vector, and associated
/// data that is authenticated but not encrypted. Returns no value when the key
/// is not one of the three valid sizes, the initialization vector is not 96
/// bits, or an input is too large to process. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::aes_gcm_encrypt(key, iv, "", "hello")};
/// assert(result.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT aes_gcm_encrypt(
    const std::string_view key, const std::string_view iv,
    const std::string_view associated_data, const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext>;

/// @ingroup crypto
/// Decrypt a message produced by aes_gcm_encrypt under the same key,
/// initialization vector, and associated data, returning the original
/// plaintext. Returns no value when the key or initialization vector is not a
/// valid length, the tag is not 16 bytes, an input is too large to process, or
/// the authentication tag does not verify, so a tampered message is rejected
/// rather than decrypted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto plaintext{sourcemeta::core::aes_gcm_decrypt(
///     key, iv, "", result.value().ciphertext(), result.value().tag())};
/// assert(plaintext.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT aes_gcm_decrypt(
    const std::string_view key, const std::string_view iv,
    const std::string_view associated_data, const std::string_view ciphertext,
    const std::string_view tag) -> std::optional<std::string>;

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
