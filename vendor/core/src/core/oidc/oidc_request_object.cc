#include <sourcemeta/core/oidc_request_object.h>

#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>

#include "oidc_verify.h"

#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_ALG{JSON::Object::hash("alg"sv)};
constexpr auto HASH_ISS{JSON::Object::hash("iss"sv)};

} // namespace

auto oidc_request_object_pairing_is_valid(const std::string_view request,
                                          const std::string_view request_uri)
    -> bool {
  // OpenID Connect Core 1.0 Section 6: the request and request_uri parameters
  // MUST NOT both be used in the same request
  return request.empty() || request_uri.empty();
}

auto oidc_build_request_object(const JSON &parameters, const JWKPrivate &key,
                               const JWSAlgorithm algorithm)
    -> std::optional<std::string> {
  if (!parameters.is_object()) {
    return std::nullopt;
  }

  auto header{JSON::make_object()};
  header.assign_assume_new("alg", JSON{jws_algorithm_name(algorithm)},
                           HASH_ALG);
  return jwt_sign(header, parameters, key);
}

auto oidc_verify_request_object(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view client_id, const std::string_view provider_issuer)
    -> std::optional<JSON> {
  // OpenID Connect Core 1.0 Section 6.3: the algorithm is pinned and the key is
  // selected by identifier, never taken from the header alone
  if (!oidc_verify_selected_signature(token, keys, allowed_algorithms)) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 6.1: the request object is issued by the
  // client and addressed to the OpenID Provider, so iss and aud are REQUIRED
  // and must match the client and the provider respectively
  const auto &payload{token.payload()};
  const auto *issuer{payload.try_at("iss"sv, HASH_ISS)};
  if (issuer == nullptr || !issuer->is_string() ||
      issuer->to_string() != client_id) {
    return std::nullopt;
  }

  if (!token.has_audience(provider_issuer)) {
    return std::nullopt;
  }

  return payload;
}

} // namespace sourcemeta::core
