#include <sourcemeta/core/oidc_id_token.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oidc_hash.h>
#include <sourcemeta/core/time.h>

#include <chrono> // std::chrono::system_clock, std::chrono::seconds, std::chrono::duration, std::chrono::duration_cast
#include <cstdint>     // std::int64_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <stdexcept>   // std::out_of_range
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_ALG{JSON::Object::hash("alg"sv)};
constexpr auto HASH_ISS{JSON::Object::hash("iss"sv)};
constexpr auto HASH_SUB{JSON::Object::hash("sub"sv)};
constexpr auto HASH_AUD{JSON::Object::hash("aud"sv)};
constexpr auto HASH_EXP{JSON::Object::hash("exp"sv)};
constexpr auto HASH_IAT{JSON::Object::hash("iat"sv)};
constexpr auto HASH_NONCE{JSON::Object::hash("nonce"sv)};
constexpr auto HASH_AZP{JSON::Object::hash("azp"sv)};
constexpr auto HASH_ACR{JSON::Object::hash("acr"sv)};
constexpr auto HASH_AUTH_TIME{JSON::Object::hash("auth_time"sv)};
constexpr auto HASH_AT_HASH{JSON::Object::hash("at_hash"sv)};
constexpr auto HASH_C_HASH{JSON::Object::hash("c_hash"sv)};
constexpr auto HASH_ID_TOKEN{JSON::Object::hash("id_token"sv)};

auto epoch_seconds(const std::chrono::system_clock::time_point point) -> JSON {
  return JSON{static_cast<std::int64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(point.time_since_epoch())
          .count())};
}

