#include <sourcemeta/core/oauth_metadata.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_error.h>
#include <sourcemeta/core/uri.h>

#include "oauth_syntax.h"

#include <algorithm> // std::ranges::find, std::ranges::any_of, std::ranges::all_of
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_ISSUER{JSON::Object::hash("issuer"sv)};
constexpr auto HASH_AUTHORIZATION_ENDPOINT{
    JSON::Object::hash("authorization_endpoint"sv)};
constexpr auto HASH_TOKEN_ENDPOINT{JSON::Object::hash("token_endpoint"sv)};
constexpr auto HASH_REGISTRATION_ENDPOINT{
    JSON::Object::hash("registration_endpoint"sv)};
constexpr auto HASH_PAR_ENDPOINT{
    JSON::Object::hash("pushed_authorization_request_endpoint"sv)};
constexpr auto HASH_REQUIRE_PAR{
    JSON::Object::hash("require_pushed_authorization_requests"sv)};
constexpr auto HASH_DEVICE_AUTHORIZATION_ENDPOINT{
    JSON::Object::hash("device_authorization_endpoint"sv)};
constexpr auto HASH_REVOCATION_ENDPOINT{
    JSON::Object::hash("revocation_endpoint"sv)};
constexpr auto HASH_INTROSPECTION_ENDPOINT{
    JSON::Object::hash("introspection_endpoint"sv)};
constexpr auto HASH_JWKS_URI{JSON::Object::hash("jwks_uri"sv)};
constexpr auto HASH_RESPONSE_TYPES{
    JSON::Object::hash("response_types_supported"sv)};
constexpr auto HASH_GRANT_TYPES{JSON::Object::hash("grant_types_supported"sv)};
constexpr auto HASH_CODE_CHALLENGE_METHODS{
    JSON::Object::hash("code_challenge_methods_supported"sv)};
constexpr auto HASH_TOKEN_AUTH_METHODS{
    JSON::Object::hash("token_endpoint_auth_methods_supported"sv)};
constexpr auto HASH_TOKEN_AUTH_ALGS{
    JSON::Object::hash("token_endpoint_auth_signing_alg_values_supported"sv)};
constexpr auto HASH_ISS_SUPPORTED{
    JSON::Object::hash("authorization_response_iss_parameter_supported"sv)};
constexpr auto HASH_RESOURCE{JSON::Object::hash("resource"sv)};
constexpr auto HASH_AUTHORIZATION_SERVERS{
    JSON::Object::hash("authorization_servers"sv)};
constexpr auto HASH_BEARER_METHODS{
    JSON::Object::hash("bearer_methods_supported"sv)};
constexpr auto HASH_SCOPES_SUPPORTED{JSON::Object::hash("scopes_supported"sv)};
constexpr auto HASH_DPOP_BOUND_REQUIRED{
    JSON::Object::hash("dpop_bound_access_tokens_required"sv)};
constexpr auto HASH_RESOURCE_SIGNING_ALGS{
    JSON::Object::hash("resource_signing_alg_values_supported"sv)};
constexpr auto HASH_RESOURCE_NAME{JSON::Object::hash("resource_name"sv)};
constexpr auto HASH_RESOURCE_DOCUMENTATION{
    JSON::Object::hash("resource_documentation"sv)};
constexpr auto HASH_RESOURCE_POLICY_URI{
    JSON::Object::hash("resource_policy_uri"sv)};
constexpr auto HASH_RESOURCE_TOS_URI{JSON::Object::hash("resource_tos_uri"sv)};
constexpr auto HASH_TLS_CLIENT_CERTIFICATE_BOUND{
    JSON::Object::hash("tls_client_certificate_bound_access_tokens"sv)};
constexpr auto HASH_AUTHORIZATION_DETAILS_TYPES{
    JSON::Object::hash("authorization_details_types_supported"sv)};
constexpr auto HASH_DPOP_SIGNING_ALGS{
    JSON::Object::hash("dpop_signing_alg_values_supported"sv)};
constexpr auto HASH_PROTECTED_RESOURCES{
    JSON::Object::hash("protected_resources"sv)};

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

