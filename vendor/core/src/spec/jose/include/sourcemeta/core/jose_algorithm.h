#ifndef SOURCEMETA_CORE_JOSE_ALGORITHM_H_
#define SOURCEMETA_CORE_JOSE_ALGORITHM_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// The JSON Web Signature algorithms from RFC 7518 Section 3.1 and the
/// Edwards-curve algorithm from RFC 8037 Section 3.1. The null algorithm is
/// intentionally absent. Each algorithm demands a key of exactly one family,
/// the symmetric algorithms an octet sequence and the asymmetric ones their own
/// key type, which is what keeps algorithm confusion attacks unexploitable.
enum class JWSAlgorithm : std::uint8_t {
  /// RSASSA-PKCS1-v1_5 using SHA-256.
  RS256,
  /// RSASSA-PKCS1-v1_5 using SHA-384.
  RS384,
  /// RSASSA-PKCS1-v1_5 using SHA-512.
  RS512,
  /// RSASSA-PSS using SHA-256 and MGF1 with SHA-256.
  PS256,
  /// RSASSA-PSS using SHA-384 and MGF1 with SHA-384.
  PS384,
  /// RSASSA-PSS using SHA-512 and MGF1 with SHA-512.
  PS512,
  /// ECDSA using the NIST P-256 curve and SHA-256.
  ES256,
  /// ECDSA using the NIST P-384 curve and SHA-384.
  ES384,
  /// ECDSA using the NIST P-521 curve and SHA-512.
  ES512,
  /// Edwards-curve Digital Signature Algorithm.
  EdDSA,
  /// HMAC using SHA-256.
  HS256,
  /// HMAC using SHA-384.
  HS384,
  /// HMAC using SHA-512.
  HS512
};

/// @ingroup jose
/// Map a JSON Web Signature `alg` value to its algorithm, returning no value
/// for any unrecognized name. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_jws_algorithm("RS256").has_value());
/// assert(!sourcemeta::core::to_jws_algorithm("none").has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto to_jws_algorithm(const std::string_view value) noexcept
    -> std::optional<JWSAlgorithm>;

/// @ingroup jose
/// Map a JSON Web Signature algorithm to its `alg` value, the inverse of
/// parsing (RFC 7515 Section 4.1.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jws_algorithm_name(
///            sourcemeta::core::JWSAlgorithm::ES256) == "ES256");
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jws_algorithm_name(const JWSAlgorithm algorithm) noexcept
    -> std::string_view;

/// @ingroup jose
/// Whether an algorithm is an asymmetric digital signature algorithm rather
/// than a symmetric message authentication code (RFC 7518 Section 3.1). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jws_algorithm_is_asymmetric(
///            sourcemeta::core::JWSAlgorithm::ES256));
/// assert(!sourcemeta::core::jws_algorithm_is_asymmetric(
///            sourcemeta::core::JWSAlgorithm::HS256));
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jws_algorithm_is_asymmetric(const JWSAlgorithm algorithm) noexcept -> bool;

/// @ingroup jose
/// The size, in bits, of the digest each signature algorithm is defined over
/// (RFC 7518 Section 3.1), so that derived hash claims such as OpenID
/// Connect's `at_hash` and `c_hash` select their digest by table rather than
/// by slicing the algorithm name. The Edwards-curve algorithm names no digest
/// and OpenID Connect leaves the choice unspecified, so it is pinned to
/// SHA-512, the convention deployed implementations follow for Ed25519, whose
/// signature scheme is internally defined over that hash (RFC 8032 Section
/// 5.1). Hash claims minted over Ed448 by a SHAKE256-based implementation
/// need curve-specific handling that no algorithm-only table can provide. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jws_algorithm_digest_bits(
///            sourcemeta::core::JWSAlgorithm::ES384) == 384);
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jws_algorithm_digest_bits(const JWSAlgorithm algorithm) noexcept
    -> std::uint16_t;