// The OpenID Connect specific ID Token checks that layer on top of the base
// JSON Web Token verification (OpenID Connect Core 1.0 Section 3.1.3.7)
auto oidc_id_token_checks(const JWT &token, const std::string_view issuer,
                          const std::string_view client_id,
                          const std::chrono::system_clock::time_point now,
                          const OIDCValidationOptions &options)
    -> std::optional<OIDCIdentity> {
  const auto &payload{token.payload()};

  // OpenID Connect Core 1.0 Section 2: the subject is REQUIRED in an ID Token
  const auto subject{token.subject()};
  if (!subject.has_value()) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 3.1.3.7 step 5: when the audience carries
  // more than one value the authorized party is REQUIRED and must be the
  // client, and when it is present at all it must be the client
  const auto *audience{payload.try_at("aud"sv, HASH_AUD)};

  // A malformed audience array, one carrying a non-string member, is rejected
  // rather than accepted because one member happened to match the client
  // (OpenID Connect Core 1.0 Section 2)
  if (audience != nullptr && audience->is_array() &&
      !audience->is_array_of_strings()) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 3.1.3.7 step 3: "The ID Token MUST be
  // rejected if the ID Token does not list the Client as a valid audience, or
  // if it contains additional audiences not trusted by the Client". The base
  // verification already confirmed the client is a listed audience, so here
  // every other audience must be one the caller has marked as trusted. With an
  // empty trusted set any audience beyond the client is rejected, the strict
  // default. A single-string audience carries no additional audience, so it is
  // unaffected
  if (audience != nullptr && audience->is_array()) {
    for (const auto &element : audience->as_array()) {
      const auto &value{element.to_string()};
      if (value == client_id) {
        continue;
      }

      bool trusted{false};
      for (const auto candidate : options.trusted_audiences) {
        if (candidate == value) {
          trusted = true;
          break;
        }
      }

      if (!trusted) {
        return std::nullopt;
      }
    }
  }

  const bool multiple_audiences{audience != nullptr && audience->is_array() &&
                                audience->size() > 1};
  const auto *authorized_party{payload.try_at("azp"sv, HASH_AZP)};
  if (multiple_audiences) {
    if (authorized_party == nullptr || !authorized_party->is_string() ||
        !secure_equals(authorized_party->to_string(), client_id)) {
      return std::nullopt;
    }
  } else if (authorized_party != nullptr) {
    if (!authorized_party->is_string() ||
        !secure_equals(authorized_party->to_string(), client_id)) {
      return std::nullopt;
    }
  }

  // OpenID Connect Core 1.0 Section 3.1.3.7 step 11: the nonce is echoed and
  // compared in constant time when one was sent
  if (options.nonce.has_value()) {
    const auto *nonce{payload.try_at("nonce"sv, HASH_NONCE)};
    if (nonce == nullptr || !nonce->is_string() ||
        !secure_equals(nonce->to_string(), options.nonce.value())) {
      return std::nullopt;
    }
  }

  // OpenID Connect Core 1.0 Section 2: iat is REQUIRED in an ID Token, so its
  // presence and validity are enforced unconditionally rather than as an opt-in
  const auto issued_at{token.issued_at()};
  if (!issued_at.has_value()) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 3.1.3.7 step 10: the optional issued-at age
  // policy
  if (options.maximum_issued_at_age.has_value() &&
      issued_at.value() <
          clock_shift_backward(now, options.maximum_issued_at_age.value())) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 3.1.3.7 step 12: the authentication context
  // class must be acceptable when a set was requested
  const auto *class_reference{payload.try_at("acr"sv, HASH_ACR)};
  if (!options.acceptable_authentication_context_classes.empty()) {
    if (class_reference == nullptr || !class_reference->is_string()) {
      return std::nullopt;
    }

    bool acceptable{false};
    for (const auto candidate :
         options.acceptable_authentication_context_classes) {
      if (candidate == class_reference->to_string()) {
        acceptable = true;
        break;
      }
    }

    if (!acceptable) {
      return std::nullopt;
    }
  }

  // OpenID Connect Core 1.0 Section 3.1.3.7 step 13: re-authentication is
  // forced when the authentication is older than the requested maximum age
  std::optional<std::chrono::system_clock::time_point> authentication_time;
  const auto *auth_time{payload.try_at("auth_time"sv, HASH_AUTH_TIME)};
  if (auth_time != nullptr && auth_time->is_number()) {
    // A NumericDate may be non-integer (RFC 7519 Section 2), and a value beyond
    // the range of a double cannot stand for a usable timestamp, so it is
    // treated as absent rather than aborting, matching how the JWT layer reads
    // the registered time claims
    try {
      authentication_time = from_unix_timestamp(
          std::chrono::duration<double>{auth_time->as_real()});
    } catch (const std::out_of_range &) {
      authentication_time = std::nullopt;
    }
  }

  if (options.maximum_authentication_age.has_value()) {
    // An authentication time in the future has not happened yet, so it cannot
    // satisfy a freshness window and is rejected before the age comparison
    if (!authentication_time.has_value() || authentication_time.value() > now ||
        authentication_time.value() <
            clock_shift_backward(now,
                                 options.maximum_authentication_age.value())) {
      return std::nullopt;
    }
  }

  // OpenID Connect Core 1.0 Sections 3.1.3.6, 3.2.2.10, and 3.3.2.11: the
  // binding hash is REQUIRED in the front-channel flows. A required binding
  // must be a string claim actually verified against its token, so an absent or
  // non-string claim, or a missing token to verify against, all fail. When it
  // is not required, a present claim is still verified whenever the token is
  // given
  const auto *at_hash{payload.try_at("at_hash"sv, HASH_AT_HASH)};
  if (options.require_access_token_hash) {
    if (at_hash == nullptr || !at_hash->is_string() ||
        !options.access_token.has_value() || !token.algorithm().has_value() ||
        !oidc_verify_token_hash(options.access_token.value(),
                                token.algorithm().value(),
                                at_hash->to_string())) {
      return std::nullopt;
    }
  } else if (at_hash != nullptr && options.access_token.has_value() &&
             (!at_hash->is_string() || !token.algorithm().has_value() ||
              !oidc_verify_token_hash(options.access_token.value(),
                                      token.algorithm().value(),
                                      at_hash->to_string()))) {
    return std::nullopt;
  }

  const auto *code_hash{payload.try_at("c_hash"sv, HASH_C_HASH)};
  if (options.require_code_hash) {
    if (code_hash == nullptr || !code_hash->is_string() ||
        !options.code.has_value() || !token.algorithm().has_value() ||
        !oidc_verify_token_hash(options.code.value(), token.algorithm().value(),
                                code_hash->to_string())) {
      return std::nullopt;
    }
  } else if (code_hash != nullptr && options.code.has_value() &&
             (!code_hash->is_string() || !token.algorithm().has_value() ||
              !oidc_verify_token_hash(options.code.value(),
                                      token.algorithm().value(),
                                      code_hash->to_string()))) {
    return std::nullopt;
  }

  OIDCIdentity identity;
  identity.subject = std::string{subject.value()};
  identity.issuer = std::string{issuer};
  if (class_reference != nullptr && class_reference->is_string()) {
    identity.authentication_context_class =
        std::string{class_reference->to_string()};
  }

  identity.authentication_time = authentication_time;
  return identity;
}

} // namespace

