#ifndef SOURCEMETA_CORE_OPENAPI_SECURITY_H_
#define SOURCEMETA_CORE_OPENAPI_SECURITY_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"
#include "reference.h"

#include <sourcemeta/core/text.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_SECURITY{JSON::Object::hash("security"sv)};
constexpr auto OPENAPI_HASH_TYPE{JSON::Object::hash("type"sv)};
constexpr auto OPENAPI_HASH_SCHEME{JSON::Object::hash("scheme"sv)};
constexpr auto OPENAPI_HASH_BEARER_FORMAT{JSON::Object::hash("bearerFormat"sv)};
constexpr auto OPENAPI_HASH_FLOWS{JSON::Object::hash("flows"sv)};
constexpr auto OPENAPI_HASH_OPENID_CONNECT_URL{
    JSON::Object::hash("openIdConnectUrl"sv)};
constexpr auto OPENAPI_HASH_SCOPES{JSON::Object::hash("scopes"sv)};
constexpr auto OPENAPI_HASH_AUTHORIZATION_URL{
    JSON::Object::hash("authorizationUrl"sv)};
constexpr auto OPENAPI_HASH_TOKEN_URL{JSON::Object::hash("tokenUrl"sv)};
constexpr auto OPENAPI_HASH_REFRESH_URL{JSON::Object::hash("refreshUrl"sv)};
constexpr auto OPENAPI_HASH_IMPLICIT{JSON::Object::hash("implicit"sv)};
constexpr auto OPENAPI_HASH_PASSWORD{JSON::Object::hash("password"sv)};
constexpr auto OPENAPI_HASH_CLIENT_CREDENTIALS{
    JSON::Object::hash("clientCredentials"sv)};
constexpr auto OPENAPI_HASH_AUTHORIZATION_CODE{
    JSON::Object::hash("authorizationCode"sv)};
constexpr auto OPENAPI_HASH_DEVICE_AUTHORIZATION{
    JSON::Object::hash("deviceAuthorization"sv)};
constexpr auto OPENAPI_HASH_DEVICE_URL{
    JSON::Object::hash("deviceAuthorizationUrl"sv)};
constexpr auto OPENAPI_HASH_OAUTH2_METADATA_URL{
    JSON::Object::hash("oauth2MetadataUrl"sv)};

constexpr std::array<JSON::StringView, 4>
    OPENAPI_SECURITY_SCHEME_APIKEY_FIELDS_3_1{
        {"type"sv, "description"sv, "name"sv, "in"sv}};

// OpenAPI Specification 3.2.1, Section 4.27 adds `deprecated`, whose
// "Applies To" column reads "Any", and `oauth2MetadataUrl`, scoped to `oauth2`
constexpr std::array<JSON::StringView, 5>
    OPENAPI_SECURITY_SCHEME_APIKEY_FIELDS_3_2{
        {"type"sv, "description"sv, "name"sv, "in"sv, "deprecated"sv}};
// OpenAPI Specification 3.1.1, Section 4.8.27 scopes `bearerFormat` to `http
// ("bearer")` in its "Applies To" column, so an HTTP scheme that is not bearer
// does not define it. The published meta-schema draws the same line, admitting
// the field only under a `scheme` matching `^[Bb][Ee][Aa][Rr][Ee][Rr]$`
constexpr std::array<JSON::StringView, 3>
    OPENAPI_SECURITY_SCHEME_HTTP_FIELDS_3_1{
        {"type"sv, "description"sv, "scheme"sv}};
constexpr std::array<JSON::StringView, 4>
    OPENAPI_SECURITY_SCHEME_HTTP_FIELDS_3_2{
        {"type"sv, "description"sv, "scheme"sv, "deprecated"sv}};
constexpr std::array<JSON::StringView, 4>
    OPENAPI_SECURITY_SCHEME_HTTP_BEARER_FIELDS_3_1{
        {"type"sv, "description"sv, "scheme"sv, "bearerFormat"sv}};
constexpr std::array<JSON::StringView, 5>
    OPENAPI_SECURITY_SCHEME_HTTP_BEARER_FIELDS_3_2{
        {"type"sv, "description"sv, "scheme"sv, "bearerFormat"sv,
         "deprecated"sv}};
constexpr std::array<JSON::StringView, 3>
    OPENAPI_SECURITY_SCHEME_OAUTH2_FIELDS_3_1{
        {"type"sv, "description"sv, "flows"sv}};
constexpr std::array<JSON::StringView, 5>
    OPENAPI_SECURITY_SCHEME_OAUTH2_FIELDS_3_2{{"type"sv, "description"sv,
                                               "flows"sv, "oauth2MetadataUrl"sv,
                                               "deprecated"sv}};
