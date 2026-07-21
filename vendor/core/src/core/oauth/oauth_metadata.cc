#include <sourcemeta/core/oauth_metadata.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_error.h>
#include <sourcemeta/core/uri.h>

#include "oauth_syntax.h"

#include <algorithm>   // std::ranges::find
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_ISSUER{JSON::Object::hash("issuer"sv)};
const auto HASH_AUTHORIZATION_ENDPOINT{
    JSON::Object::hash("authorization_endpoint"sv)};
const auto HASH_TOKEN_ENDPOINT{JSON::Object::hash("token_endpoint"sv)};
const auto HASH_REGISTRATION_ENDPOINT{
    JSON::Object::hash("registration_endpoint"sv)};
const auto HASH_PAR_ENDPOINT{
    JSON::Object::hash("pushed_authorization_request_endpoint"sv)};
const auto HASH_REQUIRE_PAR{
    JSON::Object::hash("require_pushed_authorization_requests"sv)};
const auto HASH_REVOCATION_ENDPOINT{
    JSON::Object::hash("revocation_endpoint"sv)};
const auto HASH_INTROSPECTION_ENDPOINT{
    JSON::Object::hash("introspection_endpoint"sv)};
const auto HASH_JWKS_URI{JSON::Object::hash("jwks_uri"sv)};
const auto HASH_RESPONSE_TYPES{
    JSON::Object::hash("response_types_supported"sv)};
const auto HASH_GRANT_TYPES{JSON::Object::hash("grant_types_supported"sv)};
const auto HASH_CODE_CHALLENGE_METHODS{
    JSON::Object::hash("code_challenge_methods_supported"sv)};
const auto HASH_TOKEN_AUTH_METHODS{
    JSON::Object::hash("token_endpoint_auth_methods_supported"sv)};
const auto HASH_TOKEN_AUTH_ALGS{
    JSON::Object::hash("token_endpoint_auth_signing_alg_values_supported"sv)};
const auto HASH_ISS_SUPPORTED{
    JSON::Object::hash("authorization_response_iss_parameter_supported"sv)};
const auto HASH_RESOURCE{JSON::Object::hash("resource"sv)};
const auto HASH_AUTHORIZATION_SERVERS{
    JSON::Object::hash("authorization_servers"sv)};
const auto HASH_BEARER_METHODS{
    JSON::Object::hash("bearer_methods_supported"sv)};
const auto HASH_SCOPES_SUPPORTED{JSON::Object::hash("scopes_supported"sv)};
const auto HASH_DPOP_BOUND_REQUIRED{
    JSON::Object::hash("dpop_bound_access_tokens_required"sv)};
const auto HASH_RESOURCE_SIGNING_ALGS{
    JSON::Object::hash("resource_signing_alg_values_supported"sv)};

