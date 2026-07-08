#ifndef SOURCEMETA_CORE_CRYPTO_AES_APPLE_H_
#define SOURCEMETA_CORE_CRYPTO_AES_APPLE_H_

#include <cstddef> // std::size_t

// Seal a plaintext with AES-256 in Galois/Counter Mode through CryptoKit,
// writing the ciphertext followed by the 16-byte tag into the output buffer,
// which the caller sizes to the plaintext length plus 16. Returns false when
// the key or nonce is not a valid length
extern "C" auto sourcemeta_core_aes_256_gcm_seal_cryptokit(
    const unsigned char *key, std::size_t key_size, const unsigned char *nonce,
    std::size_t nonce_size, const unsigned char *plaintext,
    std::size_t plaintext_size, unsigned char *output) -> bool;

// Open a message sealed with the function above, writing the plaintext into the
// output buffer, which the caller sizes to the ciphertext length. Returns false
// when the tag does not verify or the input is malformed
extern "C" auto sourcemeta_core_aes_256_gcm_open_cryptokit(
    const unsigned char *key, std::size_t key_size, const unsigned char *nonce,
    std::size_t nonce_size, const unsigned char *ciphertext,
    std::size_t ciphertext_size, const unsigned char *tag, std::size_t tag_size,
    unsigned char *output) -> bool;

#endif
