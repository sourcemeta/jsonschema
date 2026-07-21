#ifndef SOURCEMETA_CORE_CRYPTO_H_
#define SOURCEMETA_CORE_CRYPTO_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

/// @defgroup crypto Crypto
/// @brief Cryptographic hashing and HMAC, digital signatures, authenticated
/// encryption, random bytes, constant-time comparison, and Base64 and UUID
/// helpers.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// ```

#include <sourcemeta/core/crypto_aes_gcm.h>
#include <sourcemeta/core/crypto_base64.h>
#include <sourcemeta/core/crypto_crc32.h>
#include <sourcemeta/core/crypto_fnv128.h>
#include <sourcemeta/core/crypto_hmac_sha256.h>
#include <sourcemeta/core/crypto_hmac_sha384.h>
#include <sourcemeta/core/crypto_hmac_sha512.h>
#include <sourcemeta/core/crypto_secure.h>
#include <sourcemeta/core/crypto_sha1.h>
#include <sourcemeta/core/crypto_sha256.h>
#include <sourcemeta/core/crypto_sha384.h>
#include <sourcemeta/core/crypto_sha512.h>
#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/crypto_uuid.h>
#include <sourcemeta/core/crypto_verify.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Fill a buffer with random bytes drawn from the operating system's
/// cryptographically secure provider. The bytes are only cryptographically
/// secure when the library is built against a system provider (OpenSSL, the
/// Apple Security framework, or Windows CNG). The reference backend used when
/// no system provider is available falls back to a non-cryptographic generator.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <array>
/// #include <cstdint>
///
/// std::array<std::uint8_t, 16> buffer{};
/// sourcemeta::core::random_bytes(buffer);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT random_bytes(std::span<std::uint8_t> buffer)
    -> void;

/// @ingroup crypto
/// Return the given number of random bytes as a string, drawn from the
/// operating system's cryptographically secure provider. The same backend
/// caveat as the buffer-filling overload applies. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto bytes{sourcemeta::core::random_bytes(32)};
/// assert(bytes.size() == 32);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT random_bytes(const std::size_t length)
    -> std::string;

/// @ingroup crypto
/// Compare two byte sequences for equality in constant time when their lengths
/// match, returning false immediately when the lengths differ. The comparison
/// does not short-circuit on the first differing byte, so it does not leak the
/// position of a mismatch through timing. The length of the inputs is not
/// treated as secret. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::secure_equals("expected", "expected"));
/// assert(!sourcemeta::core::secure_equals("expected", "actual"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT secure_equals(
    const std::string_view left, const std::string_view right) noexcept -> bool;

} // namespace sourcemeta::core

#endif
