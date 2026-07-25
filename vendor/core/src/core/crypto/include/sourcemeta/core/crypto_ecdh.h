#ifndef SOURCEMETA_CORE_CRYPTO_ECDH_H_
#define SOURCEMETA_CORE_CRYPTO_ECDH_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/crypto_verify.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// Compute the raw Elliptic Curve Diffie-Hellman shared secret from a private
/// key and a peer public key on the same curve, returning the agreed point's x
/// coordinate as fixed-length big-endian bytes. Returns no value when the keys
/// are not both elliptic curve keys on the same curve, or the peer point is not
/// on the curve. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto secret{sourcemeta::core::ecdh_derive(private_key, peer_key)};
/// assert(secret.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT ecdh_derive(const PrivateKey &private_key,
                                               const PublicKey &public_key)
    -> std::optional<std::string>;

/// @ingroup crypto
/// Derive a key from a shared secret with the Concat KDF (RFC 7518
/// Section 4.6), the single-step key derivation function over SHA-256, where
/// the algorithm identifier and the two party information strings are the
/// already decoded bytes that make up the derivation context. Returns no value
/// when the context or the requested key length exceeds the 32-bit fields the
/// construction encodes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::kdf_concat(secret, "A128GCM", "Alice",
///                                             "Bob", 16)};
/// assert(key.has_value());
/// assert(key.value().size() == 16);
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT kdf_concat(
    const std::string_view shared_secret, const std::string_view algorithm_id,
    const std::string_view party_u_info, const std::string_view party_v_info,
    const std::size_t derived_key_bytes) -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
