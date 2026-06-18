#ifndef SOURCEMETA_CORE_CRYPTO_VERIFY_H_
#define SOURCEMETA_CORE_CRYPTO_VERIFY_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// The hash functions supported by signature verification.
enum class SignatureHashFunction : std::uint8_t { SHA256, SHA384, SHA512 };

/// @ingroup crypto
/// The NIST elliptic curves supported by signature verification.
enum class EllipticCurve : std::uint8_t { P256, P384, P521 };

/// @ingroup crypto
/// The Edwards curves supported by signature verification.
enum class EdwardsCurve : std::uint8_t { Ed25519, Ed448 };

/// @ingroup crypto
/// A parsed public key that holds the native key, so that the same key can
/// verify many signatures without paying the key construction cost on every
/// call. Build it once with one of the factory functions and pass it to the
/// matching verification function. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_rsa_public_key(modulus, exponent)};
/// assert(key.has_value());
/// assert(sourcemeta::core::rsassa_pkcs1_v15_verify(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, message,
///     signature));
/// ```
class SOURCEMETA_CORE_CRYPTO_EXPORT PublicKey {
public:
  /// The kind of key, which fixes the signature schemes it can verify.
  enum class Type : std::uint8_t { RSA, EllipticCurve, Edwards };

  ~PublicKey();
  PublicKey(PublicKey &&other) noexcept;
  auto operator=(PublicKey &&other) noexcept -> PublicKey &;
  PublicKey(const PublicKey &) = delete;
  auto operator=(const PublicKey &) -> PublicKey & = delete;

  /// The kind of key this is.
  [[nodiscard]] auto type() const noexcept -> Type;

  /// The backend specific parsed key state, defined by each backend
  struct Internal;
  /// Take ownership of a parsed key. Prefer the factory functions below
  explicit PublicKey(Internal *internal) noexcept;
  /// Access the parsed key, which the verification functions read. The type is
  /// opaque, so there is nothing a caller can do with it
  [[nodiscard]] auto internal() const noexcept -> const Internal * {
    return this->internal_;
  }

private:
  Internal *internal_;
};

/// @ingroup crypto
/// Parse an RSA public key from its raw big-endian modulus and exponent bytes,
/// returning no value when the material is malformed or beyond 4096 bits.
auto SOURCEMETA_CORE_CRYPTO_EXPORT make_rsa_public_key(
    const std::string_view modulus, const std::string_view exponent)
    -> std::optional<PublicKey>;

/// @ingroup crypto
/// Parse an elliptic curve public key from its raw big-endian point
/// coordinates, returning no value when the point is malformed.
auto SOURCEMETA_CORE_CRYPTO_EXPORT make_ec_public_key(
    const EllipticCurve curve, const std::string_view coordinate_x,
    const std::string_view coordinate_y) -> std::optional<PublicKey>;

/// @ingroup crypto
/// Parse an Edwards-curve public key from its raw encoded point, returning no
/// value when the key is malformed or the wrong length for the curve.
auto SOURCEMETA_CORE_CRYPTO_EXPORT make_eddsa_public_key(
    const EdwardsCurve curve, const std::string_view public_key)
    -> std::optional<PublicKey>;

/// @ingroup crypto
/// Verify an RSASSA-PKCS1-v1_5 signature (RFC 8017 Section 8.2.2) over a
/// message with the given RSA key. The signature is invalid rather than an
/// error if it is malformed or the key is not an RSA key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_rsa_public_key(modulus, exponent)};
/// assert(key.has_value());
/// assert(!sourcemeta::core::rsassa_pkcs1_v15_verify(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, "message",
///     "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsassa_pkcs1_v15_verify(
    const PublicKey &key, const SignatureHashFunction hash,
    const std::string_view message, const std::string_view signature) -> bool;

/// @ingroup crypto
/// Verify an RSASSA-PSS signature (RFC 8017 Section 8.1.2) over a message with
/// the given RSA key. The salt is expected to be as long as the hash function
/// output, as RFC 7518 requires, and signatures carrying any other salt length
/// are invalid, as are signatures verified against a non-RSA key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_rsa_public_key(modulus, exponent)};
/// assert(key.has_value());
/// assert(!sourcemeta::core::rsassa_pss_verify(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, "message",
///     "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsassa_pss_verify(
    const PublicKey &key, const SignatureHashFunction hash,
    const std::string_view message, const std::string_view signature) -> bool;

/// @ingroup crypto
/// Verify an ECDSA signature (FIPS 186-4 Section 6.4) over a message with the
/// given elliptic curve key. The signature is the raw concatenation of the two
/// integers, each padded to the curve field width, as JWS mandates (RFC 7518
/// Section 3.4). The signature is invalid rather than an error if it is
/// malformed or the key is not an elliptic curve key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_ec_public_key(
///     sourcemeta::core::EllipticCurve::P256, x, y)};
/// assert(key.has_value());
/// assert(!sourcemeta::core::ecdsa_verify(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, "message",
///     "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT ecdsa_verify(
    const PublicKey &key, const SignatureHashFunction hash,
    const std::string_view message, const std::string_view signature) -> bool;

/// @ingroup crypto
/// Verify an EdDSA signature (RFC 8032) over a message with the given Edwards
/// curve key. There is no separate hash function, as the curve fixes it. The
/// signature is invalid rather than an error if it is malformed or the key is
/// not an Edwards curve key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_eddsa_public_key(
///     sourcemeta::core::EdwardsCurve::Ed25519, public_key)};
/// assert(key.has_value());
/// assert(!sourcemeta::core::eddsa_verify(key.value(), "message",
/// "signature"));
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
eddsa_verify(const PublicKey &key, const std::string_view message,
             const std::string_view signature) -> bool;

} // namespace sourcemeta::core

#endif
