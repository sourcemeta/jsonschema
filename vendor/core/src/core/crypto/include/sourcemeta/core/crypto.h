#ifndef SOURCEMETA_CORE_CRYPTO_H_
#define SOURCEMETA_CORE_CRYPTO_H_

/// @defgroup crypto Crypto
/// @brief Cryptographic hash functions, UUID generation, and Base64 codecs.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// ```

#include <sourcemeta/core/crypto_base64.h>
#include <sourcemeta/core/crypto_crc32.h>
#include <sourcemeta/core/crypto_fnv128.h>
#include <sourcemeta/core/crypto_sha1.h>
#include <sourcemeta/core/crypto_sha256.h>
#include <sourcemeta/core/crypto_sha384.h>
#include <sourcemeta/core/crypto_sha512.h>
#include <sourcemeta/core/crypto_uuid.h>
#include <sourcemeta/core/crypto_verify.h>

#endif
