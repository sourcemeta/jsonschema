#include <sourcemeta/core/oidc_userinfo.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>

#include "oidc_verify.h"

#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view

namespace {
using namespace std::literals::string_view_literals;

// A claim name against the hash of that name. Each of these is looked up in
// both answers, and in the merged result besides, so the name is hashed once
// here rather than at every lookup
struct Claim {
  sourcemeta::core::JSON::StringView name;
  sourcemeta::core::JSON::Object::hash_type hash;
};

constexpr Claim CLAIM_EMAIL{
    .name = "email"sv, .hash = sourcemeta::core::JSON::Object::hash("email"sv)};
constexpr Claim CLAIM_EMAIL_VERIFIED{
    .name = "email_verified"sv,
    .hash = sourcemeta::core::JSON::Object::hash("email_verified"sv)};
constexpr Claim CLAIM_PHONE_NUMBER{
    .name = "phone_number"sv,
    .hash = sourcemeta::core::JSON::Object::hash("phone_number"sv)};
constexpr Claim CLAIM_PHONE_NUMBER_VERIFIED{
    .name = "phone_number_verified"sv,
    .hash = sourcemeta::core::JSON::Object::hash("phone_number_verified"sv)};
constexpr Claim CLAIM_NAMES{
    .name = "_claim_names"sv,
    .hash = sourcemeta::core::JSON::Object::hash("_claim_names"sv)};
constexpr Claim CLAIM_SOURCES{
    .name = "_claim_sources"sv,
    .hash = sourcemeta::core::JSON::Object::hash("_claim_sources"sv)};

// A claim delivers nothing when it is absent, null, or an empty string, which
// are the shapes OpenID Connect Core 1.0 Section 5.3.2 names in having an
// unreturned claim omitted rather than "present with a null or empty string
// value". It names no others, so an empty array or object is a value like any
// other here and only the aggregated members below read one differently
auto carries(const sourcemeta::core::JSON &value) -> bool {
  return !value.is_null() && !(value.is_string() && value.empty());
}

auto carries(const sourcemeta::core::JSON &claims, const Claim &claim) -> bool {
  const auto *value{claims.try_at(claim.name, claim.hash)};
  return value != nullptr && carries(*value);
}

// OpenID Connect Core 1.0 Section 5.6.2 makes `_claim_names` the object whose
// "member names are the Claim Names for the Aggregated and Distributed
// Claims" and `_claim_sources` the object those names reference, so a member
// that is empty names none and provides none, and one that is not an object
// resolves nothing at all
auto carries_aggregated(const sourcemeta::core::JSON &claims,
                        const Claim &claim) -> bool {
  const auto *value{claims.try_at(claim.name, claim.hash)};
  return value != nullptr && value->is_object() && !value->empty();
}

// A verified assertion speaks for the value delivered alongside it, so the two
// are taken whole from the answer that carried the value, and an assertion
// arriving on its own stands for nothing
auto merge_verified_pair(sourcemeta::core::JSON &result,
                         const sourcemeta::core::JSON &id_token_claims,
                         const sourcemeta::core::JSON &userinfo,
                         const Claim &subject, const Claim &assertion) -> void {
  if (carries(id_token_claims, subject)) {
    return;
  }

  if (carries(userinfo, subject)) {
    result.assign(subject.name, userinfo.at(subject.name, subject.hash));
    if (carries(userinfo, assertion)) {
      result.assign(assertion.name,
                    userinfo.at(assertion.name, assertion.hash));
    } else {
      result.erase(assertion.name, assertion.hash);
    }

    return;
  }

  result.erase(assertion.name, assertion.hash);
}

} // namespace

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_SUB{JSON::Object::hash("sub"sv)};
constexpr auto HASH_ISS{JSON::Object::hash("iss"sv)};
constexpr auto HASH_AUD{JSON::Object::hash("aud"sv)};

} // namespace

auto oidc_build_userinfo(const std::string_view subject,
                         const JSON &additional_claims) -> JSON {
  auto document{additional_claims.is_object() ? additional_claims
                                              : JSON::make_object()};
  // OpenID Connect Core 1.0 Section 5.3.2: the sub claim is REQUIRED in a
  // UserInfo response
  document.assign("sub", JSON{subject});
  return document;
}

auto oidc_userinfo_matches_subject(const JSON &userinfo,
                                   const std::string_view expected_subject)
    -> bool {
  if (!userinfo.is_object()) {
    return false;
  }

  const auto *subject{userinfo.try_at("sub"sv, HASH_SUB)};
  return subject != nullptr && subject->is_string() &&
         secure_equals(subject->to_string(), expected_subject);
}

