#ifndef SOURCEMETA_CORE_CRYPTO_UUID_H_
#define SOURCEMETA_CORE_CRYPTO_UUID_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <string>      // std::string
#include <string_view> // std::string_view

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

/// @ingroup crypto
/// Check whether the given string is shaped like a UUID: exactly 36
/// characters in the `8-4-4-4-12` hex-with-dashes layout, case-insensitive.
/// This is a purely lexical check and does not validate the version or
/// variant nibbles.
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_uuid_like("98d80576-482e-427f-8434-7f86890ab222"));
/// assert(sourcemeta::core::is_uuid_like("00000000-0000-0000-0000-000000000000"));
/// assert(!sourcemeta::core::is_uuid_like("not-a-uuid"));
/// ```
SOURCEMETA_CORE_CRYPTO_EXPORT
auto is_uuid_like(const std::string_view value) -> bool;

} // namespace sourcemeta::core

#endif
