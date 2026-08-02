#include <sourcemeta/core/oidc_metadata.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth.h>
#include <sourcemeta/core/oidc_error.h>
#include <sourcemeta/core/uri.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_USERINFO_ENDPOINT{
    JSON::Object::hash("userinfo_endpoint"sv)};
constexpr auto HASH_END_SESSION_ENDPOINT{
    JSON::Object::hash("end_session_endpoint"sv)};
constexpr auto HASH_CHECK_SESSION_IFRAME{
    JSON::Object::hash("check_session_iframe"sv)};
constexpr auto HASH_SUBJECT_TYPES{
    JSON::Object::hash("subject_types_supported"sv)};
constexpr auto HASH_ID_TOKEN_SIGNING_ALGS{
    JSON::Object::hash("id_token_signing_alg_values_supported"sv)};
constexpr auto HASH_ID_TOKEN_ENCRYPTION_ALGS{
    JSON::Object::hash("id_token_encryption_alg_values_supported"sv)};
constexpr auto HASH_ID_TOKEN_ENCRYPTION_ENCS{
    JSON::Object::hash("id_token_encryption_enc_values_supported"sv)};
constexpr auto HASH_USERINFO_SIGNING_ALGS{
    JSON::Object::hash("userinfo_signing_alg_values_supported"sv)};
constexpr auto HASH_USERINFO_ENCRYPTION_ALGS{
    JSON::Object::hash("userinfo_encryption_alg_values_supported"sv)};
constexpr auto HASH_USERINFO_ENCRYPTION_ENCS{
    JSON::Object::hash("userinfo_encryption_enc_values_supported"sv)};
constexpr auto HASH_REQUEST_OBJECT_SIGNING_ALGS{
    JSON::Object::hash("request_object_signing_alg_values_supported"sv)};
constexpr auto HASH_REQUEST_OBJECT_ENCRYPTION_ALGS{
    JSON::Object::hash("request_object_encryption_alg_values_supported"sv)};
constexpr auto HASH_REQUEST_OBJECT_ENCRYPTION_ENCS{
    JSON::Object::hash("request_object_encryption_enc_values_supported"sv)};
constexpr auto HASH_CLAIMS_SUPPORTED{JSON::Object::hash("claims_supported"sv)};
constexpr auto HASH_ACR_VALUES{JSON::Object::hash("acr_values_supported"sv)};
constexpr auto HASH_CLAIM_TYPES{JSON::Object::hash("claim_types_supported"sv)};
constexpr auto HASH_DISPLAY_VALUES{
    JSON::Object::hash("display_values_supported"sv)};
constexpr auto HASH_CLAIMS_LOCALES{
    JSON::Object::hash("claims_locales_supported"sv)};
constexpr auto HASH_UI_LOCALES{JSON::Object::hash("ui_locales_supported"sv)};
constexpr auto HASH_SCOPES_SUPPORTED{JSON::Object::hash("scopes_supported"sv)};
constexpr auto HASH_CLAIMS_PARAMETER{
    JSON::Object::hash("claims_parameter_supported"sv)};
constexpr auto HASH_REQUEST_PARAMETER{
    JSON::Object::hash("request_parameter_supported"sv)};
constexpr auto HASH_REQUEST_URI_PARAMETER{
    JSON::Object::hash("request_uri_parameter_supported"sv)};
constexpr auto HASH_REQUIRE_REQUEST_URI_REGISTRATION{
    JSON::Object::hash("require_request_uri_registration"sv)};

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

// A required "supported" list, a present and non-empty array of strings
auto is_required_string_array(const JSON *member) -> bool {
  return member != nullptr && member->is_array_of_strings() && !member->empty();
}

auto is_https_url(const std::string_view value) -> bool {
  // OpenID Connect Discovery 1.0 Section 3: every advertised endpoint is a URL
  // using the https scheme
  try {
    const URI uri{value};
    // Section 3 enumerates what an endpoint may carry as "port, path, and query
    // parameter components", so a fragment is refused as outside that list
    // rather than because it could not be dereferenced, which would be the
    // wrong reason for the two endpoints a user agent loads
    return uri.is_https() && uri.host().has_value() &&
           !uri.host().value().empty() && !uri.fragment().has_value();
  } catch (const URIParseError &) {
    return false;
  }
}

