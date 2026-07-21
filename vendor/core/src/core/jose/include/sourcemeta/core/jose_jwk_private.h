#ifndef SOURCEMETA_CORE_JOSE_JWK_PRIVATE_H_
#define SOURCEMETA_CORE_JOSE_JWK_PRIVATE_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <sourcemeta/core/jose_algorithm.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// A parsed private JSON Web Key (RFC 7517), the private counterpart to a
/// public one, restricted to RSA, elliptic curve, octet key pair (RFC 8037),
/// and symmetric octet (RFC 7518 Section 6.4) keys. The key owns its decoded
/// material, so neither the source JSON document nor the source PEM needs to
/// outlive it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::JWKPrivate::from_pem(pem)};
/// assert(key.has_value());
/// assert(key.value().private_key() != nullptr);
/// ```
class SOURCEMETA_CORE_JOSE_EXPORT JWKPrivate {
public:
  /// The family of key material a key holds.
  enum class Type : std::uint8_t {
    /// The RSA key type.
    RSA,
    /// The elliptic-curve key type.
    EllipticCurve,
    /// The Edwards-curve octet key pair type.
    OctetKeyPair,
    /// The symmetric octet sequence key type.
    Octet
  };

  /// A key exclusively owns its parsed private key, so it is move-only.
  JWKPrivate(JWKPrivate &&other) noexcept = default;
  auto operator=(JWKPrivate &&other) noexcept -> JWKPrivate & = default;
  JWKPrivate(const JWKPrivate &) = delete;
  auto operator=(const JWKPrivate &) -> JWKPrivate & = delete;
  ~JWKPrivate() = default;

  /// Parse a private JSON Web Key from a JSON value, returning no value on
  /// invalid input. The private parameters (RFC 7518 Sections 6.2.2 and 6.3.2,
  /// RFC 8037 Section 2) are required, so a public-only key is rejected.
  [[nodiscard]] static auto from(const JSON &value)
      -> std::optional<JWKPrivate>;

  /// Parse a private JSON Web Key from a JSON value, returning no value on
  /// invalid input.
  [[nodiscard]] static auto from(JSON &&value) -> std::optional<JWKPrivate>;

  /// Parse a private key from an unencrypted PKCS#8 PEM document (RFC 5958),
  /// returning no value on invalid input. The key identifier and algorithm are
  /// left unset, as a PEM document carries no such metadata.
  [[nodiscard]] static auto from_pem(const std::string_view pem)
      -> std::optional<JWKPrivate>;

  /// The family of key material this key holds.
  [[nodiscard]] auto type() const noexcept -> Type { return this->type_; }

  /// The key identifier used to select this key, if present.
  [[nodiscard]] auto key_id() const noexcept
      -> std::optional<std::string_view> {
    if (this->key_id_.has_value()) {
      return std::string_view{*this->key_id_};
    }

    return std::nullopt;
  }

  /// The algorithm this key is intended for, if present.
  [[nodiscard]] auto algorithm() const noexcept -> std::optional<JWSAlgorithm> {
    return this->algorithm_;
  }

  // Elliptic curve keys (RFC 7518 Section 6.2) and octet key pairs (RFC 8037
  // Section 2) carry a curve name, which the elliptic curve algorithms pin to
  // exactly one curve. A key parsed from a PEM document carries it once its
  // public part has been recovered, and it stays empty for a key that carries
  // no curve, such as an RSA or symmetric key
  /// The curve this key is pinned to, empty when it carries none.
  [[nodiscard]] auto curve() const noexcept -> std::string_view {
    return this->curve_;
  }

  // The parsed platform key, built once from the decoded material so that
  // signing reuses it rather than reconstructing it per signature. It is null
  // when the material could not be turned into a key
  /// The parsed private key, or null when the material could not be decoded.
  [[nodiscard]] auto private_key() const noexcept -> const PrivateKey * {
    return this->private_key_.has_value() ? &*this->private_key_ : nullptr;
  }

  // Symmetric keys carry their raw secret rather than a parsed platform key
  // (RFC 7518 Section 6.4), and knowing the secret is required for both
  // producing and checking an HMAC, so the private key class carries it
  /// The raw symmetric secret, empty for asymmetric keys.
  [[nodiscard]] auto secret() const noexcept -> std::string_view {
    return this->secret_;
  }

  /// Serialize the public part of this key as a JSON Web Key (RFC 7517),
  /// returning no value for a symmetric key, which has no public form, or for a
  /// key parsed from a PEM document whose public part could not be recovered.
  [[nodiscard]] auto public_jwk() const -> std::optional<JSON>;

  /// The SHA-256 JSON Web Key thumbprint of this key (RFC 7638),
  /// base64url-encoded, returning no value for a key whose public part could
  /// not be recovered.
  [[nodiscard]] auto thumbprint() const -> std::optional<std::string>;

private:
  JWKPrivate() = default;
  static auto parse(const JSON &value, JWKPrivate &result) -> bool;

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  Type type_{Type::RSA};
  std::optional<std::string> key_id_;
  std::optional<JWSAlgorithm> algorithm_;
  std::string curve_;
  std::optional<PrivateKey> private_key_;
  std::string secret_;
  std::string modulus_;
  std::string exponent_;
  std::string coordinate_x_;
  std::string coordinate_y_;
  std::string public_point_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
