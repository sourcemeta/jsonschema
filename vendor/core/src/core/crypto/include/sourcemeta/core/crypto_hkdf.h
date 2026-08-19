#ifndef SOURCEMETA_CORE_CRYPTO_HKDF_H_
#define SOURCEMETA_CORE_CRYPTO_HKDF_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Extract a pseudorandom key from input keying material under HMAC-SHA256,
/// the first step of RFC 5869 Section 2.2. The salt is optional and need not
/// be secret, and an empty one stands for the string of digest-length zeros
/// that section substitutes when none is provided. RFC 5869 Section 3.1
/// recommends supplying one. Throws when the underlying provider refuses the
/// extraction, since there is no pseudorandom key to report. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::hkdf_sha256_extract("salt", "secret")};
/// assert(key.size() == 32);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT hkdf_sha256_extract(
    const std::string_view salt, const std::string_view input_key_material)
    -> std::array<std::uint8_t, 32>;

/// @ingroup crypto
/// Expand a pseudorandom key into output keying material of the requested
/// length under HMAC-SHA256, the second step of RFC 5869 Section 2.3. The info
/// input binds the result to a purpose, so one secret yields several keys that
/// cannot be substituted for one another. There is no result when the key is
/// shorter than the digest or when the length exceeds the 255 digests that
/// section permits. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::hkdf_sha256_extract("salt", "secret")};
/// const std::string_view pseudorandom_key{
///     reinterpret_cast<const char *>(key.data()), key.size()};
/// const auto material{
///     sourcemeta::core::hkdf_sha256_expand(pseudorandom_key, "session", 32)};
/// assert(material.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT hkdf_sha256_expand(
    const std::string_view pseudorandom_key, const std::string_view info,
    const std::size_t length) -> std::optional<std::string>;

/// @ingroup crypto
/// Derive output keying material from input keying material under HMAC-SHA256,
/// extracting and then expanding as RFC 5869 Section 2 prescribes. Prefer this
/// over the two steps alone, since Section 3.3 warns that skipping the extract
/// step is only sound when the input is already a cryptographically strong key,
/// and never for a Diffie-Hellman value. There is no result when the length
/// exceeds the 255 digests Section 2.3 permits. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto material{
///     sourcemeta::core::hkdf_sha256("secret", "salt", "session", 32)};
/// assert(material.has_value());
/// assert(material.value().size() == 32);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
hkdf_sha256(const std::string_view input_key_material,
            const std::string_view salt, const std::string_view info,
            const std::size_t length) -> std::optional<std::string>;

/// @ingroup crypto
/// Extract a pseudorandom key from input keying material under HMAC-SHA384,
/// the first step of RFC 5869 Section 2.2. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::hkdf_sha384_extract("salt", "secret")};
/// assert(key.size() == 48);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT hkdf_sha384_extract(
    const std::string_view salt, const std::string_view input_key_material)
    -> std::array<std::uint8_t, 48>;

/// @ingroup crypto
/// Expand a pseudorandom key into output keying material of the requested
/// length under HMAC-SHA384, the second step of RFC 5869 Section 2.3.
auto SOURCEMETA_CORE_CRYPTO_EXPORT hkdf_sha384_expand(
    const std::string_view pseudorandom_key, const std::string_view info,
    const std::size_t length) -> std::optional<std::string>;

/// @ingroup crypto
/// Derive output keying material from input keying material under HMAC-SHA384,
/// extracting and then expanding as RFC 5869 Section 2 prescribes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto material{
///     sourcemeta::core::hkdf_sha384("secret", "salt", "session", 32)};
/// assert(material.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
hkdf_sha384(const std::string_view input_key_material,
            const std::string_view salt, const std::string_view info,
            const std::size_t length) -> std::optional<std::string>;

/// @ingroup crypto
/// Extract a pseudorandom key from input keying material under HMAC-SHA512,
/// the first step of RFC 5869 Section 2.2. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::hkdf_sha512_extract("salt", "secret")};
/// assert(key.size() == 64);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT hkdf_sha512_extract(
    const std::string_view salt, const std::string_view input_key_material)
    -> std::array<std::uint8_t, 64>;

/// @ingroup crypto
/// Expand a pseudorandom key into output keying material of the requested
/// length under HMAC-SHA512, the second step of RFC 5869 Section 2.3.
auto SOURCEMETA_CORE_CRYPTO_EXPORT hkdf_sha512_expand(
    const std::string_view pseudorandom_key, const std::string_view info,
    const std::size_t length) -> std::optional<std::string>;

/// @ingroup crypto
/// Derive output keying material from input keying material under HMAC-SHA512,
/// extracting and then expanding as RFC 5869 Section 2 prescribes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto material{
///     sourcemeta::core::hkdf_sha512("secret", "salt", "session", 32)};
/// assert(material.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
hkdf_sha512(const std::string_view input_key_material,
            const std::string_view salt, const std::string_view info,
            const std::size_t length) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
