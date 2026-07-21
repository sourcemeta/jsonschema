#include <sourcemeta/core/oauth_registration.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_error.h>

#include "oauth_json.h"
#include "oauth_syntax.h"

#include <chrono>      // std::chrono::seconds
#include <cstdint>     // std::int64_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_REDIRECT_URIS{JSON::Object::hash("redirect_uris"sv)};
const auto HASH_TOKEN_ENDPOINT_AUTH_METHOD{
    JSON::Object::hash("token_endpoint_auth_method"sv)};
const auto HASH_GRANT_TYPES{JSON::Object::hash("grant_types"sv)};
const auto HASH_RESPONSE_TYPES{JSON::Object::hash("response_types"sv)};
const auto HASH_CLIENT_NAME{JSON::Object::hash("client_name"sv)};
const auto HASH_CLIENT_URI{JSON::Object::hash("client_uri"sv)};
const auto HASH_LOGO_URI{JSON::Object::hash("logo_uri"sv)};
const auto HASH_SCOPE{JSON::Object::hash("scope"sv)};
const auto HASH_CONTACTS{JSON::Object::hash("contacts"sv)};
const auto HASH_TOS_URI{JSON::Object::hash("tos_uri"sv)};
const auto HASH_POLICY_URI{JSON::Object::hash("policy_uri"sv)};
const auto HASH_JWKS_URI{JSON::Object::hash("jwks_uri"sv)};
const auto HASH_JWKS{JSON::Object::hash("jwks"sv)};
const auto HASH_SOFTWARE_ID{JSON::Object::hash("software_id"sv)};
const auto HASH_SOFTWARE_VERSION{JSON::Object::hash("software_version"sv)};
const auto HASH_SOFTWARE_STATEMENT{JSON::Object::hash("software_statement"sv)};
const auto HASH_CLIENT_ID{JSON::Object::hash("client_id"sv)};
const auto HASH_CLIENT_SECRET{JSON::Object::hash("client_secret"sv)};
const auto HASH_CLIENT_ID_ISSUED_AT{
    JSON::Object::hash("client_id_issued_at"sv)};
const auto HASH_CLIENT_SECRET_EXPIRES_AT{
    JSON::Object::hash("client_secret_expires_at"sv)};
const auto HASH_REGISTRATION_ACCESS_TOKEN{
    JSON::Object::hash("registration_access_token"sv)};
const auto HASH_REGISTRATION_CLIENT_URI{
    JSON::Object::hash("registration_client_uri"sv)};
const auto HASH_ERROR{JSON::Object::hash("error"sv)};
const auto HASH_ERROR_DESCRIPTION{JSON::Object::hash("error_description"sv)};

auto array_member_contains(const JSON &data, const JSON::StringView name,
                           const JSON::Object::hash_type hash,
                           const JSON::StringView value) -> bool {
  if (!data.is_object()) {
    return false;
  }

  const auto *member{data.try_at(name, hash)};
  return member != nullptr && member->is_array() && member->contains(value);
}

auto array_member_is_present(const JSON &data, const JSON::StringView name,
                             const JSON::Object::hash_type hash) -> bool {
  const auto *member{data.try_at(name, hash)};
  return member != nullptr && member->is_array();
}

auto is_string_array(const JSON &value) -> bool {
  if (!value.is_array()) {
    return false;
  }

  for (const auto &element : value.as_array()) {
    if (!element.is_string()) {
      return false;
    }
  }

  return true;
}

// A membership predicate that falls back to the specification default when the
// array is absent, so an omitted list is read as the default value rather than
// as an empty registration (RFC 7591 Section 2)
auto array_member_contains_or_default(const JSON &data,
                                      const JSON::StringView name,
                                      const JSON::Object::hash_type hash,
                                      const std::string_view value,
                                      const std::string_view fallback) -> bool {
  if (!data.is_object()) {
    return value == fallback;
  }

  const auto *member{data.try_at(name, hash)};
  if (member == nullptr || !member->is_array()) {
    return value == fallback;
  }

  return member->contains(value);
}

