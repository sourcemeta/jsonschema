#include <sourcemeta/core/oidc_claims.h>

#include <sourcemeta/core/json.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <functional>  // std::function
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_USERINFO{JSON::Object::hash("userinfo"sv)};
constexpr auto HASH_ID_TOKEN{JSON::Object::hash("id_token"sv)};
constexpr auto HASH_ESSENTIAL{JSON::Object::hash("essential"sv)};
constexpr auto HASH_VALUE{JSON::Object::hash("value"sv)};
constexpr auto HASH_VALUES{JSON::Object::hash("values"sv)};

// OpenID Connect Core 1.0 Section 5.1: the standard claims
constexpr std::array<std::string_view, 20> STANDARD_CLAIMS{
    {"sub",
     "name",
     "given_name",
     "family_name",
     "middle_name",
     "nickname",
     "preferred_username",
     "profile",
     "picture",
     "website",
     "email",
     "email_verified",
     "gender",
     "birthdate",
     "zoneinfo",
     "locale",
     "phone_number",
     "phone_number_verified",
     "address",
     "updated_at"}};

// OpenID Connect Core 1.0 Section 5.4: the profile scope claim set
constexpr std::array<std::string_view, 14> PROFILE_CLAIMS{
    {"name", "family_name", "given_name", "middle_name", "nickname",
     "preferred_username", "profile", "picture", "website", "gender",
     "birthdate", "zoneinfo", "locale", "updated_at"}};

constexpr std::array<std::string_view, 2> EMAIL_CLAIMS{
    {"email", "email_verified"}};

constexpr std::array<std::string_view, 1> ADDRESS_CLAIMS{{"address"}};

constexpr std::array<std::string_view, 2> PHONE_CLAIMS{
    {"phone_number", "phone_number_verified"}};

auto emit_claims(const std::span<const std::string_view> claims,
                 const std::function<void(std::string_view)> &on_claim)
    -> void {
  for (const auto claim : claims) {
    on_claim(claim);
  }
}

auto claim_set_contains(const std::span<const std::string_view> claims,
                        const std::string_view claim) noexcept -> bool {
  for (const auto candidate : claims) {
    if (candidate == claim) {
      return true;
    }
  }

  return false;
}

// OpenID Connect Core 1.0 Section 5.5.1: a claim request is null in the default
// manner, or an object carrying essential, value, or values
auto build_claim_specification(const OIDCClaimRequest &request) -> JSON {
  if (!request.essential && request.value == nullptr &&
      request.values.empty()) {
    return JSON{nullptr};
  }

  auto specification{JSON::make_object()};
  if (request.essential) {
    specification.assign_assume_new("essential", JSON{true}, HASH_ESSENTIAL);
  }

  if (request.value != nullptr) {
    specification.assign_assume_new("value", JSON{*request.value}, HASH_VALUE);
  }

  if (!request.values.empty()) {
    auto candidates{JSON::make_array()};
    for (const auto &candidate : request.values) {
      candidates.push_back(JSON{candidate});
    }

    specification.assign_assume_new("values", std::move(candidates),
                                    HASH_VALUES);
  }

  return specification;
}

auto assign_claim_requests(JSON &document, const std::string_view target,
                           const JSON::Object::hash_type hash,
                           const std::span<const OIDCClaimRequest> claims)
    -> void {
  if (claims.empty()) {
    return;
  }

  auto member{JSON::make_object()};
  for (const auto &request : claims) {
    member.assign(request.name, build_claim_specification(request));
  }

  document.assign_assume_new(std::string{target}, std::move(member), hash);
}

// OpenID Connect Core 1.0 Section 5.5: the request entry for a claim under a
// target member, or no pointer when the target or claim is absent
auto claim_specification(const JSON &claims, const std::string_view target,
                         const std::string_view claim) -> const JSON * {
  if (!claims.is_object()) {
    return nullptr;
  }

  const auto *target_object{claims.try_at(target)};
  if (target_object == nullptr || !target_object->is_object()) {
    return nullptr;
  }

  return target_object->try_at(claim);
}

// RFC 7643 Section 2.4: an element of a multi-valued attribute is either a
// primitive or a complex object, where the significant value lives under
// "value" and every other sub-attribute is descriptive
auto claim_request_accepts_member(const JSON &request, const JSON &value)
    -> bool {
  // RFC 7643 Section 2.5: an unassigned attribute, the null value, and the
  // empty array "SHALL be considered to be equivalent in state", and the empty
  // array carries no membership, so neither does the null value
  if (value.is_null()) {
    return false;
  }

  if (value.is_object()) {
    const auto *significant{value.try_at("value"sv, HASH_VALUE)};
    return significant != nullptr && !significant->is_null() &&
           oidc_claim_request_accepts(request, *significant);
  }

  // A nested array is no member shape the schema defines
  if (value.is_array()) {
    return false;
  }

  return oidc_claim_request_accepts(request, value);
}

} // namespace

auto oidc_is_standard_claim(const std::string_view name) noexcept -> bool {
  return claim_set_contains(STANDARD_CLAIMS, name);
}

