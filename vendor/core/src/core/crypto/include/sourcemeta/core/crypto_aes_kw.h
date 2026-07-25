#ifndef SOURCEMETA_CORE_CRYPTO_AES_KW_H_
#define SOURCEMETA_CORE_CRYPTO_AES_KW_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Wrap a key with AES Key Wrap (RFC 3394) under a key-encryption key whose
/// size selects AES-128, AES-192, or AES-256. The wrapped output is eight bytes
/// longer than the input and carries an integrity check. Returns no value when
/// the key-encryption key is not 128, 192, or 256 bits, the key being wrapped
/// is not a whole number of at least two 64-bit blocks, or an input is too
/// large to process. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto wrapped{sourcemeta::core::aes_key_wrap(key_encryption_key, key)};
/// assert(wrapped.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT aes_key_wrap(
    const std::string_view key_encryption_key, const std::string_view key)
    -> std::optional<std::string>;

/// @ingroup crypto
/// Unwrap a key wrapped with aes_key_wrap under the same key-encryption key,
/// returning the original key. The integrity check is verified, so a tampered
/// or wrongly keyed input is rejected rather than returned. Returns no value
/// when the key-encryption key is not a valid length, the wrapped key is not a
/// whole number of at least three 64-bit blocks, an input is too large to
/// process, or the integrity check fails. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{
///     sourcemeta::core::aes_key_unwrap(key_encryption_key, wrapped.value())};
/// assert(key.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT aes_key_unwrap(
    const std::string_view key_encryption_key,
    const std::string_view wrapped_key) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