auto validated_client_metadata(JSON &&data) -> JSON {
  if (!data.is_object()) {
    throw OAuthRegistrationParseError{};
  }

  // RFC 7591 Section 2: "The 'jwks_uri' and 'jwks' parameters MUST NOT both be
  // present in the same request or response"
  if (data.defines("jwks_uri"sv, HASH_JWKS_URI) &&
      data.defines("jwks"sv, HASH_JWKS)) {
    throw OAuthRegistrationParseError{};
  }

  // RFC 7591 Section 2: the grant and response types are arrays of strings and
  // the authentication method is a string. Each has a default accessor, so a
  // member present with the wrong type, down to a non-string array element, is
  // rejected here rather than silently read as its default or as fewer values
  // than were registered
  const auto *grant_types{data.try_at("grant_types"sv, HASH_GRANT_TYPES)};
  const auto *response_types{
      data.try_at("response_types"sv, HASH_RESPONSE_TYPES)};
  const auto *auth_method{data.try_at("token_endpoint_auth_method"sv,
                                      HASH_TOKEN_ENDPOINT_AUTH_METHOD)};
  if ((grant_types != nullptr && !is_string_array(*grant_types)) ||
      (response_types != nullptr && !is_string_array(*response_types)) ||
      (auth_method != nullptr && !auth_method->is_string())) {
    throw OAuthRegistrationParseError{};
  }

  return std::move(data);
}

auto assign_scalar_member(JSON &object, const JSON::StringView name,
                          const JSON::Object::hash_type hash,
                          const std::string_view value) -> void {
  if (!value.empty()) {
    object.assign_assume_new(std::string{name}, JSON{value}, hash);
  }
}

auto assign_array_member(JSON &object, const JSON::StringView name,
                         const JSON::Object::hash_type hash,
                         const std::span<const std::string_view> values)
    -> void {
  if (values.empty()) {
    return;
  }

  auto array{JSON::make_array()};
  for (const auto value : values) {
    array.push_back(JSON{value});
  }

  object.assign_assume_new(std::string{name}, std::move(array), hash);
}

// RFC 7519 Section 4.1: the registered claims describe the JSON Web Token that
// carries the software statement rather than the client, so they are not
// flattened onto the registration record as client metadata (RFC 7591
// Section 3.2.1)
auto is_jwt_structural_claim(const std::string_view name) -> bool {
  return name == "iss"sv || name == "sub"sv || name == "aud"sv ||
         name == "exp"sv || name == "nbf"sv || name == "iat"sv ||
         name == "jti"sv;
}

} // namespace

OAuthClientMetadata::OAuthClientMetadata(JSON &&data)
    : data_{validated_client_metadata(std::move(data))} {}

auto OAuthClientMetadata::from(JSON &&data)
    -> std::optional<OAuthClientMetadata> {
  try {
    return OAuthClientMetadata{std::move(data)};
  } catch (const OAuthRegistrationParseError &) {
    return std::nullopt;
  }
}

auto OAuthClientMetadata::has_redirect_uri(const std::string_view value) const
    -> bool {
  return array_member_contains(this->data_, "redirect_uris"sv,
                               HASH_REDIRECT_URIS, value);
}

auto OAuthClientMetadata::token_endpoint_auth_method() const
    -> std::string_view {
  const auto method{oauth_json_string_member(this->data_,
                                             "token_endpoint_auth_method"sv,
                                             HASH_TOKEN_ENDPOINT_AUTH_METHOD)};
  // RFC 7591 Section 2: "If unspecified or omitted, the default is
  // 'client_secret_basic'"
  return method.value_or("client_secret_basic"sv);
}

auto OAuthClientMetadata::supports_grant_type(
    const std::string_view value) const -> bool {
  // RFC 7591 Section 2: "If omitted, the default behavior is that the client
  // will use only the 'authorization_code' Grant Type"
  return array_member_contains_or_default(this->data_, "grant_types"sv,
                                          HASH_GRANT_TYPES, value,
                                          "authorization_code"sv);
}

auto OAuthClientMetadata::supports_response_type(
    const std::string_view value) const -> bool {
  // RFC 7591 Section 2: "If omitted, the default is that the client will use
  // only the 'code' response type"
  return array_member_contains_or_default(this->data_, "response_types"sv,
                                          HASH_RESPONSE_TYPES, value, "code"sv);
}

auto OAuthClientMetadata::client_name() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "client_name"sv,
                                  HASH_CLIENT_NAME);
}

auto OAuthClientMetadata::client_uri() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "client_uri"sv, HASH_CLIENT_URI);
}

