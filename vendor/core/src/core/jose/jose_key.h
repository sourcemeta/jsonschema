#ifndef SOURCEMETA_CORE_JOSE_KEY_H_
#define SOURCEMETA_CORE_JOSE_KEY_H_

// Curve and algorithm mappings shared by the public JWK and the private
// JWKPrivate parsing paths

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose_algorithm.h>

#include <bit>         // std::countl_zero
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

// The kind of key, shared by the public and private parsing paths so that the
// algorithm matching does not need to be written twice
enum class JWKKind : std::uint8_t { RSA, EllipticCurve, OctetKeyPair, Octet };

// RFC 7518 Section 3.3: "A key of size 2048 bits or larger MUST be used with"
// the RSASSA algorithms, so a smaller modulus is refused before a key is built
inline constexpr std::size_t MINIMUM_RSA_MODULUS_BITS{2048};

inline auto jwk_rsa_modulus_is_allowed(const std::string_view modulus) noexcept
    -> bool {
  std::size_t offset{0};
  while (offset < modulus.size() &&
         static_cast<std::uint8_t>(modulus[offset]) == 0) {
    offset += 1;
  }

  if (offset >= modulus.size()) {
    return false;
  }

  const auto leading{static_cast<std::uint8_t>(modulus[offset])};
  const auto bits{(modulus.size() - offset - 1) * 8 +
                  (8 - static_cast<std::size_t>(std::countl_zero(leading)))};
  return bits >= MINIMUM_RSA_MODULUS_BITS;
}

// The coordinate octet length is fixed per curve (RFC 7518 Section 6.2.1.2)
inline auto jwk_ec_coordinate_bytes(const std::string_view curve)
    -> std::optional<std::size_t> {
  if (curve == "P-256") {
    return 32;
  } else if (curve == "P-384") {
    return 48;
  } else if (curve == "P-521") {
    return 66;
  } else {
    return std::nullopt;
  }
}

// The key octet length is fixed per Edwards curve (RFC 8032 Sections 5.1.5 and
// 5.2.5), and the private seed shares it (RFC 8037 Section 2)
inline auto jwk_okp_key_bytes(const std::string_view curve)
    -> std::optional<std::size_t> {
  if (curve == "Ed25519") {
    return 32;
  } else if (curve == "Ed448") {
    return 57;
  } else {
    return std::nullopt;
  }
}

// Both mappings are only reached after the curve has been validated above
inline auto jwk_to_elliptic_curve(const std::string_view curve) noexcept
    -> EllipticCurve {
  if (curve == "P-256") {
    return EllipticCurve::P256;
  } else if (curve == "P-384") {
    return EllipticCurve::P384;
  } else {
    return EllipticCurve::P521;
  }
}

inline auto jwk_to_edwards_curve(const std::string_view curve) noexcept
    -> EdwardsCurve {
  if (curve == "Ed25519") {
    return EdwardsCurve::Ed25519;
  } else {
    return EdwardsCurve::Ed448;
  }
}

// The RSA algorithms only require an RSA key, each ECDSA algorithm is tied to a
// specific curve (RFC 7518 Section 3.1), and the Edwards-curve algorithm
// requires an octet key pair of either curve (RFC 8037 Section 3.1)
inline auto jwk_algorithm_matches_key(const JWSAlgorithm algorithm,
                                      const JWKKind kind,
                                      const std::string_view curve) -> bool {
  switch (algorithm) {
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::RS512:
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::PS512:
      return kind == JWKKind::RSA;
    case JWSAlgorithm::ES256:
      return kind == JWKKind::EllipticCurve && curve == "P-256";
    case JWSAlgorithm::ES384:
      return kind == JWKKind::EllipticCurve && curve == "P-384";
    case JWSAlgorithm::ES512:
      return kind == JWKKind::EllipticCurve && curve == "P-521";
    case JWSAlgorithm::EdDSA:
      return kind == JWKKind::OctetKeyPair;
    case JWSAlgorithm::HS256:
    case JWSAlgorithm::HS384:
    case JWSAlgorithm::HS512:
      return kind == JWKKind::Octet;
  }

  std::unreachable();
}

// The hash function each RSA and ECDSA algorithm computes over the signing
// input (RFC 7518 Section 3.1). The Edwards-curve algorithm fixes its own hash,
// so it never reaches here
inline auto jws_hash_for(const JWSAlgorithm algorithm)
    -> SignatureHashFunction {
  switch (algorithm) {
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::ES256:
      return SignatureHashFunction::SHA256;
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::ES384:
      return SignatureHashFunction::SHA384;
    case JWSAlgorithm::RS512:
    case JWSAlgorithm::PS512:
    case JWSAlgorithm::ES512:
      return SignatureHashFunction::SHA512;
    case JWSAlgorithm::EdDSA:
    case JWSAlgorithm::HS256:
    case JWSAlgorithm::HS384:
    case JWSAlgorithm::HS512:
      break;
  }

  std::unreachable();
}

// RFC 7518 Section 3.2: "A key of the same size as the hash output (for
// instance, 256 bits for HS256) or larger MUST be used with this algorithm"
inline auto jws_hmac_minimum_secret_bytes(const JWSAlgorithm algorithm)
    -> std::size_t {
  switch (algorithm) {
    case JWSAlgorithm::HS256:
      return 32;
    case JWSAlgorithm::HS384:
      return 48;
    case JWSAlgorithm::HS512:
      return 64;
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::RS512:
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::PS512:
    case JWSAlgorithm::ES256:
    case JWSAlgorithm::ES384:
    case JWSAlgorithm::ES512:
    case JWSAlgorithm::EdDSA:
      break;
  }

  std::unreachable();
}

// The raw ECDSA signature length for each algorithm, twice the curve field
// width (RFC 7518 Section 3.4). Since each algorithm is pinned to one curve,
// this length identifies the curve the key must use, which the other
// algorithms do not need
inline auto jws_ecdsa_signature_bytes(const JWSAlgorithm algorithm)
    -> std::size_t {
  switch (algorithm) {
    case JWSAlgorithm::ES256:
      return 64;
    case JWSAlgorithm::ES384:
      return 96;
    case JWSAlgorithm::ES512:
      return 132;
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::RS512:
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::PS512:
    case JWSAlgorithm::EdDSA:
    case JWSAlgorithm::HS256:
    case JWSAlgorithm::HS384:
    case JWSAlgorithm::HS512:
      break;
  }

  std::unreachable();
}

} // namespace sourcemeta::core

#endif
