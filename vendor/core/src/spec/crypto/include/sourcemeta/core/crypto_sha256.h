#ifndef SOURCEMETA_CORE_CRYPTO_SHA256_H_
#define SOURCEMETA_CORE_CRYPTO_SHA256_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <ostream>     // std::ostream
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Hash a string using SHA-256. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::core::sha256("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha256(const std::string_view input,
                                          std::ostream &output) -> void;

/// @ingroup crypto
/// Hash a string using SHA-256, returning the hex digest as a string.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::sha256("foo bar") << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha256(const std::string_view input)
    -> std::string;

/// @ingroup crypto
/// Hash a string using SHA-256, returning the raw digest bytes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto digest{sourcemeta::core::sha256_digest("foo bar")};
/// assert(digest.size() == 32);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha256_digest(const std::string_view input)
    -> std::array<std::uint8_t, 32>;

/// @ingroup crypto
/// Hash a sequence of string parts using SHA-256 as if they were a single
/// concatenated input, returning the raw digest bytes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 2> parts{{"foo ", "bar"}};
/// const auto digest{sourcemeta::core::sha256_digest(parts)};
/// assert(digest.size() == 32);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha256_digest(
    std::span<const std::string_view> input) -> std::array<std::uint8_t, 32>;

} // namespace sourcemeta::core

#endif
