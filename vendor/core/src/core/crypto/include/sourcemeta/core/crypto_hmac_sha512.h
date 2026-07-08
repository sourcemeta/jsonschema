#ifndef SOURCEMETA_CORE_CRYPTO_HMAC_SHA512_H_
#define SOURCEMETA_CORE_CRYPTO_HMAC_SHA512_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Authenticate a message under a key using HMAC-SHA512, writing the hex
/// digest. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::core::hmac_sha512("key", "foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT hmac_sha512(const std::string_view key,
                                               const std::string_view message,
                                               std::ostream &output) -> void;

/// @ingroup crypto
/// Authenticate a message under a key using HMAC-SHA512, returning the hex
/// digest as a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::hmac_sha512("key", "foo bar") << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT hmac_sha512(const std::string_view key,
                                               const std::string_view message)
    -> std::string;

/// @ingroup crypto
/// Authenticate a message under a key using HMAC-SHA512, returning the raw
/// digest bytes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto digest{sourcemeta::core::hmac_sha512_digest("key", "foo bar")};
/// assert(digest.size() == 64);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
hmac_sha512_digest(const std::string_view key, const std::string_view message)
    -> std::array<std::uint8_t, 64>;

} // namespace sourcemeta::core

#endif