constexpr std::array<JSON::StringView, 3>
    OPENAPI_SECURITY_SCHEME_OIDC_FIELDS_3_1{
        {"type"sv, "description"sv, "openIdConnectUrl"sv}};
constexpr std::array<JSON::StringView, 4>
    OPENAPI_SECURITY_SCHEME_OIDC_FIELDS_3_2{
        {"type"sv, "description"sv, "openIdConnectUrl"sv, "deprecated"sv}};
constexpr std::array<JSON::StringView, 2>
    OPENAPI_SECURITY_SCHEME_COMMON_FIELDS_3_1{{"type"sv, "description"sv}};
constexpr std::array<JSON::StringView, 3>
    OPENAPI_SECURITY_SCHEME_COMMON_FIELDS_3_2{
        {"type"sv, "description"sv, "deprecated"sv}};

constexpr std::array<JSON::StringView, 4> OPENAPI_OAUTH_FLOWS_FIELDS_3_1{
    {"implicit"sv, "password"sv, "clientCredentials"sv, "authorizationCode"sv}};

// OpenAPI Specification 3.2.1, Sections 4.28 and 4.29 add the device
// authorization flow and the URL it is driven by
constexpr std::array<JSON::StringView, 5> OPENAPI_OAUTH_FLOWS_FIELDS_3_2{
    {"implicit"sv, "password"sv, "clientCredentials"sv, "authorizationCode"sv,
     "deviceAuthorization"sv}};

constexpr std::array<JSON::StringView, 4> OPENAPI_OAUTH_FLOW_FIELDS_3_1{
    {"authorizationUrl"sv, "tokenUrl"sv, "refreshUrl"sv, "scopes"sv}};
