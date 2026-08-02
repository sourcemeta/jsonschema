#include <sourcemeta/core/oidc_registration.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth.h>
#include <sourcemeta/core/oidc_error.h>
#include <sourcemeta/core/uri.h>

#include <chrono>      // std::chrono::seconds
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_REDIRECT_URIS{JSON::Object::hash("redirect_uris"sv)};
constexpr auto HASH_APPLICATION_TYPE{JSON::Object::hash("application_type"sv)};
constexpr auto HASH_SUBJECT_TYPE{JSON::Object::hash("subject_type"sv)};
constexpr auto HASH_SECTOR_IDENTIFIER_URI{
    JSON::Object::hash("sector_identifier_uri"sv)};
constexpr auto HASH_ID_TOKEN_SIGNED_ALG{
    JSON::Object::hash("id_token_signed_response_alg"sv)};
constexpr auto HASH_ID_TOKEN_ENCRYPTED_ALG{
    JSON::Object::hash("id_token_encrypted_response_alg"sv)};
constexpr auto HASH_USERINFO_SIGNED_ALG{
    JSON::Object::hash("userinfo_signed_response_alg"sv)};
constexpr auto HASH_DEFAULT_MAX_AGE{JSON::Object::hash("default_max_age"sv)};
constexpr auto HASH_REQUIRE_AUTH_TIME{
    JSON::Object::hash("require_auth_time"sv)};
constexpr auto HASH_JWKS_URI{JSON::Object::hash("jwks_uri"sv)};
constexpr auto HASH_INITIATE_LOGIN_URI{
    JSON::Object::hash("initiate_login_uri"sv)};
constexpr auto HASH_POST_LOGOUT_REDIRECT_URIS{
    JSON::Object::hash("post_logout_redirect_uris"sv)};

