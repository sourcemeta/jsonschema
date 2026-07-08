#include <sourcemeta/core/jose_sign.h>

#include <sourcemeta/core/crypto.h>

#include "jose_key.h"

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto jws_sign(const JWSAlgorithm algorithm,
              const std::string_view signing_input, const JWKPrivate &key)
    -> std::optional<std::string> {
  // A key that names an algorithm must not contradict the one in use (RFC 7517
  // Section 4.4)
  if (key.algorithm().has_value() && key.algorithm().value() != algorithm) {
    return std::nullopt;
  }

  const auto *private_key{key.private_key()};

  switch (algorithm) {
    // The asymmetric key material is parsed into a reusable platform key when
    // the key is constructed, so an absent one is material that never formed a
    // valid key
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::RS512:
      if (private_key == nullptr || key.type() != JWKPrivate::Type::RSA) {
        return std::nullopt;
      }

      return rsassa_pkcs1_v15_sign(*private_key, jws_hash_for(algorithm),
                                   signing_input);
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::PS512:
      if (private_key == nullptr || key.type() != JWKPrivate::Type::RSA) {
        return std::nullopt;
      }

      return rsassa_pss_sign(*private_key, jws_hash_for(algorithm),
                             signing_input);
    // Each ECDSA algorithm is pinned to exactly one curve (RFC 7518 Section
    // 3.4). The raw signature width is twice the curve field width, so it pins
    // the key's curve to the algorithm, including for a key parsed from PEM
    // which carries no curve name
    case JWSAlgorithm::ES256:
    case JWSAlgorithm::ES384:
    case JWSAlgorithm::ES512: {
      if (private_key == nullptr ||
          key.type() != JWKPrivate::Type::EllipticCurve) {
        return std::nullopt;
      }

      auto signature{
          ecdsa_sign(*private_key, jws_hash_for(algorithm), signing_input)};
      if (!signature.has_value() ||
          signature.value().size() != jws_ecdsa_signature_bytes(algorithm)) {
        return std::nullopt;
      }

      return signature;
    }
    // The Edwards-curve algorithm names one of two curves through the key
    // rather than the algorithm (RFC 8037 Section 3.1), and the key fixes the
    // curve when it is parsed
    case JWSAlgorithm::EdDSA:
      if (private_key == nullptr ||
          key.type() != JWKPrivate::Type::OctetKeyPair) {
        return std::nullopt;
      }

      return eddsa_sign(*private_key, signing_input);
    // The symmetric algorithms authenticate with the raw secret rather than a
    // parsed platform key, and the secret must be at least as large as the hash
    // output (RFC 7518 Section 3.2)
    case JWSAlgorithm::HS256: {
      if (key.type() != JWKPrivate::Type::Octet ||
          key.secret().size() < jws_hmac_minimum_secret_bytes(algorithm)) {
        return std::nullopt;
      }

      const auto digest{hmac_sha256_digest(key.secret(), signing_input)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         digest.size()};
    }
    case JWSAlgorithm::HS384: {
      if (key.type() != JWKPrivate::Type::Octet ||
          key.secret().size() < jws_hmac_minimum_secret_bytes(algorithm)) {
        return std::nullopt;
      }

      const auto digest{hmac_sha384_digest(key.secret(), signing_input)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         digest.size()};
    }
    case JWSAlgorithm::HS512: {
      if (key.type() != JWKPrivate::Type::Octet ||
          key.secret().size() < jws_hmac_minimum_secret_bytes(algorithm)) {
        return std::nullopt;
      }

      const auto digest{hmac_sha512_digest(key.secret(), signing_input)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         digest.size()};
    }
  }

  std::unreachable();
}

} // namespace sourcemeta::core