// An advertised endpoint is a location the client dereferences with its
// credentials, so a document naming a cleartext one would direct those
// credentials there. A member that is present but is not a valid https URL
// fails the parse rather than being ignored, since an accessor would otherwise
// report a malformed member as an absent one
auto validate_endpoint(const JSON &data, const JSON::StringView name,
                       const JSON::Object::hash_type hash) -> void {
  const auto *member{data.try_at(name, hash)};
  if (member == nullptr) {
    return;
  }

  if (!member->is_string() || !oauth_is_endpoint_url(member->to_string())) {
    throw OAuthMetadataParseError{};
  }
}

// The shape checks run on exact name lookup only, since RFC 9728 Section 3.2
// demands that "any metadata parameters that are not understood MUST be
// ignored", which covers the language-tagged variants of the human-readable
// members (RFC 9728 Section 2.1) whose names are distinct from the untagged
// ones. A member that is present with the wrong shape fails the parse rather
// than being ignored, since an accessor would otherwise report a malformed
// member as an absent one
auto validate_string_array(const JSON &data, const JSON::StringView name,
                           const JSON::Object::hash_type hash,
                           const bool allow_empty) -> void {
  const auto *member{data.try_at(name, hash)};
  if (member == nullptr) {
    return;
  }

  if (!member->is_array() || (!allow_empty && member->empty())) {
    throw OAuthMetadataParseError{};
  }

  for (const auto &element : member->as_array()) {
    if (!element.is_string()) {
      throw OAuthMetadataParseError{};
    }
  }
}

auto validate_boolean(const JSON &data, const JSON::StringView name,
                      const JSON::Object::hash_type hash) -> void {
  const auto *member{data.try_at(name, hash)};
  if (member != nullptr && !member->is_boolean()) {
    throw OAuthMetadataParseError{};
  }
}

