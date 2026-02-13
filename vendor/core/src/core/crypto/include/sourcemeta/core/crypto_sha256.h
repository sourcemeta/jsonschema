#ifndef SOURCEMETA_CORE_CRYPTO_SHA256_H_
#define SOURCEMETA_CORE_CRYPTO_SHA256_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <ostream>     // std::ostream
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

} // namespace sourcemeta::core

#endif