constexpr std::array<JSON::StringView, 5> OPENAPI_OAUTH_FLOW_FIELDS_3_2{
    {"authorizationUrl"sv, "tokenUrl"sv, "refreshUrl"sv, "scopes"sv,
     "deviceAuthorizationUrl"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.29: "Configuration details for a
// supported OAuth Flow". Which of the two URLs a flow must carry depends on
// which flow it is, and `scopes` is required by every one of them
inline auto openapi_check_oauth_flow(const JSON &value, const Pointer &base,
                                     const bool needs_authorization_url,
                                     const bool needs_token_url,
                                     const bool needs_device_url,
                                     OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::OAuthFlow);
  openapi_expect_object(value, base, "The OAuth Flow Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_OAUTH_FLOW_FIELDS_3_1, OPENAPI_OAUTH_FLOW_FIELDS_3_2, base,
      "The OAuth Flow Object does not define this field", walk);

  // Section 4.8.29 scopes each URL to the flows it names in its "Applies To"
  // column, and it names exactly the flows that require it. So a flow does not
  // define the URL it does not require, which the published meta-schema draws
  // the same way, giving each flow its own property set
  if (!needs_authorization_url &&
      value.try_at("authorizationUrl", OPENAPI_HASH_AUTHORIZATION_URL) !=
          nullptr) {
    throw OpenAPIError{
        openapi_child(base, "authorizationUrl"sv),
        "This OAuth Flow Object does not define an authorization URI"};
  }

  if (!needs_token_url &&
      value.try_at("tokenUrl", OPENAPI_HASH_TOKEN_URL) != nullptr) {
    throw OpenAPIError{openapi_child(base, "tokenUrl"sv),
                       "This OAuth Flow Object does not define a token URI"};
  }

  // OpenAPI Specification 3.2.1, Section 4.29 scopes `deviceAuthorizationUrl`
  // to `oauth2 ("deviceAuthorization")` and marks it REQUIRED there
  const auto *device{
      value.try_at("deviceAuthorizationUrl", OPENAPI_HASH_DEVICE_URL)};
  if (!needs_device_url && device != nullptr) {
    throw OpenAPIError{
        openapi_child(base, "deviceAuthorizationUrl"sv),
        "This OAuth Flow Object does not define a device authorization URI"};
  }

  if (needs_device_url) {
    if (device == nullptr) {
      throw OpenAPIError{
          base,
          "This OAuth Flow Object must declare a device authorization URI"};
    }

    openapi_expect_uri_reference(
        *device, base, "deviceAuthorizationUrl"sv,
        "The OAuth Flow Object device authorization URI must be a string",
        "The OAuth Flow Object device authorization URI must be a URI "
        "reference");
  }

  const auto *authorization{
      value.try_at("authorizationUrl", OPENAPI_HASH_AUTHORIZATION_URL)};
  if (needs_authorization_url && authorization == nullptr) {
    throw OpenAPIError{
        base, "This OAuth Flow Object must declare an authorization URI"};
  }

  if (authorization != nullptr) {
    openapi_expect_uri_reference(
        *authorization, base, "authorizationUrl"sv,
        "The OAuth Flow Object authorization URI must be a string",
        "The OAuth Flow Object authorization URI must be a URI reference");
  }

  const auto *token{value.try_at("tokenUrl", OPENAPI_HASH_TOKEN_URL)};
  if (needs_token_url && token == nullptr) {
    throw OpenAPIError{base, "This OAuth Flow Object must declare a token URI"};
  }

  if (token != nullptr) {
    openapi_expect_uri_reference(
        *token, base, "tokenUrl"sv,
        "The OAuth Flow Object token URI must be a string",
        "The OAuth Flow Object token URI must be a URI reference");
  }

  const auto *refresh{value.try_at("refreshUrl", OPENAPI_HASH_REFRESH_URL)};
  if (refresh != nullptr) {
    openapi_expect_uri_reference(
        *refresh, base, "refreshUrl"sv,
        "The OAuth Flow Object refresh URI must be a string",
        "The OAuth Flow Object refresh URI must be a URI reference");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.29: "scopes | Map[string, string]
  // | REQUIRED. The available scopes for the OAuth2 security scheme"
  const auto &scopes{
      openapi_require(value, "scopes"sv, OPENAPI_HASH_SCOPES, base,
                      "The OAuth Flow Object must declare its scopes")};

  openapi_check_map_of_strings(
      scopes, openapi_child(base, "scopes"sv),
      "The OAuth Flow Object scopes must be an object",
      "The OAuth Flow Object scopes must hold strings");
}

// OpenAPI Specification 3.1.1, Section 4.8.28: "Allows configuration of the
// supported OAuth Flows"
inline auto openapi_check_oauth_flows(const JSON &value, const Pointer &base,
                                      OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::OAuthFlows);
  openapi_expect_object(value, base,
                        "The OAuth Flows Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_OAUTH_FLOWS_FIELDS_3_1, OPENAPI_OAUTH_FLOWS_FIELDS_3_2,
      base, "The OAuth Flows Object does not define this field", walk);

  const auto *implicit{value.try_at("implicit"sv, OPENAPI_HASH_IMPLICIT)};
  if (implicit != nullptr) {
    openapi_check_oauth_flow(*implicit, openapi_child(base, "implicit"sv), true,
                             false, false, walk);
  }

  const auto *password{value.try_at("password"sv, OPENAPI_HASH_PASSWORD)};
  if (password != nullptr) {
    openapi_check_oauth_flow(*password, openapi_child(base, "password"sv),
                             false, true, false, walk);
  }

  const auto *client_credentials{
      value.try_at("clientCredentials"sv, OPENAPI_HASH_CLIENT_CREDENTIALS)};
  if (client_credentials != nullptr) {
    openapi_check_oauth_flow(*client_credentials,
                             openapi_child(base, "clientCredentials"sv), false,
                             true, false, walk);
  }

  // Section 4.28 adds the device authorization flow, whose required URLs are
  // its own and the token URL
  const auto *device{
      value.try_at("deviceAuthorization"sv, OPENAPI_HASH_DEVICE_AUTHORIZATION)};
  if (device != nullptr) {
    openapi_check_oauth_flow(*device,
                             openapi_child(base, "deviceAuthorization"sv),
                             false, true, true, walk);
  }

  const auto *authorization_code{
      value.try_at("authorizationCode"sv, OPENAPI_HASH_AUTHORIZATION_CODE)};
  if (authorization_code != nullptr) {
    openapi_check_oauth_flow(*authorization_code,
                             openapi_child(base, "authorizationCode"sv), true,
                             true, false, walk);
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.27: "Defines a security scheme that
// can be used by the operations". Its field table carries an "Applies To"
// column, so which fields are required depends on the type it declares
inline auto openapi_check_security_scheme(const JSON &value,
                                          const Pointer &base,
                                          OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::SecurityScheme);
  openapi_expect_object(value, base,
                        "The Security Scheme Object must be an object");

  // OpenAPI Specification 3.1.1, Section 4.8.27: "type | string | Any |
  // REQUIRED. The type of the security scheme"
  const auto &type{
      openapi_require(value, "type"sv, OPENAPI_HASH_TYPE, base,
                      "The Security Scheme Object must declare its type")};

  const auto scheme_type{openapi_expect_enumeration(
      type, base, "type"sv,
      {"apiKey"sv, "http"sv, "mutualTLS"sv, "oauth2"sv, "openIdConnect"sv},
      "The Security Scheme Object type must be a string",
      "The Security Scheme Object type is not one this specification "
      "defines")};

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Security Scheme Object description must be a string");

  // OpenAPI Specification 3.2.1, Section 4.27: "deprecated | boolean | Any".
  // Its "Applies To" column names every type, so it is read here rather than
  // under one of them, and each type's field table admits it in turn. Only
  // 3.2 defines the field, so under 3.1 the table below is what has something
  // to say about it rather than its type
  const auto *deprecated{value.try_at("deprecated", OPENAPI_HASH_DEPRECATED)};
  if (deprecated != nullptr && walk.version == OpenAPIVersion::OPENAPI_3_2) {
    openapi_expect_boolean(
        *deprecated, base, "deprecated"sv,
        "The Security Scheme Object deprecated must be a boolean");
  }

  if (scheme_type == "apiKey"sv) {
    openapi_reject_unknown_fields(
        value, OPENAPI_SECURITY_SCHEME_APIKEY_FIELDS_3_1,
        OPENAPI_SECURITY_SCHEME_APIKEY_FIELDS_3_2, base,
        "The Security Scheme Object does not define this field", walk);

    const auto &name{openapi_require(
        value, "name"sv, OPENAPI_HASH_NAME, base,
        "An apiKey Security Scheme Object must declare a name")};

    openapi_expect_string(name, base, "name"sv,
                          "The Security Scheme Object name must be a string");

    const auto &location{openapi_require(
        value, "in"sv, OPENAPI_HASH_IN, base,
        "An apiKey Security Scheme Object must declare a location")};

    openapi_expect_enumeration(
        location, base, "in"sv, {"query"sv, "header"sv, "cookie"sv},
        "The Security Scheme Object location must be a string",
        "The Security Scheme Object location is not one an apiKey admits");

    return;
  }

  if (scheme_type == "http"sv) {
    // RFC 7235 Section 2.1 makes an authentication scheme name
    // case-insensitive, which is why the meta-schema spells bearer as a
    // pattern rather than a constant
    const auto *scheme{value.try_at("scheme", OPENAPI_HASH_SCHEME)};
    if (scheme != nullptr && scheme->is_string() &&
        equals_ignore_case(scheme->to_string(), "bearer"sv)) {
      openapi_reject_unknown_fields(
          value, OPENAPI_SECURITY_SCHEME_HTTP_BEARER_FIELDS_3_1,
          OPENAPI_SECURITY_SCHEME_HTTP_BEARER_FIELDS_3_2, base,
          "The Security Scheme Object does not define this field", walk);
    } else {
      openapi_reject_unknown_fields(
          value, OPENAPI_SECURITY_SCHEME_HTTP_FIELDS_3_1,
          OPENAPI_SECURITY_SCHEME_HTTP_FIELDS_3_2, base,
          "The Security Scheme Object does not define this field", walk);
    }

    if (scheme == nullptr) {
      throw OpenAPIError{
          base, "An http Security Scheme Object must declare a scheme"};
    }

    openapi_expect_string(*scheme, base, "scheme"sv,
                          "The Security Scheme Object scheme must be a string");

    openapi_check_optional_string(
        value, base, "bearerFormat"sv, OPENAPI_HASH_BEARER_FORMAT,
        "The Security Scheme Object bearer format must be a string");

    return;
  }

  if (scheme_type == "oauth2"sv) {
    openapi_reject_unknown_fields(
        value, OPENAPI_SECURITY_SCHEME_OAUTH2_FIELDS_3_1,
        OPENAPI_SECURITY_SCHEME_OAUTH2_FIELDS_3_2, base,
        "The Security Scheme Object does not define this field", walk);

    // Section 4.27: "oauth2MetadataUrl | string | oauth2"
    const auto *metadata{
        value.try_at("oauth2MetadataUrl", OPENAPI_HASH_OAUTH2_METADATA_URL)};
    if (metadata != nullptr) {
      openapi_expect_uri_reference(
          *metadata, base, "oauth2MetadataUrl"sv,
          "The Security Scheme Object metadata URI must be a string",
          "The Security Scheme Object metadata URI must be a URI reference");
    }

    const auto &flows{openapi_require(
        value, "flows"sv, OPENAPI_HASH_FLOWS, base,
        "An oauth2 Security Scheme Object must declare its flows")};

    openapi_check_oauth_flows(flows, openapi_child(base, "flows"sv), walk);
    return;
  }

  if (scheme_type == "openIdConnect"sv) {
    openapi_reject_unknown_fields(
        value, OPENAPI_SECURITY_SCHEME_OIDC_FIELDS_3_1,
        OPENAPI_SECURITY_SCHEME_OIDC_FIELDS_3_2, base,
        "The Security Scheme Object does not define this field", walk);

    const auto &url{openapi_require(
        value, "openIdConnectUrl"sv, OPENAPI_HASH_OPENID_CONNECT_URL, base,
        "An openIdConnect Security Scheme Object must "
        "declare its discovery URI")};

    openapi_expect_uri_reference(
        url, base, "openIdConnectUrl"sv,
        "The Security Scheme Object discovery URI must be a string",
        "The Security Scheme Object discovery URI must be a URI reference");
    return;
  }

  // A mutualTLS scheme carries nothing beyond the two fields every type has
  openapi_reject_unknown_fields(
      value, OPENAPI_SECURITY_SCHEME_COMMON_FIELDS_3_1,
      OPENAPI_SECURITY_SCHEME_COMMON_FIELDS_3_2, base,
      "The Security Scheme Object does not define this field", walk);
}

inline auto openapi_check_security_scheme_or_reference(const JSON &value,
                                                       const Pointer &base,
                                                       OpenAPIWalk &walk)
    -> void {
  openapi_check_or_reference<OpenAPIObjectKind::SecurityScheme,
                             openapi_check_security_scheme>(value, base, walk);
}

// A Security Requirement Object name that no component declares. 3.1 has
// nowhere else for such a name to come from, while OpenAPI Specification
// 3.2.1, Section 4.30 widens it: "The name used for each property MUST either
// correspond to a security scheme declared in the Security Schemes under the
// Components Object, or be the URI of a Security Scheme Object". So one is
// read like a reference from there onwards, except that nothing writes it down
// as one, since a single Security Requirement Object may name several and the
// frame keys a reference by the Object that makes it. Whatever such a name
// lands on has to hold up as a Security Scheme Object, and one that lands on
// nothing is not an error, for the reason Section 4.8.23 gives of a `$ref`
inline auto openapi_check_security_scheme_name(const JSON::StringView name,
                                               const Pointer &origin,
                                               OpenAPIWalk &walk) -> void {
  if (walk.version != OpenAPIVersion::OPENAPI_3_2) {
    throw OpenAPIError{
        origin, "The Security Requirement Object must name a declared security "
                "scheme"};
  }

  const auto target{openapi_reference_target(name, walk)};
  if (!target.has_value()) {
    throw OpenAPIError{
        origin, "The Security Requirement Object must name a declared security "
                "scheme or the URI of one"};
  }

  // Naming a whole OpenAPI Description is naming something that is not a
  // Security Scheme Object, which is the same demand a Path Item Object's
  // `$ref` makes of what it points at
  openapi_follow_target(target.value(), origin,
                        OpenAPIObjectKind::SecurityScheme, true, walk);
}

inline auto openapi_check_security_requirement(const JSON &value,
                                               const Pointer &base,
                                               OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::SecurityRequirement);
  openapi_expect_object(value, base,
                        "The Security Requirement Object must be an object");
  for (const auto &entry : value.as_object()) {
    // Section 4.8.30: "Each name MUST correspond to a security scheme which is
    // declared in the Security Schemes under the Components Object". This
    // Object declares no pattern but its names, so a member called `x-` is a
    // scheme name and is held to the same requirement
    if (!walk.security_schemes.contains(entry.first)) {
      openapi_check_security_scheme_name(
          entry.first, openapi_child(base, entry.first), walk);
    }

    openapi_check_array_of_strings(
        entry.second, openapi_child(base, entry.first),
        "The Security Requirement Object entries must be arrays",
        "The Security Requirement Object entries must hold strings");
  }
}

// What comes back is where each Security Requirement Object was recorded, so
// that an operation can name the requirements in force without repeating them
inline auto openapi_check_security(const JSON &value, const Pointer &base,
                                   const char *type_message, OpenAPIWalk &walk)
    -> std::vector<JSON::String> {
  openapi_expect_array(value, base, type_message);
  std::vector<JSON::String> result;
  result.reserve(value.size());
  std::size_t index{0};
  for (const auto &requirement : value.as_array()) {
    const auto location{openapi_child(base, index)};
    openapi_check_security_requirement(requirement, location, walk);
    result.push_back(openapi_location_uri(walk.base, location));
    index += 1;
  }

  return result;
}

} // namespace sourcemeta::core

#endif