auto validate_string(const JSON &data, const JSON::StringView name,
                     const JSON::Object::hash_type hash) -> void {
  const auto *member{data.try_at(name, hash)};
  if (member != nullptr && !member->is_string()) {
    throw OAuthMetadataParseError{};
  }
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

  // RFC 8414 Section 3.3: the issuer in the document "MUST be identical to the
  // authorization server's issuer identifier value into which the well-known
  // URI string was inserted", the impersonation defense, and Section 4 fixes
  // how that comparison runs, as "a Unicode code-point-to-code-point equality
  // comparison"
  const auto issuer_value{std::string_view{issuer_member->to_string()}};
  if (issuer_value != issuer || !oauth_is_issuer_identifier(issuer_value)) {
    throw OAuthMetadataParseError{};
  }

  // Each advertised endpoint carries its own transport requirement, and the
  // document arrives from the party the client is still deciding whether to
  // trust, so the rule is enforced when the document is read and not only when
  // one is built. Some of these sources mandate the scheme outright and the
  // rest mandate the transport, which over HTTP is the same demand. RFC 8414
  // Section 2 on jwks_uri: "This URL MUST use the "https" scheme". RFC 9126
  // Section 2: "The PAR endpoint URL MUST use the "https" scheme". RFC 7009
  // Section 2: "URLs for token revocation endpoints MUST be HTTPS URLs" and,
  // addressed at this side of the exchange, "Clients MUST verify that the URL
  // is an HTTPS URL". RFC 6749 Section 3.1 and Section 3.2: "the authorization
  // server MUST require the use of TLS" for the authorization and token
  // endpoints. RFC 7591 Section 3: the "registration endpoint MUST be protected
  // by a transport-layer security mechanism". RFC 7662 Section 2: "The
  // introspection endpoint MUST be protected by a transport-layer security
  // mechanism". RFC 8628 Section 3.1 on the device authorization endpoint:
  // "All requests from the device MUST use the Transport Layer Security (TLS)
  // protocol", and the client authentication rules of RFC 6749 Section 3.2.1
  // apply there too, so it carries credentials just as the token endpoint does
  validate_endpoint(data, "authorization_endpoint"sv,
                    HASH_AUTHORIZATION_ENDPOINT);
  validate_endpoint(data, "token_endpoint"sv, HASH_TOKEN_ENDPOINT);
  validate_endpoint(data, "registration_endpoint"sv,
                    HASH_REGISTRATION_ENDPOINT);
  validate_endpoint(data, "pushed_authorization_request_endpoint"sv,
                    HASH_PAR_ENDPOINT);
  validate_endpoint(data, "device_authorization_endpoint"sv,
                    HASH_DEVICE_AUTHORIZATION_ENDPOINT);
  validate_endpoint(data, "revocation_endpoint"sv, HASH_REVOCATION_ENDPOINT);
  validate_endpoint(data, "introspection_endpoint"sv,
                    HASH_INTROSPECTION_ENDPOINT);
  validate_endpoint(data, "jwks_uri"sv, HASH_JWKS_URI);

  // RFC 8414 Section 2: response_types_supported is REQUIRED (unconditionally,
  // unlike authorization_endpoint which is conditional), and Section 3.2:
  // "Claims with zero elements MUST be omitted from the response", so a present
  // but empty array is a malformed document
  const auto *response_types{
      data.try_at("response_types_supported"sv, HASH_RESPONSE_TYPES)};
  if (response_types == nullptr || !response_types->is_array() ||
      response_types->empty()) {
    throw OAuthMetadataParseError{};
  }

  // RFC 8414 Section 2: a signing algorithm list is REQUIRED alongside the JWT
  // authentication methods, "none" MUST NOT appear in it, and Section 3.2
  // forbids a zero-element array, so an empty list is also invalid
  if (data.array_member_contains("token_endpoint_auth_methods_supported"sv,
                                 HASH_TOKEN_AUTH_METHODS, "private_key_jwt") ||
      data.array_member_contains("token_endpoint_auth_methods_supported"sv,
                                 HASH_TOKEN_AUTH_METHODS,
                                 "client_secret_jwt")) {
    const auto *algorithms{
        data.try_at("token_endpoint_auth_signing_alg_values_supported"sv,
                    HASH_TOKEN_AUTH_ALGS)};
    if (algorithms == nullptr || !algorithms->is_array() ||
        algorithms->empty() || algorithms->contains("none")) {
      throw OAuthMetadataParseError{};
    }
  }

  // RFC 9728 Section 4: the protected resources are "resource identifiers for
  // OAuth protected resources that can be used with this authorization
  // server", the Section 1.2 form, so an entry that is not one is rejected
  // like an invalid advertised issuer, with the scheme case-insensitive for a
  // received value per RFC 3986 Section 3.1. RFC 8414 Section 3.2 forbids a
  // zero-element array
  validate_string_array(data, "protected_resources"sv, HASH_PROTECTED_RESOURCES,
                        false);
  const auto *protected_resources{
      data.try_at("protected_resources"sv, HASH_PROTECTED_RESOURCES)};
  if (protected_resources != nullptr) {
    for (const auto &element : protected_resources->as_array()) {
      if (!oauth_is_advertised_resource(element.to_string())) {
        throw OAuthMetadataParseError{};
      }
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

  // RFC 9728 Section 3.3: the resource in the document "MUST be identical to
  // the protected resource's resource identifier value into which the
  // well-known URI path suffix was inserted", the impersonation defense, and
  // Section 6 fixes how that comparison runs, as "a Unicode
  // code-point-to-code-point equality comparison". It must also be a valid
  // resource identifier in the first place
  const auto resource_value{std::string_view{resource_member->to_string()}};
  if (resource_value != resource ||
      !oauth_is_resource_identifier(resource_value)) {
    throw OAuthMetadataParseError{};
  }

  // RFC 9728 Section 2 on jwks_uri: "This URL MUST use the https scheme"
  validate_endpoint(data, "jwks_uri"sv, HASH_JWKS_URI);

  // RFC 9728 Section 2: the authorization servers are "OAuth authorization
  // server issuer identifiers, as defined in [RFC8414]", and each one is where
  // the client starts its next discovery request, so an entry that is not a
  // valid issuer identifier is rejected rather than handed onwards. RFC 8414
  // Section 2 requires the https scheme, a host, and no query or fragment,
  // and the scheme of a received value stays case-insensitive per RFC 3986
  // Section 3.1 since an advertised issuer is matched against nothing at parse
  // time. Section 3.2: "Parameters with zero values MUST be omitted from the
  // response", so a present but empty array is a malformed document
  const auto *authorization_servers{
      data.try_at("authorization_servers"sv, HASH_AUTHORIZATION_SERVERS)};
  if (authorization_servers != nullptr) {
    if (!authorization_servers->is_array() || authorization_servers->empty()) {
      throw OAuthMetadataParseError{};
    }

    for (const auto &element : authorization_servers->as_array()) {
      if (!element.is_string() ||
          !oauth_is_advertised_issuer(element.to_string())) {
        throw OAuthMetadataParseError{};
      }
    }
  }

  // RFC 9728 Section 2: "none" MUST NOT appear in the resource signing
  // algorithms, and Section 3.2 forbids a zero-element array
  const auto *algorithms{data.try_at("resource_signing_alg_values_supported"sv,
                                     HASH_RESOURCE_SIGNING_ALGS)};
  if (algorithms != nullptr) {
    if (!algorithms->is_array() || algorithms->empty() ||
        algorithms->contains("none")) {
      throw OAuthMetadataParseError{};
    }

    for (const auto &element : algorithms->as_array()) {
      if (!element.is_string()) {
        throw OAuthMetadataParseError{};
      }
    }
  }

  // RFC 9728 Section 3.2: "Parameters with zero values MUST be omitted from
  // the response" is a rule on the emitting side, and no text orders a reader
  // to reject a violating document, so treating one as malformed is this
  // module's strictness stance, the same one the authorization server list
  // already receives. The bearer method list is the lone member whose empty
  // array is meaningful, since RFC 9728 Section 2 says "The empty array []
  // can be used to indicate that no bearer methods are supported"
  validate_string_array(data, "scopes_supported"sv, HASH_SCOPES_SUPPORTED,
                        false);
  validate_string_array(data, "bearer_methods_supported"sv, HASH_BEARER_METHODS,
                        true);
  validate_string_array(data, "authorization_details_types_supported"sv,
                        HASH_AUTHORIZATION_DETAILS_TYPES, false);
  validate_string_array(data, "dpop_signing_alg_values_supported"sv,
                        HASH_DPOP_SIGNING_ALGS, false);
  validate_boolean(data, "tls_client_certificate_bound_access_tokens"sv,
                   HASH_TLS_CLIENT_CERTIFICATE_BOUND);
  validate_boolean(data, "dpop_bound_access_tokens_required"sv,
                   HASH_DPOP_BOUND_REQUIRED);
  validate_string(data, "resource_name"sv, HASH_RESOURCE_NAME);
  validate_string(data, "resource_documentation"sv,
                  HASH_RESOURCE_DOCUMENTATION);
  validate_string(data, "resource_policy_uri"sv, HASH_RESOURCE_POLICY_URI);
  validate_string(data, "resource_tos_uri"sv, HASH_RESOURCE_TOS_URI);

  return std::move(data);
}

} // namespace

// RFC 6749 Section 3.1: "the authorization server MUST require the use of TLS
// as described in Section 1.6 when sending requests to the authorization
// endpoint", which over HTTP means the https scheme, and "The endpoint URI MUST
// NOT include a fragment component", while it "MAY include an
// "application/x-www-form-urlencoded" formatted (per Appendix B) query
// component". Unlike an identifier, which RFC 8414 Section 4 compares as "a
// Unicode code-point-to-code-point equality comparison", an endpoint is a
// location to dereference, so RFC 3986 Section 3.1 governs and makes its scheme
// case-insensitive
auto oauth_is_endpoint_url(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->is_https() && uri->host().has_value() &&
         !uri->host().value().empty() && !uri->fragment().has_value();
}

// RFC 9728 Section 1.2 and RFC 8707 Section 2: a resource is an https URL with
// a non-empty host and no fragment, a query tolerated unlike an issuer
auto oauth_is_resource_identifier(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->scheme().has_value() &&
         uri->scheme().value() == "https" && uri->host().has_value() &&
         !uri->host().value().empty() && !uri->fragment().has_value();
}

// RFC 8414 Section 2: an issuer is an https URL with a non-empty host (RFC 3986
// Section 3.2) and no query or fragment, its scheme matched by code points to
// reject a non-canonical case
auto oauth_is_issuer_identifier(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->scheme().has_value() &&
         uri->scheme().value() == "https" && uri->host().has_value() &&
         !uri->host().value().empty() && !uri->query().has_value() &&
         !uri->fragment().has_value();
}

auto oauth_well_known_url(const std::string_view identifier,
                          const OAuthWellKnownKind kind, std::string &sink)
    -> bool {
  static constexpr std::string_view SCHEME{"https://"};
  // Reject a missing scheme or a fragment, then require a non-empty host so an
  // authority such as ":443" with no host is not accepted (RFC 3986
  // Section 3.2)
  if (!identifier.starts_with(SCHEME) || identifier.contains('#')) {
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

  static constexpr std::string_view INFIX{"/.well-known/"};
  if (kind == OAuthWellKnownKind::OpenIDConfigurationAppended) {
    // RFC 8414 Section 5: the legacy OpenID Connect form appends the well-known
    // string after the path rather than inserting it
    auto base{identifier};
    if (base.ends_with('/')) {
      base.remove_suffix(1);
    }

    sink.reserve(sink.size() + base.size() + INFIX.size() + suffix.size());
    sink.append(base);
    sink.append(INFIX);
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
  const auto path_position{without_query.find('/', SCHEME.size())};
  if (path_position != std::string_view::npos) {
    authority = without_query.substr(0, path_position);
    path = without_query.substr(path_position);
  }

  if (path.ends_with('/')) {
    path.remove_suffix(1);
  }

  sink.reserve(sink.size() + authority.size() + INFIX.size() + suffix.size() +
               path.size() + query.size());
  sink.append(authority);
  sink.append(INFIX);
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

auto OAuthServerMetadata::device_authorization_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "device_authorization_endpoint"sv,
                       HASH_DEVICE_AUTHORIZATION_ENDPOINT);
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
  return this->data_.array_member_contains("response_types_supported"sv,
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
  return this->data_.array_member_contains("code_challenge_methods_supported"sv,
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

auto OAuthServerMetadata::supports_protected_resource(
    const std::string_view value) const -> bool {
  return this->data_.array_member_contains("protected_resources"sv,
                                           HASH_PROTECTED_RESOURCES, value);
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
  if (member == nullptr) {
    return std::nullopt;
  }

  // Construction rejects a document whose authorization servers are anything
  // other than a non-empty array of issuer identifiers, so the first element is
  // a string and needs no search past a malformed one
  return std::string_view{member->front().to_string()};
}

auto OAuthResourceMetadata::supports_authorization_server(
    const std::string_view value) const -> bool {
  return this->data_.array_member_contains("authorization_servers"sv,
                                           HASH_AUTHORIZATION_SERVERS, value);
}

auto OAuthResourceMetadata::jwks_uri() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "jwks_uri"sv, HASH_JWKS_URI);
}

auto OAuthResourceMetadata::supports_bearer_method(
    const std::string_view value) const -> bool {
  return this->data_.array_member_contains("bearer_methods_supported"sv,
                                           HASH_BEARER_METHODS, value);
}

auto OAuthResourceMetadata::supports_scope(const std::string_view value) const
    -> bool {
  return this->data_.array_member_contains("scopes_supported"sv,
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

auto OAuthResourceMetadata::resource_name() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "resource_name"sv, HASH_RESOURCE_NAME);
}

auto OAuthResourceMetadata::resource_documentation() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "resource_documentation"sv,
                       HASH_RESOURCE_DOCUMENTATION);
}

auto OAuthResourceMetadata::resource_policy_uri() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "resource_policy_uri"sv,
                       HASH_RESOURCE_POLICY_URI);
}

