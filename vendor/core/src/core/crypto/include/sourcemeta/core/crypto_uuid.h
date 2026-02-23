#ifndef SOURCEMETA_CORE_CRYPTO_UUID_H_
#define SOURCEMETA_CORE_CRYPTO_UUID_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <string> // std::string

namespace sourcemeta::core {

/// @ingroup crypto
/// Generate a random UUID v4 string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::core::uuidv4() << "\n";
/// ```
///
/// See https://www.rfc-editor.org/rfc/rfc9562#name-uuid-version-4
SOURCEMETA_CORE_CRYPTO_EXPORT auto uuidv4() -> std::string;

} // namespace sourcemeta::core

#endif