// A member that is present but is not a valid https URL fails the parse rather
// than being ignored, since an accessor would otherwise report a malformed
// member as an absent one
auto validate_endpoint(const JSON &data, const JSON::StringView name,
                       const JSON::Object::hash_type hash) -> void {
  const auto *member{data.try_at(name, hash)};
  if (member == nullptr) {
    return;
  }

  if (!member->is_string() || !is_https_url(member->to_string())) {
    throw OIDCMetadataParseError{};
  }
}

auto validate_provider_metadata(const OAuthServerMetadata &oauth) -> void {
  const auto &data{oauth.data()};

  // OpenID Connect Discovery 1.0 Section 3: OpenID Connect tightens jwks_uri
  // from OPTIONAL (RFC 8414) to REQUIRED, since the RP needs the signing keys
  if (!oauth.jwks_uri().has_value()) {
    throw OIDCMetadataParseError{};
  }

  // OpenID Connect Discovery 1.0 Section 3: subject_types_supported is REQUIRED
  const auto *subject_types{
      data.try_at("subject_types_supported"sv, HASH_SUBJECT_TYPES)};
  if (!is_required_string_array(subject_types)) {
    throw OIDCMetadataParseError{};
  }

  // OpenID Connect Discovery 1.0 Section 3:
  // id_token_signing_alg_values_supported is REQUIRED and "The algorithm RS256
  // MUST be included". "The value none MAY be supported but MUST NOT be used
  // unless the Response Type used returns no ID Token from the Authorization
  // Endpoint", so a provider is allowed to advertise it here, and the module
  // enforces the never-accept-none rule when an ID Token is validated rather
  // than when the metadata is parsed
  const auto *id_token_algs{data.try_at(
      "id_token_signing_alg_values_supported"sv, HASH_ID_TOKEN_SIGNING_ALGS)};
  if (!is_required_string_array(id_token_algs) ||
      !id_token_algs->contains("RS256")) {
    throw OIDCMetadataParseError{};
  }

  // The endpoints the OAuth layer does not know about. OpenID Connect Discovery
  // 1.0 Section 3 on userinfo_endpoint, OpenID Connect RP-Initiated Logout 1.0
  // Section 2.1 on end_session_endpoint, and OpenID Connect Session Management
  // 1.0 Section 3.3 on check_session_iframe all carry the same requirement:
  // "This URL MUST use the https scheme and MAY contain port, path, and query
  // parameter components". The endpoints shared with OAuth are already covered
  // when that document is validated, which happens before this runs
  validate_endpoint(data, "userinfo_endpoint"sv, HASH_USERINFO_ENDPOINT);
  validate_endpoint(data, "end_session_endpoint"sv, HASH_END_SESSION_ENDPOINT);
  validate_endpoint(data, "check_session_iframe"sv, HASH_CHECK_SESSION_IFRAME);
}

} // namespace

OIDCProviderMetadata::OIDCProviderMetadata(JSON &&data,
                                           const std::string_view issuer)
    : oauth_{std::move(data), issuer} {
  validate_provider_metadata(this->oauth_);
}

OIDCProviderMetadata::OIDCProviderMetadata(OAuthServerMetadata &&oauth)
    : oauth_{std::move(oauth)} {
  validate_provider_metadata(this->oauth_);
}

auto OIDCProviderMetadata::from(OAuthServerMetadata &&oauth)
    -> std::optional<OIDCProviderMetadata> {
  // The OAuth layer validated itself on the way in, so only the OpenID Connect
  // requirements can fail here
  try {
    return OIDCProviderMetadata{std::move(oauth)};
  } catch (const OIDCMetadataParseError &) {
    return std::nullopt;
  }
}

auto OIDCProviderMetadata::from(JSON &&data, const std::string_view issuer)
    -> std::optional<OIDCProviderMetadata> {
  try {
    return OIDCProviderMetadata{std::move(data), issuer};
  } catch (const OAuthMetadataParseError &) {
    return std::nullopt;
  } catch (const OIDCMetadataParseError &) {
    return std::nullopt;
  }
}

