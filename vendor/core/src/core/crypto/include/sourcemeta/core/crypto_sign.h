#ifndef SOURCEMETA_CORE_CRYPTO_SIGN_H_
#define SOURCEMETA_CORE_CRYPTO_SIGN_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <sourcemeta/core/crypto_verify.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup crypto
/// A parsed private key that holds the native key, so that the same key can
/// produce many signatures without paying the key construction cost on every
/// call. Build it once with the factory function and pass it to the matching
/// signing function. The signing counterpart to PublicKey. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_private_key(pem)};
/// assert(key.has_value());
/// const auto signature{sourcemeta::core::rsassa_pkcs1_v15_sign(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, message)};
/// assert(signature.has_value());
/// ```
class SOURCEMETA_CORE_CRYPTO_EXPORT PrivateKey {
public:
  /// The kind of key, which fixes the signature schemes it can produce.
  enum class Type : std::uint8_t {
    /// The RSA key type.
    RSA,
    /// The elliptic-curve key type.
    EllipticCurve,
    /// The Edwards-curve key type.
    Edwards
  };

  ~PrivateKey();
  /// Move constructor
  PrivateKey(PrivateKey &&other) noexcept;
  auto operator=(PrivateKey &&other) noexcept -> PrivateKey &;
  PrivateKey(const PrivateKey &) = delete;
  auto operator=(const PrivateKey &) -> PrivateKey & = delete;

  /// The kind of key this is.
  [[nodiscard]] auto type() const noexcept -> Type;

  /// The backend specific parsed key state, defined by each backend
  struct Internal;
  /// Take ownership of a parsed key. Prefer the factory function below
  explicit PrivateKey(Internal *internal) noexcept;
  /// Access the parsed key, which the signing functions read. The type is
  /// opaque, so there is nothing a caller can do with it
  [[nodiscard]] auto internal() const noexcept -> const Internal * {
    return this->internal_;
  }

private:
  Internal *internal_;
};

/// @ingroup crypto
/// Parse a private key from an unencrypted PKCS#8 PEM document (RFC 5958),
/// returning no value when the input is not a supported RSA, elliptic curve, or
/// Edwards private key. Encrypted documents are not supported. An elliptic
/// curve key must carry its public point, which is optional in SEC1 (RFC 5915),
/// because some backends store the point alongside the private scalar.
auto SOURCEMETA_CORE_CRYPTO_EXPORT make_private_key(const std::string_view pem)
    -> std::optional<PrivateKey>;

/// @ingroup crypto
/// Parse an elliptic curve private key from its raw big-endian private scalar
/// and public point coordinates, returning no value when the material is
/// malformed. The public coordinates are required because the platform key
/// backends store the point alongside the scalar.
auto SOURCEMETA_CORE_CRYPTO_EXPORT make_ec_private_key(
    const EllipticCurve curve, const std::string_view scalar,
    const std::string_view coordinate_x, const std::string_view coordinate_y)
    -> std::optional<PrivateKey>;

/// @ingroup crypto
/// Parse an Edwards-curve private key from its raw seed, returning no value
/// when the seed is the wrong length for the curve.
auto SOURCEMETA_CORE_CRYPTO_EXPORT
make_edwards_private_key(const EdwardsCurve curve, const std::string_view seed)
    -> std::optional<PrivateKey>;

/// @ingroup crypto
/// Parse an RSA private key from the components of its two-prime form (RFC 8017
/// Section 3.2), each a raw big-endian integer, in the order modulus, public
/// exponent, private exponent, first prime, second prime, first prime exponent,
/// second prime exponent, and coefficient. Every component is required, as the
/// platform backends import the key from its full private structure rather than
/// recomputing the primes. Returns no value when the material is malformed.
auto SOURCEMETA_CORE_CRYPTO_EXPORT make_rsa_private_key(
    const std::string_view modulus, const std::string_view public_exponent,
    const std::string_view private_exponent, const std::string_view prime1,
    const std::string_view prime2, const std::string_view exponent1,
    const std::string_view exponent2, const std::string_view coefficient)
    -> std::optional<PrivateKey>;

/// @ingroup crypto
/// Produce an RSASSA-PKCS1-v1_5 signature (RFC 8017 Section 8.2.1) over a
/// message, returning no value when the key is not an RSA key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_private_key(pem)};
/// assert(key.has_value());
/// const auto signature{sourcemeta::core::rsassa_pkcs1_v15_sign(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, message)};
/// assert(signature.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT rsassa_pkcs1_v15_sign(
    const PrivateKey &key, const SignatureHashFunction hash,
    const std::string_view message) -> std::optional<std::string>;

/// @ingroup crypto
/// Produce an RSASSA-PSS signature (RFC 8017 Section 8.1.1) over a message,
/// with the salt length fixed to the hash function output as RFC 7518 Section
/// 3.5 requires. Returns no value when the key is not an RSA key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_private_key(pem)};
/// assert(key.has_value());
/// const auto signature{sourcemeta::core::rsassa_pss_sign(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, message)};
/// assert(signature.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT
rsassa_pss_sign(const PrivateKey &key, const SignatureHashFunction hash,
                const std::string_view message) -> std::optional<std::string>;

/// @ingroup crypto
/// Produce an ECDSA signature (FIPS 186-4 Section 6.4) over a message. The
/// signature is the raw concatenation of the two integers, each padded to the
/// curve field width, as JWS mandates (RFC 7518 Section 3.4), matching what
/// ecdsa_verify expects. Returns no value when the key is not an elliptic curve
/// key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_private_key(pem)};
/// assert(key.has_value());
/// const auto signature{sourcemeta::core::ecdsa_sign(
///     key.value(), sourcemeta::core::SignatureHashFunction::SHA256, message)};
/// assert(signature.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT ecdsa_sign(const PrivateKey &key,
                                              const SignatureHashFunction hash,
                                              const std::string_view message)
    -> std::optional<std::string>;

/// @ingroup crypto
/// Produce an EdDSA signature (RFC 8032) over a message. There is no separate
/// hash function, as the curve fixes it. Returns no value when the key is not
/// an Edwards curve key. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::make_private_key(pem)};
/// assert(key.has_value());
/// const auto signature{sourcemeta::core::eddsa_sign(key.value(), message)};
/// assert(signature.has_value());
/// ```
auto SOURCEMETA_CORE_CRYPTO_EXPORT eddsa_sign(const PrivateKey &key,
                                              const std::string_view message)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