auto oidc_verify_userinfo(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view expected_subject,
    const std::string_view expected_issuer,
    const std::string_view expected_client_id) -> std::optional<JSON> {
  // OpenID Connect Core 1.0 Section 5.3.2: the algorithm is pinned and the key
  // selected by identifier, never taken from the token header alone
  if (!oidc_verify_selected_signature(token, keys, allowed_algorithms)) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 5.3.2: the sub must match the ID Token sub
  if (!oidc_userinfo_matches_subject(token.payload(), expected_subject)) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 errata set 2 Section 5.3.2: "If signed, the
  // UserInfo Response ... MUST contain the Claims iss (issuer) and aud
  // (audience) as members". So a signed response is rejected when either is
  // absent, the iss must be this provider, and the aud must include this
  // client, which stops a response minted for another client from being
  // accepted here
  const auto *issuer{token.payload().try_at("iss"sv, HASH_ISS)};
  if (issuer == nullptr || !issuer->is_string() ||
      issuer->to_string() != expected_issuer) {
    return std::nullopt;
  }

  const auto *audience{token.payload().try_at("aud"sv, HASH_AUD)};
  if (audience == nullptr || !token.has_audience(expected_client_id)) {
    return std::nullopt;
  }

  return token.payload();
}

auto oidc_merge_claims(const JSON &id_token_claims, const JSON &userinfo)
    -> std::optional<JSON> {
  if (!id_token_claims.is_object() || !userinfo.is_object()) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 2 requires the subject of an ID Token, and
  // Section 5.3.2 requires the UserInfo subject to match it exactly, without
  // which "the UserInfo Response values MUST NOT be used"
  const auto *subject{id_token_claims.try_at("sub"sv, HASH_SUB)};
  if (subject == nullptr || !subject->is_string() ||
      !oidc_userinfo_matches_subject(userinfo, subject->to_string())) {
    return std::nullopt;
  }

  // Only the ID Token is signed, so what it says stands and the second answer
  // only fills what the first left out
  auto result{id_token_claims};
  merge_verified_pair(result, id_token_claims, userinfo, CLAIM_EMAIL,
                      CLAIM_EMAIL_VERIFIED);
  merge_verified_pair(result, id_token_claims, userinfo, CLAIM_PHONE_NUMBER,
                      CLAIM_PHONE_NUMBER_VERIFIED);

  // Section 5.6.2 makes the member values of `_claim_names` "references to the
  // member names in the `_claim_sources` member", so the two are taken from
  // one answer or neither, never spliced into a reference with nothing to
  // resolve against
  const auto aggregated{carries_aggregated(id_token_claims, CLAIM_NAMES) ||
                        carries_aggregated(id_token_claims, CLAIM_SOURCES)};
  if (!aggregated && carries_aggregated(userinfo, CLAIM_NAMES) &&
      carries_aggregated(userinfo, CLAIM_SOURCES)) {
    result.assign(CLAIM_NAMES.name,
                  userinfo.at(CLAIM_NAMES.name, CLAIM_NAMES.hash));
    result.assign(CLAIM_SOURCES.name,
                  userinfo.at(CLAIM_SOURCES.name, CLAIM_SOURCES.hash));
  }

  for (const auto &entry : userinfo.as_object()) {
    if (entry.key_equals(CLAIM_EMAIL.name, CLAIM_EMAIL.hash) ||
        entry.key_equals(CLAIM_EMAIL_VERIFIED.name,
                         CLAIM_EMAIL_VERIFIED.hash) ||
        entry.key_equals(CLAIM_PHONE_NUMBER.name, CLAIM_PHONE_NUMBER.hash) ||
        entry.key_equals(CLAIM_PHONE_NUMBER_VERIFIED.name,
                         CLAIM_PHONE_NUMBER_VERIFIED.hash) ||
        entry.key_equals(CLAIM_NAMES.name, CLAIM_NAMES.hash) ||
        entry.key_equals(CLAIM_SOURCES.name, CLAIM_SOURCES.hash)) {
      continue;
    }

    // A second answer only fills what the first left out, and neither side
    // fills anything with a value that carries nothing
    if (carries(entry.second) &&
        !carries(result, {.name = entry.first, .hash = entry.hash})) {
      result.assign(entry.first, entry.second);
    }
  }

  return result;
}

} // namespace sourcemeta::core
