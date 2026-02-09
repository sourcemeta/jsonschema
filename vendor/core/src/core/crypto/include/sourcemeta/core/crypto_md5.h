#ifndef SOURCEMETA_CORE_CRYPTO_MD5_H_
#define SOURCEMETA_CORE_CRYPTO_MD5_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <ostream>     // std::ostream
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Hash a string using MD5. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::core::md5("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT md5(const std::string_view input,
                                       std::ostream &output) -> void;

} // namespace sourcemeta::core

#endif