auto OAuthResourceMetadata::resource_tos_uri() const
    -> std::optional<std::string_view> {
  return string_member(this->data_, "resource_tos_uri"sv,
                       HASH_RESOURCE_TOS_URI);
}

auto OAuthResourceMetadata::tls_client_certificate_bound_access_tokens() const
    -> bool {
  if (!this->data_.is_object()) {
    return false;
  }

  const auto *member{
      this->data_.try_at("tls_client_certificate_bound_access_tokens"sv,
                         HASH_TLS_CLIENT_CERTIFICATE_BOUND)};
  return member != nullptr && member->is_boolean() && member->to_boolean();
}

auto OAuthResourceMetadata::supports_resource_signing_alg(
    const std::string_view value) const -> bool {
  return this->data_.array_member_contains(
      "resource_signing_alg_values_supported"sv, HASH_RESOURCE_SIGNING_ALGS,
      value);
}

auto OAuthResourceMetadata::supports_dpop_signing_alg(
    const std::string_view value) const -> bool {
  return this->data_.array_member_contains(
      "dpop_signing_alg_values_supported"sv, HASH_DPOP_SIGNING_ALGS, value);
}

auto OAuthResourceMetadata::supports_authorization_details_type(
    const std::string_view value) const -> bool {
  return this->data_.array_member_contains(
      "authorization_details_types_supported"sv,
      HASH_AUTHORIZATION_DETAILS_TYPES, value);
}