/// @ingroup jose
/// The JSON Web Encryption key-management algorithms (the `alg` value) from RFC
/// 7518 Section 4.1. Each one determines how the content encryption key is
/// delivered to the recipient, and demands a key of exactly one family, which
/// is what keeps algorithm confusion attacks unexploitable.
enum class JWEAlgorithm : std::uint8_t {
  /// RSAES OAEP using default parameters (SHA-1).
  RSA_OAEP,
  /// RSAES OAEP using SHA-256.
  RSA_OAEP_256,
  /// Elliptic Curve Diffie-Hellman Ephemeral Static key agreement, direct.
  ECDH_ES,
  /// ECDH-ES using Concat KDF and the CEK wrapped with AES-128 Key Wrap.
  ECDH_ES_A128KW,
  /// ECDH-ES using Concat KDF and the CEK wrapped with AES-192 Key Wrap.
  ECDH_ES_A192KW,
  /// ECDH-ES using Concat KDF and the CEK wrapped with AES-256 Key Wrap.
  ECDH_ES_A256KW,
  /// AES-128 Key Wrap.
  A128KW,
  /// AES-192 Key Wrap.
  A192KW,
  /// AES-256 Key Wrap.
  A256KW,
  /// Direct use of a shared symmetric key as the CEK.
  DIR
};

/// @ingroup jose
/// The JSON Web Encryption content encryption algorithms (the `enc` value) from
/// RFC 7518 Section 5.1. Each one seals the plaintext under the content
/// encryption key with an authenticated encryption scheme.
enum class JWEEncryption : std::uint8_t {
  /// AES-128 in Galois/Counter Mode.
  A128GCM,
  /// AES-192 in Galois/Counter Mode.
  A192GCM,
  /// AES-256 in Galois/Counter Mode.
  A256GCM,
  /// AES-128 in CBC mode with an HMAC SHA-256 authentication tag.
  A128CBC_HS256,
  /// AES-192 in CBC mode with an HMAC SHA-384 authentication tag.
  A192CBC_HS384,
  /// AES-256 in CBC mode with an HMAC SHA-512 authentication tag.
  A256CBC_HS512
};

/// @ingroup jose
/// Map a JSON Web Encryption `alg` value to its algorithm, returning no value
/// for any unrecognized name. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_jwe_algorithm("RSA-OAEP-256").has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto to_jwe_algorithm(const std::string_view value) noexcept
    -> std::optional<JWEAlgorithm>;

/// @ingroup jose
/// Map a JSON Web Encryption key-management algorithm to its `alg` value, the
/// inverse of parsing (RFC 7518 Section 4.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jwe_algorithm_name(
///            sourcemeta::core::JWEAlgorithm::DIR) == "dir");
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwe_algorithm_name(const JWEAlgorithm algorithm) noexcept
    -> std::string_view;

/// @ingroup jose
/// Map a JSON Web Encryption `enc` value to its algorithm, returning no value
/// for any unrecognized name. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_jwe_encryption("A128GCM").has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto to_jwe_encryption(const std::string_view value) noexcept
    -> std::optional<JWEEncryption>;

/// @ingroup jose
/// Map a JSON Web Encryption content encryption algorithm to its `enc` value,
/// the inverse of parsing (RFC 7518 Section 5.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jwe_encryption_name(
///            sourcemeta::core::JWEEncryption::A128GCM) == "A128GCM");
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwe_encryption_name(const JWEEncryption encryption) noexcept
    -> std::string_view;

/// @ingroup jose
/// Whether the key-management algorithm consumes an asymmetric recipient key
/// (`RSA-OAEP*`, `ECDH-ES*`) rather than a shared octet secret (`A*KW`, `dir`)
/// (RFC 7518 Section 4.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jwe_algorithm_is_asymmetric(
///            sourcemeta::core::JWEAlgorithm::RSA_OAEP));
/// assert(!sourcemeta::core::jwe_algorithm_is_asymmetric(
///            sourcemeta::core::JWEAlgorithm::A128KW));
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwe_algorithm_is_asymmetric(const JWEAlgorithm algorithm) noexcept -> bool;

/// @ingroup jose
/// The content encryption key length, in bytes, that a content encryption
/// algorithm requires (RFC 7518 Sections 5.2 and 5.3). The CBC-HMAC algorithms
/// take a double-width key that is split into a MAC key and an encryption key.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::jwe_encryption_key_bytes(
///            sourcemeta::core::JWEEncryption::A128GCM) == 16);
/// assert(sourcemeta::core::jwe_encryption_key_bytes(
///            sourcemeta::core::JWEEncryption::A128CBC_HS256) == 32);
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwe_encryption_key_bytes(const JWEEncryption encryption) noexcept
    -> std::size_t;

} // namespace sourcemeta::core

#endif
