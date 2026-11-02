#ifndef SOURCEMETA_CORE_CRYPTO_EDDSA_APPLE_H_
#define SOURCEMETA_CORE_CRYPTO_EDDSA_APPLE_H_

#include <cstddef> // std::size_t

// Verify an Ed25519 signature through CryptoKit, defined in the Objective-C++
// bridge that consumes the Swift shim. The signature is invalid rather than an
// error for any malformed input, including a key or signature of the wrong
// length, since CryptoKit rejects those inputs
extern "C" auto sourcemeta_core_eddsa_ed25519_verify_cryptokit(
    const unsigned char *public_key, std::size_t public_key_size,
    const unsigned char *message, std::size_t message_size,
    const unsigned char *signature, std::size_t signature_size) -> bool;

// Produce an Ed25519 signature through CryptoKit, writing the 64-byte signature
// into the output buffer. Returns false when the seed is not a valid key
extern "C" auto sourcemeta_core_eddsa_ed25519_sign_cryptokit(
    const unsigned char *seed, std::size_t seed_size,
    const unsigned char *message, std::size_t message_size,
    unsigned char *signature) -> bool;

#endif
