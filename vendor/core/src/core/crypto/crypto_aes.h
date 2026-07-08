#ifndef SOURCEMETA_CORE_CRYPTO_AES_H_
#define SOURCEMETA_CORE_CRYPTO_AES_H_

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// The raw AES-256 Galois/Counter Mode primitive (NIST SP 800-38D) with a 96-bit
// nonce, no associated data, and a 128-bit tag, defined once per backend. The
// framing that adds the nonce and length checks lives in the shared seal and
// unseal functions, so these assume a 32-byte key and a 12-byte nonce

// Encrypt the plaintext, returning the ciphertext followed by the 16-byte
// authentication tag
auto aes_256_gcm_encrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view plaintext)
    -> std::optional<std::string>;

// Decrypt the ciphertext, which is followed by its 16-byte tag, verifying the
// tag before returning the plaintext and rejecting a mismatch
auto aes_256_gcm_decrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view ciphertext)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
