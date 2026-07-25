#ifndef SOURCEMETA_CORE_CRYPTO_AES_APPLE_H_
#define SOURCEMETA_CORE_CRYPTO_AES_APPLE_H_

#include <cstddef> // std::size_t

// Seal a plaintext with AES in Galois/Counter Mode through CryptoKit, writing
// the ciphertext (the plaintext length) and its 16-byte tag into the two output
// buffers. Returns false when the key, nonce, or a buffer is invalid
extern "C" auto sourcemeta_core_aes_gcm_seal_cryptokit(
    const unsigned char *key, std::size_t key_size, const unsigned char *nonce,
    std::size_t nonce_size, const unsigned char *plaintext,
    std::size_t plaintext_size, const unsigned char *associated_data,
    std::size_t associated_data_size, unsigned char *ciphertext_output,
    unsigned char *tag_output) -> bool;

// Open a message sealed with the function above, writing the plaintext into the
// output buffer, which the caller sizes to the ciphertext length. Returns false
// when the tag does not verify or the input is malformed
extern "C" auto sourcemeta_core_aes_gcm_open_cryptokit(
    const unsigned char *key, std::size_t key_size, const unsigned char *nonce,
    std::size_t nonce_size, const unsigned char *ciphertext,
    std::size_t ciphertext_size, const unsigned char *tag, std::size_t tag_size,
    const unsigned char *associated_data, std::size_t associated_data_size,
    unsigned char *output) -> bool;

#endif