auto OAuthClientMetadata::logo_uri() const -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "logo_uri"sv, HASH_LOGO_URI);
}

auto OAuthClientMetadata::scope() const -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "scope"sv, HASH_SCOPE);
}

auto OAuthClientMetadata::has_contact(const std::string_view value) const
    -> bool {
  return array_member_contains(this->data_, "contacts"sv, HASH_CONTACTS, value);
}

auto OAuthClientMetadata::tos_uri() const -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "tos_uri"sv, HASH_TOS_URI);
}

auto OAuthClientMetadata::policy_uri() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "policy_uri"sv, HASH_POLICY_URI);
}

auto OAuthClientMetadata::jwks_uri() const -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "jwks_uri"sv, HASH_JWKS_URI);
}

auto OAuthClientMetadata::software_id() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "software_id"sv,
                                  HASH_SOFTWARE_ID);
}

auto OAuthClientMetadata::software_version() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "software_version"sv,
                                  HASH_SOFTWARE_VERSION);
}

auto OAuthClientMetadata::software_statement() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "software_statement"sv,
                                  HASH_SOFTWARE_STATEMENT);
}

auto OAuthClientMetadata::client_id() const -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "client_id"sv, HASH_CLIENT_ID);
}

auto OAuthClientMetadata::client_secret() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "client_secret"sv,
                                  HASH_CLIENT_SECRET);
}

auto OAuthClientMetadata::client_id_issued_at() const
    -> std::optional<std::chrono::seconds> {
  return oauth_json_seconds_member(this->data_, "client_id_issued_at"sv,
                                   HASH_CLIENT_ID_ISSUED_AT);
}

auto OAuthClientMetadata::client_secret_expires_at() const
    -> std::optional<std::chrono::seconds> {
  return oauth_json_seconds_member(this->data_, "client_secret_expires_at"sv,
                                   HASH_CLIENT_SECRET_EXPIRES_AT);
}

auto OAuthClientMetadata::registration_access_token() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "registration_access_token"sv,
                                  HASH_REGISTRATION_ACCESS_TOKEN);
}

auto OAuthClientMetadata::registration_client_uri() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(this->data_, "registration_client_uri"sv,
                                  HASH_REGISTRATION_CLIENT_URI);
}

auto OAuthClientMetadata::data() const -> const JSON & { return this->data_; }

auto oauth_make_registration_request(
    const OAuthClientRegistrationConfig &config) -> std::optional<JSON> {
  // RFC 7591 Section 2: the client information and JWK Set locations MUST point
  // to valid URIs, so a malformed one yields a request the server would reject
  if ((!config.client_uri.empty() &&
       !oauth_try_parse_uri(config.client_uri).has_value()) ||
      (!config.logo_uri.empty() &&
       !oauth_try_parse_uri(config.logo_uri).has_value()) ||
      (!config.tos_uri.empty() &&
       !oauth_try_parse_uri(config.tos_uri).has_value()) ||
      (!config.policy_uri.empty() &&
       !oauth_try_parse_uri(config.policy_uri).has_value()) ||
      (!config.jwks_uri.empty() &&
       !oauth_try_parse_uri(config.jwks_uri).has_value())) {
    return std::nullopt;
  }

  // RFC 6749 Section 3.1.2: a redirection URI MUST be an absolute URI and MUST
  // NOT include a fragment, so a malformed or fragment-bearing one yields a
  // request the server would reject
  for (const auto redirect_uri : config.redirect_uris) {
    const auto parsed{oauth_try_parse_uri(redirect_uri)};
    if (!parsed.has_value() || parsed.value().fragment().has_value()) {
      return std::nullopt;
    }
  }

  // RFC 7591 Section 2: the JWK Set is a JSON object, and it "MUST NOT" be
  // present together with the JWK Set location
  if (config.jwks != nullptr &&
      (!config.jwks->is_object() || !config.jwks_uri.empty())) {
    return std::nullopt;
  }

  auto document{JSON::make_object()};
  assign_array_member(document, "redirect_uris", HASH_REDIRECT_URIS,
                      config.redirect_uris);
  if (config.jwks != nullptr) {
    document.assign_assume_new(std::string{"jwks"}, JSON{*config.jwks},
                               HASH_JWKS);
  }
  assign_scalar_member(document, "token_endpoint_auth_method",
                       HASH_TOKEN_ENDPOINT_AUTH_METHOD,
                       config.token_endpoint_auth_method);
  assign_array_member(document, "grant_types", HASH_GRANT_TYPES,
                      config.grant_types);
  assign_array_member(document, "response_types", HASH_RESPONSE_TYPES,
                      config.response_types);
  assign_scalar_member(document, "client_name", HASH_CLIENT_NAME,
                       config.client_name);
  assign_scalar_member(document, "client_uri", HASH_CLIENT_URI,
                       config.client_uri);
  assign_scalar_member(document, "logo_uri", HASH_LOGO_URI, config.logo_uri);
  assign_scalar_member(document, "scope", HASH_SCOPE, config.scope);
  assign_array_member(document, "contacts", HASH_CONTACTS, config.contacts);
  assign_scalar_member(document, "tos_uri", HASH_TOS_URI, config.tos_uri);
  assign_scalar_member(document, "policy_uri", HASH_POLICY_URI,
                       config.policy_uri);
  assign_scalar_member(document, "jwks_uri", HASH_JWKS_URI, config.jwks_uri);
  assign_scalar_member(document, "software_id", HASH_SOFTWARE_ID,
                       config.software_id);
  assign_scalar_member(document, "software_version", HASH_SOFTWARE_VERSION,
                       config.software_version);
  assign_scalar_member(document, "software_statement", HASH_SOFTWARE_STATEMENT,
                       config.software_statement);

  return document;
}

