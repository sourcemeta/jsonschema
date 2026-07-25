#include <sourcemeta/core/jose_algorithm.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto to_jws_algorithm(const std::string_view value) noexcept
    -> std::optional<JWSAlgorithm> {
  if (value == "RS256") {
    return JWSAlgorithm::RS256;
  } else if (value == "RS384") {
    return JWSAlgorithm::RS384;
  } else if (value == "RS512") {
    return JWSAlgorithm::RS512;
  } else if (value == "PS256") {
    return JWSAlgorithm::PS256;
  } else if (value == "PS384") {
    return JWSAlgorithm::PS384;
  } else if (value == "PS512") {
    return JWSAlgorithm::PS512;
  } else if (value == "ES256") {
    return JWSAlgorithm::ES256;
  } else if (value == "ES384") {
    return JWSAlgorithm::ES384;
  } else if (value == "ES512") {
    return JWSAlgorithm::ES512;
  } else if (value == "EdDSA") {
    return JWSAlgorithm::EdDSA;
  } else if (value == "HS256") {
    return JWSAlgorithm::HS256;
  } else if (value == "HS384") {
    return JWSAlgorithm::HS384;
  } else if (value == "HS512") {
    return JWSAlgorithm::HS512;
  } else {
    return std::nullopt;
  }
}

auto jws_algorithm_name(const JWSAlgorithm algorithm) noexcept
    -> std::string_view {
  switch (algorithm) {
    case JWSAlgorithm::RS256:
      return "RS256";
    case JWSAlgorithm::RS384:
      return "RS384";
    case JWSAlgorithm::RS512:
      return "RS512";
    case JWSAlgorithm::PS256:
      return "PS256";
    case JWSAlgorithm::PS384:
      return "PS384";
    case JWSAlgorithm::PS512:
      return "PS512";
    case JWSAlgorithm::ES256:
      return "ES256";
    case JWSAlgorithm::ES384:
      return "ES384";
    case JWSAlgorithm::ES512:
      return "ES512";
    case JWSAlgorithm::EdDSA:
      return "EdDSA";
    case JWSAlgorithm::HS256:
      return "HS256";
    case JWSAlgorithm::HS384:
      return "HS384";
    case JWSAlgorithm::HS512:
      return "HS512";
  }

  std::unreachable();
}

auto jws_algorithm_is_asymmetric(const JWSAlgorithm algorithm) noexcept
    -> bool {
  switch (algorithm) {
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
      return true;
    case JWSAlgorithm::HS256:
    case JWSAlgorithm::HS384:
    case JWSAlgorithm::HS512:
      return false;
  }

  std::unreachable();
}

auto jws_algorithm_digest_bits(const JWSAlgorithm algorithm) noexcept
    -> std::uint16_t {
  switch (algorithm) {
    case JWSAlgorithm::RS256:
    case JWSAlgorithm::PS256:
    case JWSAlgorithm::ES256:
    case JWSAlgorithm::HS256:
      return 256;
    case JWSAlgorithm::RS384:
    case JWSAlgorithm::PS384:
    case JWSAlgorithm::ES384:
    case JWSAlgorithm::HS384:
      return 384;
    case JWSAlgorithm::RS512:
    case JWSAlgorithm::PS512:
    case JWSAlgorithm::ES512:
    case JWSAlgorithm::HS512:
    case JWSAlgorithm::EdDSA:
      return 512;
  }

  std::unreachable();
}

auto to_jwe_algorithm(const std::string_view value) noexcept
    -> std::optional<JWEAlgorithm> {
  if (value == "RSA-OAEP") {
    return JWEAlgorithm::RSA_OAEP;
  } else if (value == "RSA-OAEP-256") {
    return JWEAlgorithm::RSA_OAEP_256;
  } else if (value == "ECDH-ES") {
    return JWEAlgorithm::ECDH_ES;
  } else if (value == "ECDH-ES+A128KW") {
    return JWEAlgorithm::ECDH_ES_A128KW;
  } else if (value == "ECDH-ES+A192KW") {
    return JWEAlgorithm::ECDH_ES_A192KW;
  } else if (value == "ECDH-ES+A256KW") {
    return JWEAlgorithm::ECDH_ES_A256KW;
  } else if (value == "A128KW") {
    return JWEAlgorithm::A128KW;
  } else if (value == "A192KW") {
    return JWEAlgorithm::A192KW;
  } else if (value == "A256KW") {
    return JWEAlgorithm::A256KW;
  } else if (value == "dir") {
    return JWEAlgorithm::DIR;
  } else {
    return std::nullopt;
  }
}