auto oidc_parse_id_token(const JSON &token_response)
    -> std::optional<std::string> {
  if (!token_response.is_object()) {
    return std::nullopt;
  }

  const auto *member{token_response.try_at("id_token"sv, HASH_ID_TOKEN)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  return member->to_string();
}

auto oidc_validate_id_token(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view issuer, const std::string_view client_id,
    const std::chrono::system_clock::time_point now,
    const OIDCValidationOptions &options, const JWTClockSkew clock_skew)
    -> std::optional<OIDCIdentity> {
  // The base verification pins the algorithm and checks the signature, issuer,
  // audience, expiration, not-before, and issued-at (OpenID Connect Core 1.0
  // Section 3.1.3.7 steps 6 through 9)
  const auto error{jwt_verify(token, keys, allowed_algorithms, issuer,
                              client_id, now, clock_skew, std::nullopt,
                              std::nullopt)};
  if (error.has_value()) {
    return std::nullopt;
  }

  return oidc_id_token_checks(token, issuer, client_id, now, options);
}

auto oidc_validate_id_token(
    JWKSProvider &provider, const JWT &token,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view issuer, const std::string_view client_id,
    const OIDCValidationOptions &options) -> std::optional<OIDCIdentity> {
  // The provider owns the clock, and the OpenID Connect steps run against the
  // same reading the signature verification used
  std::chrono::system_clock::time_point resolved_now;
  const auto error{provider.verify(token, allowed_algorithms, issuer, client_id,
                                   std::nullopt, std::nullopt, resolved_now)};
  if (error.has_value()) {
    return std::nullopt;
  }

  return oidc_id_token_checks(token, issuer, client_id, resolved_now, options);
}

auto oidc_mint_id_token(const OIDCIdTokenClaims &claims, const JWKPrivate &key,
                        const JWSAlgorithm algorithm)
    -> std::optional<std::string> {
  auto header{JSON::make_object()};
  header.assign_assume_new("alg", JSON{jws_algorithm_name(algorithm)},
                           HASH_ALG);

  auto payload{JSON::make_object()};
  payload.assign_assume_new("iss", JSON{claims.issuer}, HASH_ISS);
  payload.assign_assume_new("sub", JSON{claims.subject}, HASH_SUB);
  payload.assign_assume_new("aud", JSON{claims.audience}, HASH_AUD);
  payload.assign_assume_new("exp", epoch_seconds(claims.expiration), HASH_EXP);
  payload.assign_assume_new("iat", epoch_seconds(claims.issued_at), HASH_IAT);

  if (claims.nonce.has_value()) {
    payload.assign_assume_new("nonce", JSON{claims.nonce.value()}, HASH_NONCE);
  }

  if (claims.authorized_party.has_value()) {
    payload.assign_assume_new("azp", JSON{claims.authorized_party.value()},
                              HASH_AZP);
  }

  if (claims.authentication_context_class.has_value()) {
    payload.assign_assume_new(
        "acr", JSON{claims.authentication_context_class.value()}, HASH_ACR);
  }

  if (claims.authentication_time.has_value()) {
    payload.assign_assume_new("auth_time",
                              epoch_seconds(claims.authentication_time.value()),
                              HASH_AUTH_TIME);
  }

  // OpenID Connect Core 1.0 Section 3.1.3.6: at_hash binds the access token and
  // c_hash binds the code, both hashed under the ID Token signing algorithm.
  // When a token to bind is supplied but its hash cannot be computed for the
  // algorithm, minting fails rather than emitting a token missing a REQUIRED
  // binding
  if (claims.access_token.has_value()) {
    const auto hash{oidc_token_hash(claims.access_token.value(), algorithm)};
    if (!hash.has_value()) {
      return std::nullopt;
    }

    payload.assign_assume_new("at_hash", JSON{hash.value()}, HASH_AT_HASH);
  }

  if (claims.code.has_value()) {
    const auto hash{oidc_token_hash(claims.code.value(), algorithm)};
    if (!hash.has_value()) {
      return std::nullopt;
    }

    payload.assign_assume_new("c_hash", JSON{hash.value()}, HASH_C_HASH);
  }

  return jwt_sign(header, payload, key);
}

} // namespace sourcemeta::core
