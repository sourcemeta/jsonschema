#ifndef SOURCEMETA_CORE_CRYPTO_SHA384_H_
#define SOURCEMETA_CORE_CRYPTO_SHA384_H_

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
/// Hash a string using SHA-384. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::core::sha384("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha384(const std::string_view input,
                                          std::ostream &output) -> void;

/// @ingroup crypto
/// Hash a string using SHA-384, returning the hex digest as a string.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::sha384("foo bar") << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha384(const std::string_view input)
    -> std::string;

/// @ingroup crypto
/// Hash a string using SHA-384, returning the raw digest bytes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto digest{sourcemeta::core::sha384_digest("foo bar")};
/// assert(digest.size() == 48);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha384_digest(const std::string_view input)
    -> std::array<std::uint8_t, 48>;

} // namespace sourcemeta::core

#endif