auto jwe_algorithm_name(const JWEAlgorithm algorithm) noexcept
    -> std::string_view {
  switch (algorithm) {
    case JWEAlgorithm::RSA_OAEP:
      return "RSA-OAEP";
    case JWEAlgorithm::RSA_OAEP_256:
      return "RSA-OAEP-256";
    case JWEAlgorithm::ECDH_ES:
      return "ECDH-ES";
    case JWEAlgorithm::ECDH_ES_A128KW:
      return "ECDH-ES+A128KW";
    case JWEAlgorithm::ECDH_ES_A192KW:
      return "ECDH-ES+A192KW";
    case JWEAlgorithm::ECDH_ES_A256KW:
      return "ECDH-ES+A256KW";
    case JWEAlgorithm::A128KW:
      return "A128KW";
    case JWEAlgorithm::A192KW:
      return "A192KW";
    case JWEAlgorithm::A256KW:
      return "A256KW";
    case JWEAlgorithm::DIR:
      return "dir";
  }

  std::unreachable();
}

auto to_jwe_encryption(const std::string_view value) noexcept
    -> std::optional<JWEEncryption> {
  if (value == "A128GCM") {
    return JWEEncryption::A128GCM;
  } else if (value == "A192GCM") {
    return JWEEncryption::A192GCM;
  } else if (value == "A256GCM") {
    return JWEEncryption::A256GCM;
  } else if (value == "A128CBC-HS256") {
    return JWEEncryption::A128CBC_HS256;
  } else if (value == "A192CBC-HS384") {
    return JWEEncryption::A192CBC_HS384;
  } else if (value == "A256CBC-HS512") {
    return JWEEncryption::A256CBC_HS512;
  } else {
    return std::nullopt;
  }
}

auto jwe_encryption_name(const JWEEncryption encryption) noexcept
    -> std::string_view {
  switch (encryption) {
    case JWEEncryption::A128GCM:
      return "A128GCM";
    case JWEEncryption::A192GCM:
      return "A192GCM";
    case JWEEncryption::A256GCM:
      return "A256GCM";
    case JWEEncryption::A128CBC_HS256:
      return "A128CBC-HS256";
    case JWEEncryption::A192CBC_HS384:
      return "A192CBC-HS384";
    case JWEEncryption::A256CBC_HS512:
      return "A256CBC-HS512";
  }

  std::unreachable();
}

auto jwe_algorithm_is_asymmetric(const JWEAlgorithm algorithm) noexcept
    -> bool {
  switch (algorithm) {
    case JWEAlgorithm::RSA_OAEP:
    case JWEAlgorithm::RSA_OAEP_256:
    case JWEAlgorithm::ECDH_ES:
    case JWEAlgorithm::ECDH_ES_A128KW:
    case JWEAlgorithm::ECDH_ES_A192KW:
    case JWEAlgorithm::ECDH_ES_A256KW:
      return true;
    case JWEAlgorithm::A128KW:
    case JWEAlgorithm::A192KW:
    case JWEAlgorithm::A256KW:
    case JWEAlgorithm::DIR:
      return false;
  }

  std::unreachable();
}

auto jwe_encryption_key_bytes(const JWEEncryption encryption) noexcept
    -> std::size_t {
  switch (encryption) {
    case JWEEncryption::A128GCM:
      return 16;
    case JWEEncryption::A192GCM:
      return 24;
    // A GCM key is used directly, while a CBC-HMAC key of the same length is
    // the MAC key followed by an equal-length encryption key (RFC 7518 Section
    // 5.2.2.1), so A256GCM and A128CBC-HS256 both take 32 octets
    case JWEEncryption::A256GCM:
    case JWEEncryption::A128CBC_HS256:
      return 32;
    case JWEEncryption::A192CBC_HS384:
      return 48;
    case JWEEncryption::A256CBC_HS512:
      return 64;
  }

  std::unreachable();
}

} // namespace sourcemeta::core