auto oauth_make_registration_error_response(
    const std::string_view error, const std::string_view error_description)
    -> JSON {
  auto response{JSON::make_object()};
  // RFC 7591 Section 3.2.2: error is REQUIRED and error_description is OPTIONAL
  response.assign_assume_new("error", JSON{error}, HASH_ERROR);
  if (!error_description.empty()) {
    response.assign_assume_new("error_description", JSON{error_description},
                               HASH_ERROR_DESCRIPTION);
  }

  return response;
}

auto oauth_registration_grant_response_consistent(
    const OAuthClientMetadata &metadata) -> bool {
  const auto &data{metadata.data()};
  const bool grant_types_present{
      data.is_object() &&
      array_member_is_present(data, "grant_types"sv, HASH_GRANT_TYPES)};
  const bool response_types_present{
      data.is_object() &&
      array_member_is_present(data, "response_types"sv, HASH_RESPONSE_TYPES)};
  const bool grants_authorization_code{array_member_contains(
      data, "grant_types"sv, HASH_GRANT_TYPES, "authorization_code"sv)};
  const bool grants_implicit{array_member_contains(
      data, "grant_types"sv, HASH_GRANT_TYPES, "implicit"sv)};
  const bool responds_code{array_member_contains(
      data, "response_types"sv, HASH_RESPONSE_TYPES, "code"sv)};
  const bool responds_token{array_member_contains(
      data, "response_types"sv, HASH_RESPONSE_TYPES, "token"sv)};

  // RFC 7591 Section 2.1: the authorization code grant pairs with the code
  // response type and the implicit grant with the token response type. Only
  // the explicitly registered lists are compared, so an omitted list, which
  // the server fills with a default, registers no value that could contradict
  // the other and a grant-only client that omits the response types stays
  // consistent
  if (response_types_present &&
      ((grants_authorization_code && !responds_code) ||
       (grants_implicit && !responds_token))) {
    return false;
  }

  if (grant_types_present && ((responds_code && !grants_authorization_code) ||
                              (responds_token && !grants_implicit))) {
    return false;
  }

  return true;
}

auto oauth_apply_software_statement_claims(JSON &metadata, const JSON &claims)
    -> bool {
  if (!metadata.is_object() || !claims.is_object()) {
    return false;
  }

  for (const auto &entry : claims.as_object()) {
    // RFC 7591 Section 3.2.1: the statement is echoed unmodified, so a claim
    // that happens to be named for it does not overwrite the echoed value
    if (is_jwt_structural_claim(entry.first) ||
        entry.first == "software_statement"sv) {
      continue;
    }

    // RFC 7591 Section 3.1.1: a client metadata value conveyed in a trusted
    // software statement takes precedence over the plain JSON value of the
    // same name, so it overwrites rather than augments the record
    metadata.assign(entry.first, entry.second);
  }

  return true;
}

