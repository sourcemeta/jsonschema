#ifndef SOURCEMETA_CORE_CRYPTO_FNV128_H_
#define SOURCEMETA_CORE_CRYPTO_FNV128_H_

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
/// Hash a string using the non-cryptographic 128-bit FNV-1 function,
/// returning the raw digest in big-endian byte order. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
///
/// const auto digest{sourcemeta::core::fnv128_digest("foo bar")};
/// assert(digest.size() == 16);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT fnv128_digest(const std::string_view input)
    -> std::array<std::uint8_t, 16>;

/// @ingroup crypto
/// Hash a string using the non-cryptographic 128-bit FNV-1 function. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::core::fnv128("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT fnv128(const std::string_view input,
                                          std::ostream &output) -> void;

/// @ingroup crypto
/// Hash a string using the non-cryptographic 128-bit FNV-1 function,
/// returning the hex digest as a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::fnv128("foo bar") << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT fnv128(const std::string_view input)
    -> std::string;

} // namespace sourcemeta::core

#endif