auto string_member(const JSON &data, const JSON::StringView name,
                   const JSON::Object::hash_type hash)
    -> std::optional<std::string_view> {
  const auto *member{data.try_at(name, hash)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  return std::string_view{member->to_string()};
}

// A member that is absent, or present and a string. A present member of any
// other type is malformed for a field the accessors read as a string
auto absent_or_string(const JSON &data, const JSON::StringView name,
                      const JSON::Object::hash_type hash) -> bool {
  const auto *member{data.try_at(name, hash)};
  return member == nullptr || member->is_string();
}

// RFC 6749 Section 3.1.2: a redirection URI is an absolute URI without a
// fragment
auto is_absolute_uri_without_fragment(const std::string_view value) -> bool {
  try {
    const URI uri{value};
    return uri.scheme().has_value() && !uri.fragment().has_value();
  } catch (const URIParseError &) {
    return false;
  }
}

auto is_https_url_with_host(const std::string_view value) -> bool {
  try {
    const URI uri{value};
    return uri.is_https() && uri.host().has_value() &&
           !uri.host().value().empty();
  } catch (const URIParseError &) {
    return false;
  }
}

// OpenID Connect Dynamic Client Registration 1.0 Section 2: a web client using
// the implicit grant "MUST only register URLs using the https scheme as
// redirect_uris" and "they MUST NOT use localhost as the hostname"
auto is_web_implicit_redirect_uri(const std::string_view value) -> bool {
  try {
    const URI uri{value};
    return uri.is_https() && uri.host().has_value() &&
           !uri.host().value().empty() && !uri.is_localhost();
  } catch (const URIParseError &) {
    return false;
  }
}

// OpenID Connect Dynamic Client Registration 1.0 Section 2: a native client
// "MUST only register redirect_uris using custom URI schemes or loopback URLs
// using the http scheme", where a loopback URL names localhost or an IP
// loopback literal
auto is_native_redirect_uri(const std::string_view value) -> bool {
  try {
    const URI uri{value};
    if (uri.is_http()) {
      return uri.is_loopback() || uri.is_localhost();
    }

    return uri.scheme().has_value() && !uri.is_https();
  } catch (const URIParseError &) {
    return false;
  }
}

auto validate_client_metadata(const OAuthClientMetadata &oauth) -> void {
  const auto &data{oauth.data()};

  // OpenID Connect Dynamic Client Registration 1.0 Section 2: redirect_uris is
  // REQUIRED (OpenID Connect tightens the OAuth OPTIONAL), and each entry is an
  // absolute URI without a fragment (RFC 6749 Section 3.1.2), so an unusable
  // callback is never treated as registered
  const auto *redirect_uris{data.try_at("redirect_uris"sv, HASH_REDIRECT_URIS)};
  if (redirect_uris == nullptr || !redirect_uris->is_array() ||
      redirect_uris->empty()) {
    throw OIDCRegistrationParseError{};
  }

  for (const auto &element : redirect_uris->as_array()) {
    if (!element.is_string() ||
        !is_absolute_uri_without_fragment(element.to_string())) {
      throw OIDCRegistrationParseError{};
    }
  }

  // A present member whose JSON type is wrong is rejected rather than read as
  // its default, so malformed metadata cannot silently weaken the settings
  if (!absent_or_string(data, "application_type"sv, HASH_APPLICATION_TYPE) ||
      !absent_or_string(data, "subject_type"sv, HASH_SUBJECT_TYPE) ||
      !absent_or_string(data, "id_token_signed_response_alg"sv,
                        HASH_ID_TOKEN_SIGNED_ALG) ||
      !absent_or_string(data, "id_token_encrypted_response_alg"sv,
                        HASH_ID_TOKEN_ENCRYPTED_ALG) ||
      !absent_or_string(data, "userinfo_signed_response_alg"sv,
                        HASH_USERINFO_SIGNED_ALG)) {
    throw OIDCRegistrationParseError{};
  }

  // OpenID Connect Dynamic Client Registration 1.0 Section 2: "The
  // Authorization Server MUST verify that all the registered redirect_uris
  // conform to these constraints". A native client is bound to custom-scheme or
  // http loopback callbacks, while a web client using the implicit grant is
  // bound to https callbacks that are not localhost. The default application
  // type is web, and a web client that does not use the implicit grant carries
  // no such restriction
  const auto application_type{
      string_member(data, "application_type"sv, HASH_APPLICATION_TYPE)
          .value_or("web"sv)};
  // OpenID Connect Dynamic Client Registration 1.0 Section 2 defines only the
  // web and native application types, so an unknown value is rejected rather
  // than defaulted and left unrestricted
  if (application_type != "web"sv && application_type != "native"sv) {
    throw OIDCRegistrationParseError{};
  }
  const bool native{application_type == "native"sv};
  if (native || oauth.supports_grant_type("implicit"sv)) {
    for (const auto &element : redirect_uris->as_array()) {
      const std::string_view redirect_uri{element.to_string()};
      if (native ? !is_native_redirect_uri(redirect_uri)
                 : !is_web_implicit_redirect_uri(redirect_uri)) {
        throw OIDCRegistrationParseError{};
      }
    }
  }

  const auto *max_age{data.try_at("default_max_age"sv, HASH_DEFAULT_MAX_AGE)};
  if (max_age != nullptr &&
      (!max_age->is_integer() || max_age->to_integer() < 0)) {
    throw OIDCRegistrationParseError{};
  }

  const auto *require_auth_time{
      data.try_at("require_auth_time"sv, HASH_REQUIRE_AUTH_TIME)};
  if (require_auth_time != nullptr && !require_auth_time->is_boolean()) {
    throw OIDCRegistrationParseError{};
  }

  const auto *post_logout{data.try_at("post_logout_redirect_uris"sv,
                                      HASH_POST_LOGOUT_REDIRECT_URIS)};
  if (post_logout != nullptr && !post_logout->is_array_of_strings()) {
    throw OIDCRegistrationParseError{};
  }

  // OpenID Connect Dynamic Client Registration 1.0 Section 5: the
  // sector_identifier_uri is fetched over https by the OpenID Provider, so a
  // non-https or hostless value would point it at an insecure or invalid
  // location
  const auto *sector{
      data.try_at("sector_identifier_uri"sv, HASH_SECTOR_IDENTIFIER_URI)};
  if (sector != nullptr &&
      (!sector->is_string() || !is_https_url_with_host(sector->to_string()))) {
    throw OIDCRegistrationParseError{};
  }

  // OpenID Connect Dynamic Client Registration 1.0 Section 2: the jwks_uri is a
  // "URL for the Client's JWK Set document, which MUST use the https scheme".
  // The provider fetches it to obtain the keys that authenticate the client, so
  // over cleartext an attacker substitutes them and impersonates the client.
  // Note that request_uris carries the neighbouring rule conditionally, "unless
  // the target Request Object is signed in a way that is verifiable by the OP",
  // which is not knowable here, so it is deliberately left unchecked
  const auto *client_keys{data.try_at("jwks_uri"sv, HASH_JWKS_URI)};
  if (client_keys != nullptr &&
      (!client_keys->is_string() ||
       !is_https_url_with_host(client_keys->to_string()))) {
    throw OIDCRegistrationParseError{};
  }

  // OpenID Connect Core 1.0 Section 4: the initiate_login_uri is a third-party
  // login target that "MUST use the https scheme"
  const auto *initiate_login{
      data.try_at("initiate_login_uri"sv, HASH_INITIATE_LOGIN_URI)};
  if (initiate_login != nullptr &&
      (!initiate_login->is_string() ||
       !is_https_url_with_host(initiate_login->to_string()))) {
    throw OIDCRegistrationParseError{};
  }
}

} // namespace

