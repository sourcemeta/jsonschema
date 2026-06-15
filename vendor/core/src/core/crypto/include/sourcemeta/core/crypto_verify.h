#ifndef SOURCEMETA_CORE_CRYPTO_VERIFY_H_
#define SOURCEMETA_CORE_CRYPTO_VERIFY_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// The hash functions supported by signature verification.
enum class SignatureHashFunction : std::uint8_t { SHA256, SHA384, SHA512 };

/// @ingroup crypto
/// The NIST elliptic curves supported by signature verification.
enum class EllipticCurve : std::uint8_t { P256, P384, P521 };

/// @ingroup crypto
/// Verify an RSASSA-PKCS1-v1_5 signature (RFC 8017 Section 8.2.2) over a
/// message, given the public key as raw big-endian modulus and exponent
/// bytes. The signature is invalid rather than an error if any input is
/// malformed, including keys beyond 4096 bits. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// assert(!sourcemeta::core::rsassa_pkcs1_v15_verify(
///     sourcemeta::core::SignatureHashFunction::SHA256,
///     "", "", "message", "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsassa_pkcs1_v15_verify(
    const SignatureHashFunction hash, const std::string_view modulus,
    const std::string_view exponent, const std::string_view message,
    const std::string_view signature) -> bool;

/// @ingroup crypto
/// Verify an RSASSA-PSS signature (RFC 8017 Section 8.1.2) over a message,
/// given the public key as raw big-endian modulus and exponent bytes. The
/// salt is expected to be as long as the hash function output, as RFC 7518
/// requires, and signatures carrying any other salt length are invalid, as
/// are keys beyond 4096 bits. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// assert(!sourcemeta::core::rsassa_pss_verify(
///     sourcemeta::core::SignatureHashFunction::SHA256,
///     "", "", "message", "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsassa_pss_verify(
    const SignatureHashFunction hash, const std::string_view modulus,
    const std::string_view exponent, const std::string_view message,
    const std::string_view signature) -> bool;

/// @ingroup crypto
/// Verify an ECDSA signature (FIPS 186-4 Section 6.4) over a message, given
/// the public key as raw big-endian point coordinates. The signature is the
/// raw concatenation of the two integers, each padded to the curve field
/// width, as JWS mandates (RFC 7518 Section 3.4). The signature is invalid
/// rather than an error if any input is malformed or the point is not on the
/// curve. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// assert(!sourcemeta::core::ecdsa_verify(
///     sourcemeta::core::EllipticCurve::P256,
///     sourcemeta::core::SignatureHashFunction::SHA256,
///     "", "", "message", "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT ecdsa_verify(
    const EllipticCurve curve, const SignatureHashFunction hash,
    const std::string_view coordinate_x, const std::string_view coordinate_y,
    const std::string_view message, const std::string_view signature) -> bool;

} // namespace sourcemeta::core

#endif
