#ifndef SOURCEMETA_HYDRA_CRYPTO_H
#define SOURCEMETA_HYDRA_CRYPTO_H

#ifndef SOURCEMETA_HYDRA_CRYPTO_EXPORT
#include <sourcemeta/hydra/crypto_export.h>
#endif

#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup crypto Crypto
/// @brief This module offers a collection of cryptographic utilities for use in
/// the other modules.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// ```

namespace sourcemeta::hydra {

/// @ingroup crypto
/// Generate a random UUID v4 string. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// #include <iostream>
///
/// std::cout << sourcemeta::hydra::uuid() << "\n";
/// ```
auto SOURCEMETA_HYDRA_CRYPTO_EXPORT uuid() -> std::string;

/// @ingroup crypto
/// Encode a string using Base64. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::hydra::base64_encode("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_HYDRA_CRYPTO_EXPORT base64_encode(std::string_view input,
                                                  std::ostream &output) -> void;

/// @ingroup crypto
/// Hash a string using MD5. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::hydra::md5("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_HYDRA_CRYPTO_EXPORT md5(std::string_view input,
                                        std::ostream &output) -> void;

/// @ingroup crypto
/// Hash a string using SHA256. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::hydra::sha256("foo bar", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_HYDRA_CRYPTO_EXPORT sha256(std::string_view input,
                                           std::ostream &output) -> void;

/// @ingroup crypto
/// Compute a HMAC-SHA256 key. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/crypto.h>
/// #include <sstream>
/// #include <iostream>
///
/// std::ostringstream result;
/// sourcemeta::hydra::hmac_sha256("my secret", "my value", result);
/// std::cout << result.str() << "\n";
/// ```
auto SOURCEMETA_HYDRA_CRYPTO_EXPORT hmac_sha256(std::string_view secret,
                                                std::string_view value,
                                                std::ostream &output) -> void;

} // namespace sourcemeta::hydra

#endif
