#ifndef SOURCEMETA_CORE_CRYPTO_CRC32_H_
#define SOURCEMETA_CORE_CRYPTO_CRC32_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <cstdint>     // std::uint32_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Compute the CRC-32 checksum (ISO 3309, polynomial 0xEDB88320) of a byte
/// sequence. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::crc32("123456789") << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT crc32(const std::string_view input)
    -> std::uint32_t;

/// @ingroup crypto
/// Extend an existing CRC-32 checksum with additional bytes, returning the
/// updated value. Pass zero as the previous value to start a fresh
/// computation. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// auto checksum{sourcemeta::core::crc32_update(0, "hello")};
/// checksum = sourcemeta::core::crc32_update(checksum, " world");
/// std::cout << checksum << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT crc32_update(const std::uint32_t previous,
                                                const std::string_view input)
    -> std::uint32_t;

} // namespace sourcemeta::core

#endif