auto OAuthResourceMetadata::data() const -> const JSON & { return this->data_; }

namespace {

auto span_contains(const std::span<const std::string_view> values,
                   const std::string_view target) -> bool {
  return std::ranges::find(values, target) != values.end();
}

// RFC 9728 Section 2 calls each human-readable page location a URL without
// mandating a scheme, so the check is for an absolute URI rather than an https
// one, and a fragment stays legitimate on a page location, unlike under the
// stricter RFC 3986 Section 4.3 absolute-URI rule
auto oauth_is_page_url(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->is_absolute();
}

} // namespace

auto oauth_make_server_metadata(const OAuthServerMetadataConfig &config)
    -> std::optional<JSON> {
  // RFC 8414 Section 2: the issuer is REQUIRED and must be a valid issuer
  // identifier
  if (!oauth_is_issuer_identifier(config.issuer)) {
    return std::nullopt;
  }

  // RFC 8414 Section 2: the authorization endpoint is "REQUIRED unless no grant
  // types are supported that use the authorization endpoint", and the token
  // endpoint is REQUIRED unless the only grant type is the implicit one. The
  // authorization endpoint grant types are the authorization code and implicit
  // grants, and an omitted grant type list defaults to both, so both endpoints
  // are needed by default. Every advertised URL is an https location, and a
  // present scalar that is not a valid https URL, or a missing required
  // endpoint, would yield an unusable discovery document
  const bool authorization_endpoint_needed{
      config.grant_types_supported.empty() ||
      std::ranges::any_of(config.grant_types_supported,
                          [](const std::string_view grant) -> bool {
                            return grant == "authorization_code" ||
                                   grant == "implicit";
                          })};
  const bool token_endpoint_needed{
      config.grant_types_supported.empty() ||
      std::ranges::any_of(config.grant_types_supported,
                          [](const std::string_view grant) -> bool {
                            return grant != "implicit";
                          })};

  // RFC 8414 Section 2: response_types_supported is REQUIRED unconditionally
  // (unlike authorization_endpoint, which is conditional), and Section 3.2
  // forbids a zero-element array
  if (config.response_types_supported.empty()) {
    return std::nullopt;
  }

  const bool authorization_endpoint_required_and_valid{
      authorization_endpoint_needed
          ? oauth_is_endpoint_url(config.authorization_endpoint)
          : config.authorization_endpoint.empty() ||
                oauth_is_endpoint_url(config.authorization_endpoint)};
  const bool token_endpoint_required_and_valid{
      token_endpoint_needed ? oauth_is_endpoint_url(config.token_endpoint)
                            : config.token_endpoint.empty() ||
                                  oauth_is_endpoint_url(config.token_endpoint)};
  if (!authorization_endpoint_required_and_valid ||
      !token_endpoint_required_and_valid ||
      (!config.registration_endpoint.empty() &&
       !oauth_is_endpoint_url(config.registration_endpoint)) ||
      (!config.pushed_authorization_request_endpoint.empty() &&
       !oauth_is_endpoint_url(config.pushed_authorization_request_endpoint)) ||
      (!config.jwks_uri.empty() && !oauth_is_endpoint_url(config.jwks_uri))) {
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

  // RFC 9728 Section 4: the advertised protected resources are "resource
  // identifiers for OAuth protected resources", so an entry that is not one
  // would yield an unusable document
  if (!std::ranges::all_of(config.protected_resources,
                           oauth_is_resource_identifier)) {
    return std::nullopt;
  }

  auto document{JSON::make_object()};
  document.assign_assume_new("issuer", JSON{config.issuer}, HASH_ISSUER);
  document.assign_if_nonempty("authorization_endpoint",
                              HASH_AUTHORIZATION_ENDPOINT,
                              config.authorization_endpoint);
  document.assign_if_nonempty("token_endpoint", HASH_TOKEN_ENDPOINT,
                              config.token_endpoint);
  document.assign_if_nonempty("registration_endpoint",
                              HASH_REGISTRATION_ENDPOINT,
                              config.registration_endpoint);
  document.assign_if_nonempty("pushed_authorization_request_endpoint",
                              HASH_PAR_ENDPOINT,
                              config.pushed_authorization_request_endpoint);
  document.assign_if_nonempty("jwks_uri", HASH_JWKS_URI, config.jwks_uri);
  document.assign_if_nonempty("response_types_supported", HASH_RESPONSE_TYPES,
                              config.response_types_supported);
  document.assign_if_nonempty("grant_types_supported", HASH_GRANT_TYPES,
                              config.grant_types_supported);
  document.assign_if_nonempty("code_challenge_methods_supported",
                              HASH_CODE_CHALLENGE_METHODS,
                              config.code_challenge_methods_supported);
  document.assign_if_nonempty("token_endpoint_auth_methods_supported",
                              HASH_TOKEN_AUTH_METHODS,
                              config.token_endpoint_auth_methods_supported);
  document.assign_if_nonempty(
      "token_endpoint_auth_signing_alg_values_supported", HASH_TOKEN_AUTH_ALGS,
      config.token_endpoint_auth_signing_alg_values_supported);
  document.assign_if_nonempty("scopes_supported", HASH_SCOPES_SUPPORTED,
                              config.scopes_supported);
  document.assign_if_nonempty("protected_resources", HASH_PROTECTED_RESOURCES,
                              config.protected_resources);
  // RFC 9126 Section 5: the default is false, so the flag is emitted only when
  // the server requires pushed authorization requests
  if (config.require_pushed_authorization_requests) {
    document.assign_assume_new("require_pushed_authorization_requests",
                               JSON{true}, HASH_REQUIRE_PAR);
  }

  return document;
}

auto oauth_make_resource_metadata(const OAuthResourceMetadataConfig &config)
    -> std::optional<JSON> {
  // RFC 9728 Section 2: the resource is REQUIRED and must be a valid resource
  // identifier per Section 1.2
  if (!oauth_is_resource_identifier(config.resource)) {
    return std::nullopt;
  }

  // RFC 9728 Section 2: the authorization servers are "OAuth authorization
  // server issuer identifiers, as defined in [RFC8414]". A client resolves an
  // entry by inserting the well-known string into it (RFC 8414 Section 3) and
  // then requires the metadata issuer to be "identical" to it by code points
  // (RFC 8414 Sections 3.3 and 4), so only an entry in the exact issuer
  // identifier form can ever complete discovery, and emitting any other form
  // would advertise a dead end
  if (!std::ranges::all_of(config.authorization_servers,
                           oauth_is_issuer_identifier)) {
    return std::nullopt;
  }

  // RFC 9728 Section 2 on jwks_uri: "This URL MUST use the https scheme"
  if (!config.jwks_uri.empty() && !oauth_is_endpoint_url(config.jwks_uri)) {
    return std::nullopt;
  }

  // RFC 9728 Section 2: "The value none MUST NOT be used" in the resource
  // signing algorithms
  if (span_contains(config.resource_signing_alg_values_supported, "none")) {
    return std::nullopt;
  }

  // RFC 9449 Section 4.2: a DPoP proof algorithm "MUST NOT be none or an
  // identifier for a symmetric algorithm (Message Authentication Code (MAC))",
  // so advertising one describes a proof that cannot exist. The MAC exclusion
  // covers the registered JWS MAC identifiers, and a later registration would
  // pass, since the registry is extensible and not modelled here
  if (span_contains(config.dpop_signing_alg_values_supported, "none") ||
      span_contains(config.dpop_signing_alg_values_supported, "HS256") ||
      span_contains(config.dpop_signing_alg_values_supported, "HS384") ||
      span_contains(config.dpop_signing_alg_values_supported, "HS512")) {
    return std::nullopt;
  }

  if ((!config.resource_documentation.empty() &&
       !oauth_is_page_url(config.resource_documentation)) ||
      (!config.resource_policy_uri.empty() &&
       !oauth_is_page_url(config.resource_policy_uri)) ||
      (!config.resource_tos_uri.empty() &&
       !oauth_is_page_url(config.resource_tos_uri))) {
    return std::nullopt;
  }

  auto document{JSON::make_object()};
  document.assign_assume_new("resource", JSON{config.resource}, HASH_RESOURCE);
  document.assign_if_nonempty("authorization_servers",
                              HASH_AUTHORIZATION_SERVERS,
                              config.authorization_servers);
  document.assign_if_nonempty("jwks_uri", HASH_JWKS_URI, config.jwks_uri);
  document.assign_if_nonempty("scopes_supported", HASH_SCOPES_SUPPORTED,
                              config.scopes_supported);
  // RFC 9728 Section 2: "The empty array [] can be used to indicate that no
  // bearer methods are supported", the one member whose engaged empty state is
  // emitted despite the Section 3.2 zero-value omission rule, whose general
  // form the specific text governs over
  if (config.bearer_methods_supported.has_value()) {
    auto methods{JSON::make_array()};
    for (const auto &method : config.bearer_methods_supported.value()) {
      methods.push_back(JSON{method});
    }

    document.assign_assume_new("bearer_methods_supported", std::move(methods),
                               HASH_BEARER_METHODS);
  }

  document.assign_if_nonempty("resource_signing_alg_values_supported",
                              HASH_RESOURCE_SIGNING_ALGS,
                              config.resource_signing_alg_values_supported);
  document.assign_if_nonempty("resource_name", HASH_RESOURCE_NAME,
                              config.resource_name);
  document.assign_if_nonempty("resource_documentation",
                              HASH_RESOURCE_DOCUMENTATION,
                              config.resource_documentation);
  document.assign_if_nonempty("resource_policy_uri", HASH_RESOURCE_POLICY_URI,
                              config.resource_policy_uri);
  document.assign_if_nonempty("resource_tos_uri", HASH_RESOURCE_TOS_URI,
                              config.resource_tos_uri);
  // RFC 9728 Section 2: both boolean defaults are false when absent, so each
  // flag is emitted only when true
  if (config.tls_client_certificate_bound_access_tokens) {
    document.assign_assume_new("tls_client_certificate_bound_access_tokens",
                               JSON{true}, HASH_TLS_CLIENT_CERTIFICATE_BOUND);
  }

  document.assign_if_nonempty("authorization_details_types_supported",
                              HASH_AUTHORIZATION_DETAILS_TYPES,
                              config.authorization_details_types_supported);
  document.assign_if_nonempty("dpop_signing_alg_values_supported",
                              HASH_DPOP_SIGNING_ALGS,
                              config.dpop_signing_alg_values_supported);
  if (config.dpop_bound_access_tokens_required) {
    document.assign_assume_new("dpop_bound_access_tokens_required", JSON{true},
                               HASH_DPOP_BOUND_REQUIRED);
  }

  return document;
}

} // namespace sourcemeta::core