auto string_member(const JSON &data, const JSON::StringView name,
                   const JSON::Object::hash_type hash)
    -> std::optional<std::string_view> {
  if (!data.is_object()) {
    return std::nullopt;
  }

  const auto *member{data.try_at(name, hash)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  return std::string_view{member->to_string()};
}

auto array_member_contains(const JSON &data, const JSON::StringView name,
                           const JSON::Object::hash_type hash,
                           const JSON::StringView value) -> bool {
  if (!data.is_object()) {
    return false;
  }

  const auto *member{data.try_at(name, hash)};
  return member != nullptr && member->is_array() && member->contains(value);
}

auto validated_server_metadata(JSON &&data, const std::string_view issuer)
    -> JSON {
  if (!data.is_object()) {
    throw OAuthMetadataParseError{};
  }

  const auto *issuer_member{data.try_at("issuer"sv, HASH_ISSUER)};
  if (issuer_member == nullptr || !issuer_member->is_string()) {
    throw OAuthMetadataParseError{};
  }

  // RFC 8414 Section 3.3: the issuer in the document must be identical by code
  // points to the one the document was retrieved for, the impersonation defense
  const auto issuer_value{std::string_view{issuer_member->to_string()}};
  if (issuer_value != issuer || !oauth_is_issuer_identifier(issuer_value)) {
    throw OAuthMetadataParseError{};
  }

  // RFC 8414 Section 2: response_types_supported is REQUIRED, and Section 3.2:
  // "Claims with zero elements MUST be omitted from the response", so a
  // present but empty array is a malformed document
  const auto *response_types{
      data.try_at("response_types_supported"sv, HASH_RESPONSE_TYPES)};
  if (response_types == nullptr || !response_types->is_array() ||
      response_types->empty()) {
    throw OAuthMetadataParseError{};
  }

  // RFC 8414 Section 2: a signing algorithm list is REQUIRED alongside the JWT
  // authentication methods, "none" MUST NOT appear in it, and Section 3.2
  // forbids a zero-element array, so an empty list is also invalid
  if (array_member_contains(data, "token_endpoint_auth_methods_supported"sv,
                            HASH_TOKEN_AUTH_METHODS, "private_key_jwt") ||
      array_member_contains(data, "token_endpoint_auth_methods_supported"sv,
                            HASH_TOKEN_AUTH_METHODS, "client_secret_jwt")) {
    const auto *algorithms{
        data.try_at("token_endpoint_auth_signing_alg_values_supported"sv,
                    HASH_TOKEN_AUTH_ALGS)};
    if (algorithms == nullptr || !algorithms->is_array() ||
        algorithms->empty() || algorithms->contains("none")) {
      throw OAuthMetadataParseError{};
    }
  }

  return std::move(data);
}

auto validated_resource_metadata(JSON &&data, const std::string_view resource)
    -> JSON {
  if (!data.is_object()) {
    throw OAuthMetadataParseError{};
  }

  const auto *resource_member{data.try_at("resource"sv, HASH_RESOURCE)};
  if (resource_member == nullptr || !resource_member->is_string()) {
    throw OAuthMetadataParseError{};
  }

  // RFC 9728 Section 3.3: the resource in the document must be a valid resource
  // identifier and identical by code points to the one the document was
  // retrieved for, the impersonation defense
  const auto resource_value{std::string_view{resource_member->to_string()}};
  if (resource_value != resource ||
      !oauth_is_resource_identifier(resource_value)) {
    throw OAuthMetadataParseError{};
  }

  // RFC 9728 Section 2: "none" MUST NOT appear in the resource signing
  // algorithms, and Section 3.2 forbids a zero-element array
  const auto *algorithms{data.try_at("resource_signing_alg_values_supported"sv,
                                     HASH_RESOURCE_SIGNING_ALGS)};
  if (algorithms != nullptr &&
      (!algorithms->is_array() || algorithms->empty() ||
       algorithms->contains("none"))) {
    throw OAuthMetadataParseError{};
  }

  return std::move(data);
}

} // namespace

auto oauth_well_known_url(const std::string_view identifier,
                          const OAuthWellKnownKind kind, std::string &sink)
    -> bool {
  static constexpr std::string_view scheme{"https://"};
  // Reject a missing scheme or a fragment, then require a non-empty host so an
  // authority such as ":443" with no host is not accepted (RFC 3986
  // Section 3.2)
  if (!identifier.starts_with(scheme) ||
      identifier.find('#') != std::string_view::npos) {
    return false;
  }

  const auto parsed{oauth_try_parse_uri(identifier)};
  if (!parsed.has_value() || !parsed->host().has_value() ||
      parsed->host().value().empty()) {
    return false;
  }

  const bool protected_resource{kind == OAuthWellKnownKind::ProtectedResource};
  const auto query_position{identifier.find('?')};
  // RFC 8414 Section 2: an issuer carries no query, unlike a protected resource
  if (query_position != std::string_view::npos && !protected_resource) {
    return false;
  }

  std::string_view suffix;
  switch (kind) {
    case OAuthWellKnownKind::AuthorizationServer:
      suffix = "oauth-authorization-server";
      break;
    case OAuthWellKnownKind::ProtectedResource:
      suffix = "oauth-protected-resource";
      break;
    case OAuthWellKnownKind::OpenIDConfigurationInserted:
    case OAuthWellKnownKind::OpenIDConfigurationAppended:
      suffix = "openid-configuration";
      break;
  }

  static constexpr std::string_view infix{"/.well-known/"};
  if (kind == OAuthWellKnownKind::OpenIDConfigurationAppended) {
    // RFC 8414 Section 5: the legacy OpenID Connect form appends the well-known
    // string after the path rather than inserting it
    auto base{identifier};
    if (base.ends_with('/')) {
      base.remove_suffix(1);
    }

    sink.reserve(sink.size() + base.size() + infix.size() + suffix.size());
    sink.append(base);
    sink.append(infix);
    sink.append(suffix);
    return true;
  }

  std::string_view query;
  auto without_query{identifier};
  if (query_position != std::string_view::npos) {
    query = identifier.substr(query_position);
    without_query = identifier.substr(0, query_position);
  }

  // RFC 8414 Section 3.1: the well-known string is inserted between the host
  // and the path, with any terminating slash on the path removed first
  auto authority{without_query};
  std::string_view path;
  const auto path_position{without_query.find('/', scheme.size())};
  if (path_position != std::string_view::npos) {
    authority = without_query.substr(0, path_position);
    path = without_query.substr(path_position);
  }

  if (path.ends_with('/')) {
    path.remove_suffix(1);
  }

  sink.reserve(sink.size() + authority.size() + infix.size() + suffix.size() +
               path.size() + query.size());
  sink.append(authority);
  sink.append(infix);
  sink.append(suffix);
  sink.append(path);
  sink.append(query);
  return true;
}

