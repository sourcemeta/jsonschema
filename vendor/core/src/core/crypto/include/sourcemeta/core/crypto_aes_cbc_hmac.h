#ifndef SOURCEMETA_CORE_CRYPTO_AES_CBC_HMAC_H_
#define SOURCEMETA_CORE_CRYPTO_AES_CBC_HMAC_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <cassert>     // assert
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// The output of an AES-CBC-HMAC encryption, a single owned buffer holding the
/// ciphertext followed by its authentication tag, exposed as views.
struct AESCBCHMACCiphertext {
  /// The ciphertext followed by its authentication tag.
  std::string data;

  /// The length in bytes of the trailing authentication tag.
  std::string::size_type tag_length;

  /// The ciphertext, a whole number of 16-byte blocks.
  [[nodiscard]] auto ciphertext() const -> std::string_view {
    assert(this->data.size() >= this->tag_length);
    return std::string_view{this->data}.substr(0, this->data.size() -
                                                      this->tag_length);
  }

  /// The authentication tag.
  [[nodiscard]] auto tag() const -> std::string_view {
    assert(this->data.size() >= this->tag_length);
    return std::string_view{this->data}.substr(this->data.size() -
                                               this->tag_length);
  }
};

/// @ingroup crypto
/// Encrypt a plaintext with AES-CBC-HMAC-SHA2 (RFC 7518 Section 5.2) under a
/// key whose size selects the variant, a 128-bit initialization vector, and
/// associated data that is authenticated but not encrypted. A 32, 48, or
/// 64-byte key selects A128CBC-HS256, A192CBC-HS384, or A256CBC-HS512
/// respectively. Returns no value when the key is not one of the three valid
/// sizes, the initialization vector is not 128 bits, or an input is too large
/// to process. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto result{
///     sourcemeta::core::aes_cbc_hmac_encrypt(key, iv, "", "hello")};
/// assert(result.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT aes_cbc_hmac_encrypt(
    const std::string_view key, const std::string_view iv,
    const std::string_view associated_data, const std::string_view plaintext)
    -> std::optional<AESCBCHMACCiphertext>;

/// @ingroup crypto
/// Decrypt a message produced by aes_cbc_hmac_encrypt under the same key,
/// initialization vector, and associated data, returning the original
/// plaintext. The authentication tag is verified in constant time before, and
/// independent of, the decryption, so a tampered message is rejected rather
/// than decrypted. Returns no value when the key or initialization vector is
/// not a valid length, the tag length does not match the key, an input is
/// malformed or too large to process, or the authentication tag does not
/// verify. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto plaintext{sourcemeta::core::aes_cbc_hmac_decrypt(
///     key, iv, "", result.value().ciphertext(), result.value().tag())};
/// assert(plaintext.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT aes_cbc_hmac_decrypt(
    const std::string_view key, const std::string_view iv,
    const std::string_view associated_data, const std::string_view ciphertext,
    const std::string_view tag) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
