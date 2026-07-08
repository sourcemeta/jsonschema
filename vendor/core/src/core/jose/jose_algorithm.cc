#include <sourcemeta/core/jose_algorithm.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

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

} // namespace sourcemeta::core