OAuthServerMetadata::OAuthServerMetadata(JSON &&data,
                                         const std::string_view issuer)
    : data_{validated_server_metadata(std::move(data), issuer)} {}

auto OAuthServerMetadata::from(JSON &&data, const std::string_view issuer)
    -> std::optional<OAuthServerMetadata> {
  try {
    return OAuthServerMetadata{std::move(data), issuer};
  } catch (const OAuthMetadataParseError &) {
    return std::nullopt;
  }
}

auto OAuthServerMetadata::issuer() const -> std::string_view {
  return string_member(this->data_, "issuer"sv, HASH_ISSUER).value();
}

auto OAuthServerMetadata::authorization_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "authorization_endpoint"sv,
                       HASH_AUTHORIZATION_ENDPOINT);
}

auto OAuthServerMetadata::token_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "token_endpoint"sv, HASH_TOKEN_ENDPOINT);
}

auto OAuthServerMetadata::registration_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "registration_endpoint"sv,
                       HASH_REGISTRATION_ENDPOINT);
}

auto OAuthServerMetadata::revocation_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "revocation_endpoint"sv,
                       HASH_REVOCATION_ENDPOINT);
}

auto OAuthServerMetadata::introspection_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "introspection_endpoint"sv,
                       HASH_INTROSPECTION_ENDPOINT);
}

auto OAuthServerMetadata::jwks_uri() const -> std::optional<std::string_view> {
  return string_member(this->data_, "jwks_uri"sv, HASH_JWKS_URI);
}

auto OAuthServerMetadata::pushed_authorization_request_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "pushed_authorization_request_endpoint"sv,
                       HASH_PAR_ENDPOINT);
}

auto OAuthServerMetadata::require_pushed_authorization_requests() const
    -> bool {
  if (!this->data_.is_object()) {
    return false;
  }

  const auto *member{this->data_.try_at(
      "require_pushed_authorization_requests"sv, HASH_REQUIRE_PAR)};
  return member != nullptr && member->is_boolean() && member->to_boolean();
}

auto OAuthServerMetadata::authorization_response_iss_parameter_supported() const
    -> bool {
  if (!this->data_.is_object()) {
    return false;
  }

  const auto *member{this->data_.try_at(
      "authorization_response_iss_parameter_supported"sv, HASH_ISS_SUPPORTED)};
  return member != nullptr && member->is_boolean() && member->to_boolean();
}

auto OAuthServerMetadata::supports_response_type(
    const std::string_view value) const -> bool {
  return array_member_contains(this->data_, "response_types_supported"sv,
                               HASH_RESPONSE_TYPES, value);
}

auto OAuthServerMetadata::supports_grant_type(
    const std::string_view value) const -> bool {
  const auto *member{
      this->data_.is_object()
          ? this->data_.try_at("grant_types_supported"sv, HASH_GRANT_TYPES)
          : nullptr};
  if (member == nullptr || !member->is_array()) {
    // RFC 8414 Section 2: the default is the authorization code and implicit
    // grants
    return value == "authorization_code" || value == "implicit";
  }

  return member->contains(value);
}

