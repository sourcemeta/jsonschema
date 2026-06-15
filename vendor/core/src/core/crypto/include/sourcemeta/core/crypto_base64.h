#ifndef SOURCEMETA_CORE_CRYPTO_BASE64_H_
#define SOURCEMETA_CORE_CRYPTO_BASE64_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Encode a byte sequence using Base64 (RFC 4648 Section 4) into a stream.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::ostringstream result;
/// sourcemeta::core::base64_encode("foobar", result);
/// assert(result.str() == "Zm9vYmFy");
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT base64_encode(const std::string_view input,
                                                 std::ostream &output) -> void;

/// @ingroup crypto
/// Encode a byte sequence using Base64 (RFC 4648 Section 4). For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::base64_encode("foobar") == "Zm9vYmFy");
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT base64_encode(const std::string_view input)
    -> std::string;

/// @ingroup crypto
/// Decode a Base64 string (RFC 4648 Section 4), returning no value unless the
/// input is a canonical padded encoding. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::base64_decode("Zm9vYmFy")};
/// assert(result.has_value());
/// assert(result.value() == "foobar");
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT base64_decode(const std::string_view input)
    -> std::optional<std::string>;

/// @ingroup crypto
/// Encode a byte sequence using unpadded Base64url (RFC 4648 Section 5) into a
/// stream. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::ostringstream result;
/// sourcemeta::core::base64url_encode("fo", result);
/// assert(result.str() == "Zm8");
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
base64url_encode(const std::string_view input, std::ostream &output) -> void;

/// @ingroup crypto
/// Encode a byte sequence using unpadded Base64url (RFC 4648 Section 5). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::base64url_encode("fo") == "Zm8");
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
base64url_encode(const std::string_view input) -> std::string;

/// @ingroup crypto
/// Decode an unpadded Base64url string (RFC 4648 Section 5), returning no
/// value unless the input is a canonical encoding. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::base64url_decode("Zm8")};
/// assert(result.has_value());
/// assert(result.value() == "fo");
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
base64url_decode(const std::string_view input) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