OIDCClientMetadata::OIDCClientMetadata(JSON &&data) : oauth_{std::move(data)} {
  validate_client_metadata(this->oauth_);
}

auto OIDCClientMetadata::from(JSON &&data)
    -> std::optional<OIDCClientMetadata> {
  try {
    return OIDCClientMetadata{std::move(data)};
  } catch (const OAuthRegistrationParseError &) {
    return std::nullopt;
  } catch (const OIDCRegistrationParseError &) {
    return std::nullopt;
  }
}

auto OIDCClientMetadata::has_redirect_uri(const std::string_view value) const
    -> bool {
  return this->oauth_.has_redirect_uri(value);
}

auto OIDCClientMetadata::application_type() const -> std::string_view {
  // OpenID Connect Dynamic Client Registration 1.0 Section 2: the default is
  // web
  const auto value{string_member(this->oauth_.data(), "application_type"sv,
                                 HASH_APPLICATION_TYPE)};
  return value.value_or("web");
}

auto OIDCClientMetadata::subject_type() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "subject_type"sv,
                       HASH_SUBJECT_TYPE);
}

auto OIDCClientMetadata::sector_identifier_uri() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "sector_identifier_uri"sv,
                       HASH_SECTOR_IDENTIFIER_URI);
}

auto OIDCClientMetadata::id_token_signed_response_alg() const
    -> std::string_view {
  // OpenID Connect Dynamic Client Registration 1.0 Section 2: the default is
  // RS256
  const auto value{string_member(this->oauth_.data(),
                                 "id_token_signed_response_alg"sv,
                                 HASH_ID_TOKEN_SIGNED_ALG)};
  return value.value_or("RS256");
}

auto OIDCClientMetadata::id_token_encrypted_response_alg() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "id_token_encrypted_response_alg"sv,
                       HASH_ID_TOKEN_ENCRYPTED_ALG);
}

auto OIDCClientMetadata::userinfo_signed_response_alg() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "userinfo_signed_response_alg"sv,
                       HASH_USERINFO_SIGNED_ALG);
}

auto OIDCClientMetadata::default_max_age() const
    -> std::optional<std::chrono::seconds> {
  const auto *member{
      this->oauth_.data().try_at("default_max_age"sv, HASH_DEFAULT_MAX_AGE)};
  if (member == nullptr || !member->is_integer() || member->to_integer() < 0 ||
      member->to_integer() >
          std::numeric_limits<std::chrono::seconds::rep>::max()) {
    return std::nullopt;
  }

  return std::chrono::seconds{member->to_integer()};
}

auto OIDCClientMetadata::require_auth_time() const -> bool {
  const auto *member{this->oauth_.data().try_at("require_auth_time"sv,
                                                HASH_REQUIRE_AUTH_TIME)};
  return member != nullptr && member->is_boolean() && member->to_boolean();
}

auto OIDCClientMetadata::initiate_login_uri() const
    -> std::optional<std::string_view> {
  return string_member(this->oauth_.data(), "initiate_login_uri"sv,
                       HASH_INITIATE_LOGIN_URI);
}

auto OIDCClientMetadata::has_post_logout_redirect_uri(
    const std::string_view value) const -> bool {
  return this->oauth_.data().array_member_contains(
      "post_logout_redirect_uris"sv, HASH_POST_LOGOUT_REDIRECT_URIS, value);
}

auto OIDCClientMetadata::oauth() const -> const OAuthClientMetadata & {
  return this->oauth_;
}

auto OIDCClientMetadata::data() const -> const JSON & {
  return this->oauth_.data();
}

auto oidc_sector_identifier_contains(
    const JSON &sector_document,
    const std::span<const std::string_view> redirect_uris) -> bool {
  // OpenID Connect Dynamic Client Registration 1.0 Section 5: the document is a
  // JSON array of redirection URIs, so a non-string element makes it malformed,
  // and every registered URI must appear in it
  if (!sector_document.is_array_of_strings()) {
    return false;
  }

  for (const auto redirect_uri : redirect_uris) {
    bool found{false};
    for (const auto &element : sector_document.as_array()) {
      if (element.to_string() == redirect_uri) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

} // namespace sourcemeta::core