auto OAuthServerMetadata::supports_code_challenge_method(
    const std::string_view value) const -> bool {
  // RFC 8414 Section 2: an omitted list means PKCE is not supported, so there
  // is no default to fall back to
  return array_member_contains(this->data_,
                               "code_challenge_methods_supported"sv,
                               HASH_CODE_CHALLENGE_METHODS, value);
}

auto OAuthServerMetadata::supports_token_endpoint_auth_method(
    const std::string_view value) const -> bool {
  const auto *member{
      this->data_.is_object()
          ? this->data_.try_at("token_endpoint_auth_methods_supported"sv,
                               HASH_TOKEN_AUTH_METHODS)
          : nullptr};
  if (member == nullptr || !member->is_array()) {
    // RFC 8414 Section 2: the default is client_secret_basic
    return value == "client_secret_basic";
  }

  return member->contains(value);
}

auto OAuthServerMetadata::data() const -> const JSON & { return this->data_; }

OAuthResourceMetadata::OAuthResourceMetadata(JSON &&data,
                                             const std::string_view resource)
    : data_{validated_resource_metadata(std::move(data), resource)} {}

auto OAuthResourceMetadata::from(JSON &&data, const std::string_view resource)
    -> std::optional<OAuthResourceMetadata> {
  try {
    return OAuthResourceMetadata{std::move(data), resource};
  } catch (const OAuthMetadataParseError &) {
    return std::nullopt;
  }
}

auto OAuthResourceMetadata::resource() const -> std::string_view {
  return string_member(this->data_, "resource"sv, HASH_RESOURCE).value();
}

auto OAuthResourceMetadata::first_authorization_server() const
    -> std::optional<std::string_view> {
  if (!this->data_.is_object()) {
    return std::nullopt;
  }

  const auto *member{this->data_.try_at("authorization_servers"sv,
                                        HASH_AUTHORIZATION_SERVERS)};
  if (member == nullptr || !member->is_array()) {
    return std::nullopt;
  }

  for (const auto &element : member->as_array()) {
    if (element.is_string()) {
      return std::string_view{element.to_string()};
    }
  }

  return std::nullopt;
}

auto OAuthResourceMetadata::supports_authorization_server(
    const std::string_view value) const -> bool {
  return array_member_contains(this->data_, "authorization_servers"sv,
                               HASH_AUTHORIZATION_SERVERS, value);
}

auto OAuthResourceMetadata::jwks_uri() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "jwks_uri"sv, HASH_JWKS_URI);
}

auto OAuthResourceMetadata::supports_bearer_method(
    const std::string_view value) const -> bool {
  return array_member_contains(this->data_, "bearer_methods_supported"sv,
                               HASH_BEARER_METHODS, value);
}

auto OAuthResourceMetadata::supports_scope(const std::string_view value) const
    -> bool {
  return array_member_contains(this->data_, "scopes_supported"sv,
                               HASH_SCOPES_SUPPORTED, value);
}

auto OAuthResourceMetadata::dpop_bound_access_tokens_required() const -> bool {
  if (!this->data_.is_object()) {
    return false;
  }

  const auto *member{this->data_.try_at("dpop_bound_access_tokens_required"sv,
                                        HASH_DPOP_BOUND_REQUIRED)};
  return member != nullptr && member->is_boolean() && member->to_boolean();
}

auto OAuthResourceMetadata::data() const -> const JSON & { return this->data_; }

namespace {

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
  // RFC 8414 Section 3.2: "Claims with zero elements MUST be omitted", so an
  // empty list is not emitted
  if (values.empty()) {
    return;
  }

  auto array{JSON::make_array()};
  for (const auto value : values) {
    array.push_back(JSON{value});
  }

  object.assign_assume_new(std::string{name}, std::move(array), hash);
}

auto span_contains(const std::span<const std::string_view> values,
                   const std::string_view target) -> bool {
  return std::ranges::find(values, target) != values.end();
}

} // namespace