auto OIDCProviderMetadata::issuer() const -> std::string_view {
  return this->oauth_.issuer();
}

auto OIDCProviderMetadata::authorization_endpoint() const
    -> std::optional<std::string_view> {
  return this->oauth_.authorization_endpoint();
}

auto OIDCProviderMetadata::token_endpoint() const
    -> std::optional<std::string_view> {
  return this->oauth_.token_endpoint();
}

auto OIDCProviderMetadata::userinfo_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "userinfo_endpoint"sv,
                       HASH_USERINFO_ENDPOINT);
}

auto OIDCProviderMetadata::registration_endpoint() const
    -> std::optional<std::string_view> {
  return this->oauth_.registration_endpoint();
}

auto OIDCProviderMetadata::jwks_uri() const -> std::string_view {
  return this->oauth_.jwks_uri().value();
}

auto OIDCProviderMetadata::end_session_endpoint() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "end_session_endpoint"sv,
                       HASH_END_SESSION_ENDPOINT);
}

auto OIDCProviderMetadata::check_session_iframe() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "check_session_iframe"sv,
                       HASH_CHECK_SESSION_IFRAME);
}

auto OIDCProviderMetadata::supports_subject_type(
    const std::string_view value) const -> bool {
  return this->oauth_.data().array_member_contains("subject_types_supported"sv,
                                                   HASH_SUBJECT_TYPES, value);
}

auto OIDCProviderMetadata::supports_id_token_signing_alg(
    const std::string_view value) const -> bool {
  return this->oauth_.data().array_member_contains(
      "id_token_signing_alg_values_supported"sv, HASH_ID_TOKEN_SIGNING_ALGS,
      value);
}

auto OIDCProviderMetadata::supports_response_type(
    const std::string_view value) const -> bool {
  return this->oauth_.supports_response_type(value);
}

auto OIDCProviderMetadata::supports_token_endpoint_auth_method(
    const std::string_view value) const -> bool {
  return this->oauth_.supports_token_endpoint_auth_method(value);
}

auto OIDCProviderMetadata::supports_scope(const std::string_view value) const
    -> bool {
  return this->oauth_.data().array_member_contains(
      "scopes_supported"sv, HASH_SCOPES_SUPPORTED, value);
}

auto OIDCProviderMetadata::supports_claim(const std::string_view value) const
    -> bool {
  return this->oauth_.data().array_member_contains(
      "claims_supported"sv, HASH_CLAIMS_SUPPORTED, value);
}

auto OIDCProviderMetadata::oauth() const -> const OAuthServerMetadata & {
  return this->oauth_;
}

auto OIDCProviderMetadata::data() const -> const JSON & {
  return this->oauth_.data();
}

