#include <sourcemeta/core/jose_algorithm.h>

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

} // namespace sourcemeta::core