auto oauth_make_server_metadata(const OAuthServerMetadataConfig &config)
    -> std::optional<JSON> {
  // RFC 8414 Section 2: issuer and a non-empty response_types_supported are
  // REQUIRED, and the issuer must be a valid issuer identifier
  if (!oauth_is_issuer_identifier(config.issuer) ||
      config.response_types_supported.empty()) {
    return std::nullopt;
  }

  // RFC 8414 Section 2: the authorization endpoint is REQUIRED once a response
  // type is advertised, and the token endpoint unless the only grant type is
  // the implicit one, and every advertised URL is an https location. A present
  // scalar that is not a valid https URL, or a missing required endpoint, would
  // yield an unusable discovery document
  const bool token_endpoint_needed{
      config.grant_types_supported.empty() ||
      std::ranges::any_of(config.grant_types_supported,
                          [](const std::string_view grant) -> bool {
                            return grant != "implicit";
                          })};
  const bool token_endpoint_required_and_valid{
      token_endpoint_needed
          ? oauth_is_resource_identifier(config.token_endpoint)
          : config.token_endpoint.empty() ||
                oauth_is_resource_identifier(config.token_endpoint)};
  if (!oauth_is_resource_identifier(config.authorization_endpoint) ||
      !token_endpoint_required_and_valid ||
      (!config.registration_endpoint.empty() &&
       !oauth_is_resource_identifier(config.registration_endpoint)) ||
      (!config.pushed_authorization_request_endpoint.empty() &&
       !oauth_is_resource_identifier(
           config.pushed_authorization_request_endpoint)) ||
      (!config.jwks_uri.empty() &&
       !oauth_is_resource_identifier(config.jwks_uri))) {
    return std::nullopt;
  }

  // RFC 9126 Section 5: requiring pushed authorization requests without
  // advertising the endpoint to submit them to yields a document a client
  // cannot comply with
  if (config.require_pushed_authorization_requests &&
      config.pushed_authorization_request_endpoint.empty()) {
    return std::nullopt;
  }

  // RFC 8414 Section 2: "none" MUST NOT appear in the signing algorithm list
  // unconditionally, the list is REQUIRED alongside the JWT authentication
  // methods, and Section 3.2 forbids a zero-element array, so the document
  // would otherwise be rejected by its own parser
  if (span_contains(config.token_endpoint_auth_signing_alg_values_supported,
                    "none")) {
    return std::nullopt;
  }

  if ((span_contains(config.token_endpoint_auth_methods_supported,
                     "private_key_jwt") ||
       span_contains(config.token_endpoint_auth_methods_supported,
                     "client_secret_jwt")) &&
      config.token_endpoint_auth_signing_alg_values_supported.empty()) {
    return std::nullopt;
  }

  auto document{JSON::make_object()};
  document.assign_assume_new("issuer", JSON{config.issuer}, HASH_ISSUER);
  assign_scalar_member(document, "authorization_endpoint",
                       HASH_AUTHORIZATION_ENDPOINT,
                       config.authorization_endpoint);
  assign_scalar_member(document, "token_endpoint", HASH_TOKEN_ENDPOINT,
                       config.token_endpoint);
  assign_scalar_member(document, "registration_endpoint",
                       HASH_REGISTRATION_ENDPOINT,
                       config.registration_endpoint);
  assign_scalar_member(document, "pushed_authorization_request_endpoint",
                       HASH_PAR_ENDPOINT,
                       config.pushed_authorization_request_endpoint);
  assign_scalar_member(document, "jwks_uri", HASH_JWKS_URI, config.jwks_uri);
  assign_array_member(document, "response_types_supported", HASH_RESPONSE_TYPES,
                      config.response_types_supported);
  assign_array_member(document, "grant_types_supported", HASH_GRANT_TYPES,
                      config.grant_types_supported);
  assign_array_member(document, "code_challenge_methods_supported",
                      HASH_CODE_CHALLENGE_METHODS,
                      config.code_challenge_methods_supported);
  assign_array_member(document, "token_endpoint_auth_methods_supported",
                      HASH_TOKEN_AUTH_METHODS,
                      config.token_endpoint_auth_methods_supported);
  assign_array_member(document,
                      "token_endpoint_auth_signing_alg_values_supported",
                      HASH_TOKEN_AUTH_ALGS,
                      config.token_endpoint_auth_signing_alg_values_supported);
  assign_array_member(document, "scopes_supported", HASH_SCOPES_SUPPORTED,
                      config.scopes_supported);
  // RFC 9126 Section 5: the default is false, so the flag is emitted only when
  // the server requires pushed authorization requests
  if (config.require_pushed_authorization_requests) {
    document.assign_assume_new("require_pushed_authorization_requests",
                               JSON{true}, HASH_REQUIRE_PAR);
  }

  return document;
}

} // namespace sourcemeta::core