auto oauth_make_registration_response(
    const JSON &metadata, const OAuthClientRegistrationResult &result)
    -> std::optional<JSON> {
  // RFC 7591 Section 3.2.1: the client identifier is REQUIRED, and the secret
  // and its expiry are returned together, so one without the other yields a
  // response this module's own reader would misread
  if (!metadata.is_object() || result.client_id.empty() ||
      (result.client_secret.empty() !=
       !result.client_secret_expires_at.has_value())) {
    return std::nullopt;
  }

  // RFC 7591 Section 3.2.1: the issue and expiry times are a number of seconds
  // since the epoch, so a negative value is not representable
  if ((result.client_id_issued_at.has_value() &&
       result.client_id_issued_at.value() < std::chrono::seconds::zero()) ||
      (result.client_secret_expires_at.has_value() &&
       result.client_secret_expires_at.value() <
           std::chrono::seconds::zero())) {
    return std::nullopt;
  }

  // RFC 7592 Section 3: the registration access token and its management
  // location are both REQUIRED in a management response, so one without the
  // other is not a well-formed response
  if (result.registration_access_token.empty() !=
      result.registration_client_uri.empty()) {
    return std::nullopt;
  }

  // RFC 7592 Section 3: the registration management location is a fully
  // qualified URL
  if (!result.registration_client_uri.empty() &&
      !oauth_try_parse_uri(result.registration_client_uri).has_value()) {
    return std::nullopt;
  }

  auto response{metadata};
  // RFC 7591 Section 3.2.1: these members are provisioned by the server, so any
  // value the accepted record carried for them is dropped and only the assigned
  // value is returned, leaving no client-supplied value to survive
  response.erase("client_id"sv);
  response.erase("client_secret"sv);
  response.erase("client_id_issued_at"sv);
  response.erase("client_secret_expires_at"sv);
  response.erase("registration_access_token"sv);
  response.erase("registration_client_uri"sv);

  response.assign_assume_new(std::string{"client_id"}, JSON{result.client_id},
                             HASH_CLIENT_ID);
  if (!result.client_secret.empty()) {
    response.assign_assume_new(std::string{"client_secret"},
                               JSON{result.client_secret}, HASH_CLIENT_SECRET);
  }

  if (result.client_id_issued_at.has_value()) {
    response.assign_assume_new(std::string{"client_id_issued_at"},
                               JSON{static_cast<std::int64_t>(
                                   result.client_id_issued_at.value().count())},
                               HASH_CLIENT_ID_ISSUED_AT);
  }

  if (result.client_secret_expires_at.has_value()) {
    response.assign_assume_new(
        std::string{"client_secret_expires_at"},
        JSON{static_cast<std::int64_t>(
            result.client_secret_expires_at.value().count())},
        HASH_CLIENT_SECRET_EXPIRES_AT);
  }

  if (!result.registration_access_token.empty()) {
    response.assign_assume_new(std::string{"registration_access_token"},
                               JSON{result.registration_access_token},
                               HASH_REGISTRATION_ACCESS_TOKEN);
  }

  if (!result.registration_client_uri.empty()) {
    response.assign_assume_new(std::string{"registration_client_uri"},
                               JSON{result.registration_client_uri},
                               HASH_REGISTRATION_CLIENT_URI);
  }

  return response;
}

auto oauth_make_registration_update_request(
    const OAuthClientRegistrationConfig &config,
    const std::string_view client_id, const std::string_view client_secret)
    -> std::optional<JSON> {
  // RFC 7592 Section 2.2: the current client identifier MUST be present
  if (client_id.empty()) {
    return std::nullopt;
  }

  auto document{oauth_make_registration_request(config)};
  if (!document.has_value()) {
    return std::nullopt;
  }

  // The registration request builder never writes these members, so they are
  // known-new on the freshly built object and their precomputed hashes apply
  document.value().assign_assume_new(std::string{"client_id"}, JSON{client_id},
                                     HASH_CLIENT_ID);
  // RFC 7592 Section 2.2: the client MAY include its current secret to be
  // matched against, but MUST NOT choose a new one, so an empty value is
  // omitted rather than sent as a chosen secret
  if (!client_secret.empty()) {
    document.value().assign_assume_new(std::string{"client_secret"},
                                       JSON{client_secret}, HASH_CLIENT_SECRET);
  }

  return document;
}

} // namespace sourcemeta::core