auto oidc_make_provider_metadata(const OIDCProviderMetadataConfig &config)
    -> std::optional<JSON> {
  // OpenID Connect Discovery 1.0 Section 3: jwks_uri, subject_types_supported,
  // and id_token_signing_alg_values_supported are REQUIRED for OpenID Connect
  if (config.base.jwks_uri.empty() || config.subject_types_supported.empty() ||
      config.id_token_signing_alg_values_supported.empty()) {
    return std::nullopt;
  }

  // OpenID Connect Discovery 1.0 Section 3: RS256 MUST be included. The module
  // never publishes none even though the specification permits advertising it
  // (module design Section 14)
  bool advertises_rs256{false};
  for (const auto algorithm : config.id_token_signing_alg_values_supported) {
    if (algorithm == "none") {
      return std::nullopt;
    }

    if (algorithm == "RS256") {
      advertises_rs256 = true;
    }
  }

  if (!advertises_rs256) {
    return std::nullopt;
  }

  // OpenID Connect Discovery 1.0 Section 3: the OpenID Connect endpoints use
  // the https scheme
  if ((!config.userinfo_endpoint.empty() &&
       !is_https_url(config.userinfo_endpoint)) ||
      (!config.end_session_endpoint.empty() &&
       !is_https_url(config.end_session_endpoint)) ||
      (!config.check_session_iframe.empty() &&
       !is_https_url(config.check_session_iframe))) {
    return std::nullopt;
  }

  auto base_document{oauth_make_server_metadata(config.base)};
  if (!base_document.has_value()) {
    return std::nullopt;
  }

  auto &document{base_document.value()};
  document.assign_if_nonempty("userinfo_endpoint", HASH_USERINFO_ENDPOINT,
                              config.userinfo_endpoint);
  document.assign_if_nonempty("end_session_endpoint", HASH_END_SESSION_ENDPOINT,
                              config.end_session_endpoint);
  document.assign_if_nonempty("check_session_iframe", HASH_CHECK_SESSION_IFRAME,
                              config.check_session_iframe);
  document.assign_if_nonempty("subject_types_supported", HASH_SUBJECT_TYPES,
                              config.subject_types_supported);
  document.assign_if_nonempty("id_token_signing_alg_values_supported",
                              HASH_ID_TOKEN_SIGNING_ALGS,
                              config.id_token_signing_alg_values_supported);
  document.assign_if_nonempty("id_token_encryption_alg_values_supported",
                              HASH_ID_TOKEN_ENCRYPTION_ALGS,
                              config.id_token_encryption_alg_values_supported);
  document.assign_if_nonempty("id_token_encryption_enc_values_supported",
                              HASH_ID_TOKEN_ENCRYPTION_ENCS,
                              config.id_token_encryption_enc_values_supported);
  document.assign_if_nonempty("userinfo_signing_alg_values_supported",
                              HASH_USERINFO_SIGNING_ALGS,
                              config.userinfo_signing_alg_values_supported);
  document.assign_if_nonempty("userinfo_encryption_alg_values_supported",
                              HASH_USERINFO_ENCRYPTION_ALGS,
                              config.userinfo_encryption_alg_values_supported);
  document.assign_if_nonempty("userinfo_encryption_enc_values_supported",
                              HASH_USERINFO_ENCRYPTION_ENCS,
                              config.userinfo_encryption_enc_values_supported);
  document.assign_if_nonempty(
      "request_object_signing_alg_values_supported",
      HASH_REQUEST_OBJECT_SIGNING_ALGS,
      config.request_object_signing_alg_values_supported);
  document.assign_if_nonempty(
      "request_object_encryption_alg_values_supported",
      HASH_REQUEST_OBJECT_ENCRYPTION_ALGS,
      config.request_object_encryption_alg_values_supported);
  document.assign_if_nonempty(
      "request_object_encryption_enc_values_supported",
      HASH_REQUEST_OBJECT_ENCRYPTION_ENCS,
      config.request_object_encryption_enc_values_supported);
  document.assign_if_nonempty("claims_supported", HASH_CLAIMS_SUPPORTED,
                              config.claims_supported);
  document.assign_if_nonempty("acr_values_supported", HASH_ACR_VALUES,
                              config.acr_values_supported);
  document.assign_if_nonempty("claim_types_supported", HASH_CLAIM_TYPES,
                              config.claim_types_supported);
  document.assign_if_nonempty("display_values_supported", HASH_DISPLAY_VALUES,
                              config.display_values_supported);
  document.assign_if_nonempty("claims_locales_supported", HASH_CLAIMS_LOCALES,
                              config.claims_locales_supported);
  document.assign_if_nonempty("ui_locales_supported", HASH_UI_LOCALES,
                              config.ui_locales_supported);

  // OpenID Connect Discovery 1.0 Section 3: each capability flag is emitted
  // only when it differs from its default, keeping the document minimal
  if (config.claims_parameter_supported) {
    document.assign_assume_new("claims_parameter_supported", JSON{true},
                               HASH_CLAIMS_PARAMETER);
  }

  if (config.request_parameter_supported) {
    document.assign_assume_new("request_parameter_supported", JSON{true},
                               HASH_REQUEST_PARAMETER);
  }

  if (!config.request_uri_parameter_supported) {
    document.assign_assume_new("request_uri_parameter_supported", JSON{false},
                               HASH_REQUEST_URI_PARAMETER);
  }

  if (config.require_request_uri_registration) {
    document.assign_assume_new("require_request_uri_registration", JSON{true},
                               HASH_REQUIRE_REQUEST_URI_REGISTRATION);
  }

  return document;
}

} // namespace sourcemeta::core
