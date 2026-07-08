#include <sourcemeta/core/jose_verify.h>

#include <sourcemeta/core/crypto.h>

#include "jose_key.h"

#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto jws_verify_signature(const std::optional<JWSAlgorithm> algorithm,
                          const std::string_view signing_input,
                          const std::string_view signature, const JWK &key)
    -> bool {
  if (!algorithm.has_value()) {
    return false;
  }

  // A key that names an algorithm must not contradict the one in use (RFC 7517
  // Section 4.4)
  if (key.algorithm().has_value() &&
      key.algorithm().value() != algorithm.value()) {
    return false;
  }

  const auto *public_key{key.public_key()};

  switch (algorithm.value()) {
    // The asymmetric key material is parsed into a reusable platform key when
    // the key is constructed, so an absent one is material that never formed a
    // valid key
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::RS512:
      return public_key != nullptr && key.type() == JWK::Type::RSA &&
             rsassa_pkcs1_v15_verify(*public_key,
                                     jws_hash_for(algorithm.value()),
                                     signing_input, signature);
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::PS512:
      return public_key != nullptr && key.type() == JWK::Type::RSA &&
             rsassa_pss_verify(*public_key, jws_hash_for(algorithm.value()),
                               signing_input, signature);
    // Each ECDSA algorithm is pinned to exactly one curve (RFC 7518 Section
    // 3.4), so the key's curve is checked independently of any algorithm it
    // declares
    case JWSAlgorithm::ES256:
      return public_key != nullptr && key.type() == JWK::Type::EllipticCurve &&
             key.curve() == "P-256" &&
             ecdsa_verify(*public_key, SignatureHashFunction::SHA256,
                          signing_input, signature);
    case JWSAlgorithm::ES384:
      return public_key != nullptr && key.type() == JWK::Type::EllipticCurve &&
             key.curve() == "P-384" &&
             ecdsa_verify(*public_key, SignatureHashFunction::SHA384,
                          signing_input, signature);
    case JWSAlgorithm::ES512:
      return public_key != nullptr && key.type() == JWK::Type::EllipticCurve &&
             key.curve() == "P-521" &&
             ecdsa_verify(*public_key, SignatureHashFunction::SHA512,
                          signing_input, signature);
    // The Edwards-curve algorithm names one of two curves through the key
    // rather than the algorithm (RFC 8037 Section 3.1), and the key fixes the
    // curve when it is parsed
    case JWSAlgorithm::EdDSA:
      return public_key != nullptr && key.type() == JWK::Type::OctetKeyPair &&
             eddsa_verify(*public_key, signing_input, signature);
    // The symmetric algorithms recompute the tag and compare it in constant
    // time (RFC 7518 Section 3.2), and the secret must be at least as large as
    // the hash output
    case JWSAlgorithm::HS256: {
      if (key.type() != JWK::Type::Octet ||
          key.secret().size() <
              jws_hmac_minimum_secret_bytes(algorithm.value())) {
        return false;
      }

      const auto digest{hmac_sha256_digest(key.secret(), signing_input)};
      return secure_equals(
          {reinterpret_cast<const char *>(digest.data()), digest.size()},
          signature);
    }
    case JWSAlgorithm::HS384: {
      if (key.type() != JWK::Type::Octet ||
          key.secret().size() <
              jws_hmac_minimum_secret_bytes(algorithm.value())) {
        return false;
      }

      const auto digest{hmac_sha384_digest(key.secret(), signing_input)};
      return secure_equals(
          {reinterpret_cast<const char *>(digest.data()), digest.size()},
          signature);
    }
    case JWSAlgorithm::HS512: {
      if (key.type() != JWK::Type::Octet ||
          key.secret().size() <
              jws_hmac_minimum_secret_bytes(algorithm.value())) {
        return false;
      }

      const auto digest{hmac_sha512_digest(key.secret(), signing_input)};
      return secure_equals(
          {reinterpret_cast<const char *>(digest.data()), digest.size()},
          signature);
    }
  }

  std::unreachable();
}

} // namespace sourcemeta::core
