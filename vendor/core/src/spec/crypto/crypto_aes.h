#ifndef SOURCEMETA_CORE_CRYPTO_AES_H_
#define SOURCEMETA_CORE_CRYPTO_AES_H_

#include <sourcemeta/core/crypto_aes_gcm.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// The raw AES Galois/Counter Mode primitive (NIST SP 800-38D), defined once per
// backend, over a 128, 192, or 256-bit key, a 96-bit initialization vector, and
// associated data that is authenticated but not encrypted. The length checks
// live in the shared encrypt and decrypt functions, so these assume a 16, 24,
// or 32-byte key, a 12-byte initialization vector, and a 16-byte tag

// Encrypt the plaintext, returning the ciphertext and its 16-byte tag
auto aes_gcm_seal(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext>;

// Decrypt the ciphertext, verifying the tag before returning the plaintext and
// rejecting a mismatch
auto aes_gcm_open(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view ciphertext, const std::string_view tag)
    -> std::optional<std::string>;

// The raw AES Cipher Block Chaining primitive (NIST SP 800-38A), defined once
// per backend, over a 128, 192, or 256-bit key and a 128-bit initialization
// vector. The key splitting, padding, and authentication live in the shared
// functions, so these assume a 16, 24, or 32-byte key, a 16-byte initialization
// vector, and input that is a whole number of 16-byte blocks

auto aes_cbc_encrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view plaintext)
    -> std::optional<std::string>;

auto aes_cbc_decrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view ciphertext)
    -> std::optional<std::string>;

// The raw AES Key Wrap primitive (RFC 3394), defined once per backend, over a
// 128, 192, or 256-bit key-encryption key. The length checks live in the shared
// functions, so these assume a 16, 24, or 32-byte key-encryption key and input
// that is a whole number of 64-bit blocks

auto aes_wrap(const std::string_view key_encryption_key,
              const std::string_view key) -> std::optional<std::string>;

auto aes_unwrap(const std::string_view key_encryption_key,
                const std::string_view wrapped_key)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