auto oidc_scope_to_claims(const std::string_view scopes,
                          const std::function<void(std::string_view)> &on_claim)
    -> void {
  bool has_openid{false};
  bool has_profile{false};
  bool has_email{false};
  bool has_address{false};
  bool has_phone{false};

  std::size_t position{0};
  while (position <= scopes.size()) {
    const auto space{scopes.find(' ', position)};
    const auto value{space == std::string_view::npos
                         ? scopes.substr(position)
                         : scopes.substr(position, space - position)};
    if (value == "openid") {
      has_openid = true;
    } else if (value == "profile") {
      has_profile = true;
    } else if (value == "email") {
      has_email = true;
    } else if (value == "address") {
      has_address = true;
    } else if (value == "phone") {
      has_phone = true;
    }

    if (space == std::string_view::npos) {
      break;
    }

    position = space + 1;
  }

  // OpenID Connect Core 1.0 Section 5.4: each claim-requesting scope yields
  // its claim set, and openid yields sub, which is always returned
  // (Section 5.3.2). The scope claim sets are disjoint, so a claim is reported
  // at most once
  if (has_openid) {
    on_claim("sub");
  }

  if (has_profile) {
    emit_claims(PROFILE_CLAIMS, on_claim);
  }

  if (has_email) {
    emit_claims(EMAIL_CLAIMS, on_claim);
  }

  if (has_address) {
    emit_claims(ADDRESS_CLAIMS, on_claim);
  }

  if (has_phone) {
    emit_claims(PHONE_CLAIMS, on_claim);
  }
}

auto oidc_claim_to_scope(const std::string_view claim) noexcept
    -> std::optional<std::string_view> {
  // OpenID Connect Core 1.0 Section 5.3.2: "The sub (subject) Claim MUST
  // always be returned in the UserInfo Response", so the openid scope itself
  // is what requests it
  if (claim == "sub") {
    return "openid";
  }

  if (claim_set_contains(PROFILE_CLAIMS, claim)) {
    return "profile";
  }

  if (claim_set_contains(EMAIL_CLAIMS, claim)) {
    return "email";
  }

  if (claim_set_contains(ADDRESS_CLAIMS, claim)) {
    return "address";
  }

  if (claim_set_contains(PHONE_CLAIMS, claim)) {
    return "phone";
  }

  // OpenID Connect Core 1.0 Section 5.4 defines no other claim-requesting
  // scope, and a scope name is never invented from a claim name, since a
  // server may reject a request carrying an unknown scope as invalid_scope
  // (RFC 6749 Section 4.1.2.1)
  return std::nullopt;
}

auto oidc_build_claims_parameter(
    const std::span<const OIDCClaimRequest> userinfo_claims,
    const std::span<const OIDCClaimRequest> id_token_claims) -> JSON {
  auto document{JSON::make_object()};
  assign_claim_requests(document, "userinfo", HASH_USERINFO, userinfo_claims);
  assign_claim_requests(document, "id_token", HASH_ID_TOKEN, id_token_claims);
  return document;
}

auto oidc_claims_parameter_requests(const JSON &claims,
                                    const std::string_view target,
                                    const std::string_view claim) -> bool {
  // OpenID Connect Core 1.0 Section 5.5: a claim entry is either null (the
  // default manner) or an object, so a malformed value such as a string is not
  // honored as a request
  const auto *specification{claim_specification(claims, target, claim)};
  return specification != nullptr &&
         (specification->is_null() || specification->is_object());
}

auto oidc_claims_parameter_is_essential(const JSON &claims,
                                        const std::string_view target,
                                        const std::string_view claim) -> bool {
  const auto *specification{claim_specification(claims, target, claim)};
  if (specification == nullptr || !specification->is_object()) {
    return false;
  }

  const auto *essential{specification->try_at("essential"sv, HASH_ESSENTIAL)};
  return essential != nullptr && essential->is_boolean() &&
         essential->to_boolean();
}

auto oidc_claims_parameter_value(const JSON &claims,
                                 const std::string_view target,
                                 const std::string_view claim) -> const JSON * {
  const auto *specification{claim_specification(claims, target, claim)};
  if (specification == nullptr || !specification->is_object()) {
    return nullptr;
  }

  return specification->try_at("value"sv, HASH_VALUE);
}

auto oidc_claim_request_accepts(const JSON &request, const JSON &value)
    -> bool {
  // OpenID Connect Core 1.0 Section 5.5.1: only a null or object request is
  // valid, so a malformed one permits nothing
  if (!(request.is_null() || request.is_object())) {
    return false;
  }

  // A null request carries no value constraint, so it permits any value
  if (request.is_null()) {
    return true;
  }

  // OpenID Connect Core 1.0 Section 5.5.1: value requests an exact value and
  // values a set of acceptable ones, so a request carrying neither is
  // unconstrained, and a present but malformed values constraint permits
  // nothing rather than silently opening the request up
  const auto *requested_value{request.try_at("value"sv, HASH_VALUE)};
  const auto *requested_values{request.try_at("values"sv, HASH_VALUES)};
  if (requested_value == nullptr && requested_values == nullptr) {
    return true;
  }

  if (requested_value != nullptr && value == *requested_value) {
    return true;
  }

  if (requested_values != nullptr && requested_values->is_array()) {
    for (const auto &candidate : requested_values->as_array()) {
      if (candidate == value) {
        return true;
      }
    }
  }

  return false;
}

auto oidc_claims_parameter_accepts(const JSON &claims,
                                   const std::string_view target,
                                   const std::string_view claim,
                                   const JSON &value) -> bool {
  // An unrequested claim permits nothing, and what a present request permits
  // is a single question with a single answer, shared with the predicate that
  // takes the request directly
  const auto *specification{claim_specification(claims, target, claim)};
  return specification != nullptr &&
         oidc_claim_request_accepts(*specification, value);
}

auto oidc_claim_request_accepts_multi_valued(const JSON &request,
                                             const JSON &value) -> bool {
  // A set the caller belongs to, so belonging by any one of its members is
  // what the request asks about, and belonging to none is belonging to nothing
  if (value.is_array()) {
    for (const auto &member : value.as_array()) {
      if (claim_request_accepts_member(request, member)) {
        return true;
      }
    }

    return false;
  }

  return claim_request_accepts_member(request, value);
}

} // namespace sourcemeta::core
