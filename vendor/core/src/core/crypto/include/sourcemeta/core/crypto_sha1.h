#ifndef SOURCEMETA_CORE_CRYPTO_SHA1_H_
#define SOURCEMETA_CORE_CRYPTO_SHA1_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Hash a string using SHA-1 (RFC 3174). For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::core::sha1("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha1(const std::string_view input,
                                        std::ostream &output) -> void;

/// @ingroup crypto
/// Hash a string using SHA-1 (RFC 3174), returning the hex digest as a
/// string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::sha1("foo bar") << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT sha1(const std::string_view input)
    -> std::string;

} // namespace sourcemeta::core

#endif
