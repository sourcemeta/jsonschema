#ifndef SOURCEMETA_CORE_CRYPTO_KDF_APPLE_H_
#define SOURCEMETA_CORE_CRYPTO_KDF_APPLE_H_

#include <cstddef> // std::size_t

// The RFC 5869 steps through CryptoKit, defined in the Objective-C++ bridge
// that consumes the Swift shim. The hash selector matches the order of the
// shared enumeration. Each returns false when the running system predates the
// CryptoKit key derivation interface, which is macOS 11, so the caller
// composes the derivation from HMAC instead

extern "C" auto sourcemeta_core_hkdf_extract_cryptokit(
    int hash, const unsigned char *input_key_material,
    std::size_t input_key_material_size, const unsigned char *salt,
    std::size_t salt_size, unsigned char *output, std::size_t output_size)
    -> bool;

extern "C" auto sourcemeta_core_hkdf_expand_cryptokit(
    int hash, const unsigned char *pseudorandom_key,
    std::size_t pseudorandom_key_size, const unsigned char *info,
    std::size_t info_size, unsigned char *output, std::size_t length) -> bool;

extern "C" auto sourcemeta_core_hkdf_derive_cryptokit(
    int hash, const unsigned char *input_key_material,
    std::size_t input_key_material_size, const unsigned char *salt,
    std::size_t salt_size, const unsigned char *info, std::size_t info_size,
    unsigned char *output, std::size_t length) -> bool;

#endif
