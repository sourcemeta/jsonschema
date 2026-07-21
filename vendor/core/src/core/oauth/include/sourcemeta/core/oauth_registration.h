#ifndef SOURCEMETA_CORE_OAUTH_REGISTRATION_H_
#define SOURCEMETA_CORE_OAUTH_REGISTRATION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono::seconds
#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

/// @ingroup oauth
/// A dynamic client registration metadata document (RFC 7591 Section 2), owning
/// its JSON. The same document is exchanged in both roles: a client's
/// registration request (RFC 7591 Section 3.1) and the server's registration
/// response (RFC 7591 Section 3.2.1), so the response-only members carry a
/// value only when read over a response. The document is validated on
/// construction to reject a `jwks` and `jwks_uri` present together, and a
/// `grant_types`, `response_types`, or `token_endpoint_auth_method` present
/// with the wrong type, since each of those carries a default an accessor would
/// otherwise substitute (RFC 7591 Section 2). Accessors apply the specification
/// defaults, and any member
/// without a typed accessor, such as an internationalized name (RFC 7591
/// Section 2.2), is reachable through the underlying document. A string
/// accessor returns a view into the owned document, valid for the lifetime of
/// this object, so a view taken before the object is moved from must not be
/// used afterward. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto document{sourcemeta::core::parse_json(R"JSON({
///   "client_id": "s6BhdRkqt3",
///   "redirect_uris": [ "https://client.example.org/callback" ],
///   "token_endpoint_auth_method": "client_secret_basic"
/// })JSON")};
/// const auto metadata{
///     sourcemeta::core::OAuthClientMetadata::from(std::move(document))};
/// assert(metadata.has_value());
/// assert(metadata.value().client_id().value() == "s6BhdRkqt3");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthClientMetadata {
public:
  /// Construct and validate a client registration document, throwing when it is
  /// invalid. The document is moved in.
  explicit OAuthClientMetadata(JSON &&data);

  /// Construct and validate a client registration document, returning no value
  /// when it is invalid. The document is moved in.
  [[nodiscard]] static auto from(JSON &&data)
      -> std::optional<OAuthClientMetadata>;

  /// Whether a redirection URI is registered (RFC 7591 Section 2).
  [[nodiscard]] auto has_redirect_uri(const std::string_view value) const
      -> bool;

  /// The requested token endpoint authentication method, defaulting to
  /// `client_secret_basic` when absent (RFC 7591 Section 2).
  [[nodiscard]] auto token_endpoint_auth_method() const -> std::string_view;

  /// Whether a grant type is registered, defaulting to the authorization code
  /// grant when absent (RFC 7591 Section 2).
  [[nodiscard]] auto supports_grant_type(const std::string_view value) const
      -> bool;

  /// Whether a response type is registered, defaulting to the code response
  /// type when absent (RFC 7591 Section 2).
  [[nodiscard]] auto supports_response_type(const std::string_view value) const
      -> bool;

  /// The human-readable client name (RFC 7591 Section 2).
  [[nodiscard]] auto client_name() const -> std::optional<std::string_view>;

  /// The client information page (RFC 7591 Section 2).
  [[nodiscard]] auto client_uri() const -> std::optional<std::string_view>;

  /// The client logo location (RFC 7591 Section 2).
  [[nodiscard]] auto logo_uri() const -> std::optional<std::string_view>;

  /// The space-separated scopes the client may request (RFC 7591 Section 2).
  [[nodiscard]] auto scope() const -> std::optional<std::string_view>;

  /// Whether a contact is listed (RFC 7591 Section 2).
  [[nodiscard]] auto has_contact(const std::string_view value) const -> bool;

  /// The terms of service page (RFC 7591 Section 2).
  [[nodiscard]] auto tos_uri() const -> std::optional<std::string_view>;

  /// The privacy policy page (RFC 7591 Section 2).
  [[nodiscard]] auto policy_uri() const -> std::optional<std::string_view>;

  /// The JWK Set document location for the client's keys (RFC 7591 Section 2).
  [[nodiscard]] auto jwks_uri() const -> std::optional<std::string_view>;

  /// The client software identifier, stable across instances (RFC 7591
  /// Section 2).
  [[nodiscard]] auto software_id() const -> std::optional<std::string_view>;

  /// The client software version (RFC 7591 Section 2).
  [[nodiscard]] auto software_version() const
      -> std::optional<std::string_view>;

  /// The software statement asserting the metadata, echoed unmodified in a
  /// response (RFC 7591 Section 2.3).
  [[nodiscard]] auto software_statement() const
      -> std::optional<std::string_view>;

  /// The issued client identifier, present only in a registration response
  /// (RFC 7591 Section 3.2.1).
  [[nodiscard]] auto client_id() const -> std::optional<std::string_view>;

  /// The issued client secret, present only in a response to a confidential
  /// client (RFC 7591 Section 3.2.1).
  [[nodiscard]] auto client_secret() const -> std::optional<std::string_view>;

  /// The time the client identifier was issued, present only in a response
  /// (RFC 7591 Section 3.2.1).
  [[nodiscard]] auto client_id_issued_at() const
      -> std::optional<std::chrono::seconds>;

  /// The time the client secret expires, zero meaning it never expires, present
  /// only in a response that issued a secret (RFC 7591 Section 3.2.1).
  [[nodiscard]] auto client_secret_expires_at() const
      -> std::optional<std::chrono::seconds>;

  /// The registration access token for managing the registration, present only
  /// in a response when management is offered (RFC 7592 Section 3).
  [[nodiscard]] auto registration_access_token() const
      -> std::optional<std::string_view>;

  /// The registration management location, present only in a response when
  /// management is offered (RFC 7592 Section 3).
  [[nodiscard]] auto registration_client_uri() const
      -> std::optional<std::string_view>;

  /// The underlying document, for reaching members without a typed accessor.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  JSON data_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif

/// @ingroup oauth
/// The client metadata a registration request carries (RFC 7591 Section 2),
/// each field a non-owning view. An empty scalar and a zero-element array are
/// omitted.
struct OAuthClientRegistrationConfig {
  /// The redirection URIs the client registers for redirect-based flows
  /// (RFC 7591 Section 2).
  std::span<const std::string_view> redirect_uris;
  /// The requested token endpoint authentication method, omitted to take the
  /// `client_secret_basic` default (RFC 7591 Section 2).
  std::string_view token_endpoint_auth_method;
  /// The grant types the client will use (RFC 7591 Section 2).
  std::span<const std::string_view> grant_types;
  /// The response types the client will use (RFC 7591 Section 2).
  std::span<const std::string_view> response_types;
  /// The human-readable client name (RFC 7591 Section 2).
  std::string_view client_name;
  /// The client information page (RFC 7591 Section 2).
  std::string_view client_uri;
  /// The client logo location (RFC 7591 Section 2).
  std::string_view logo_uri;
  /// The space-separated scopes the client may request (RFC 7591 Section 2).
  std::string_view scope;
  /// The ways to contact those responsible for the client (RFC 7591 Section 2).
  std::span<const std::string_view> contacts;
  /// The terms of service page (RFC 7591 Section 2).
  std::string_view tos_uri;
  /// The privacy policy page (RFC 7591 Section 2).
  std::string_view policy_uri;
  /// The JWK Set document location for the client's keys, mutually exclusive
  /// with an inline key set (RFC 7591 Section 2).
  std::string_view jwks_uri;
  /// The client's JWK Set carried inline for a client that cannot host one,
  /// mutually exclusive with its location, a non-owning pointer left null when
  /// absent (RFC 7591 Section 2).
  const JSON *jwks{nullptr};
  /// The client software identifier (RFC 7591 Section 2).
  std::string_view software_id;
  /// The client software version (RFC 7591 Section 2).
  std::string_view software_version;
  /// The software statement asserting the metadata (RFC 7591 Section 2.3).
  std::string_view software_statement;
};

/// @ingroup oauth
/// Build a dynamic client registration request body (RFC 7591 Section 3.1),
/// returning no value when a present `client_uri`, `logo_uri`, `tos_uri`,
/// `policy_uri`, `jwks_uri`, or redirection URI is not a valid URI, or an
/// inline key set is not a JSON object or is given together with its location.
/// Each present scalar and each non-empty array is emitted. The caller
/// serializes the object as the `application/json` request body. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 1> redirect_uris{
///     {"https://client.example.org/callback"}};
/// sourcemeta::core::OAuthClientRegistrationConfig config;
/// config.redirect_uris = redirect_uris;
/// config.client_name = "My Example Client";
/// const auto body{
///     sourcemeta::core::oauth_make_registration_request(config)};
/// assert(body.has_value());
/// assert(body.value().at("client_name").to_string() == "My Example Client");
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_make_registration_request(const OAuthClientRegistrationConfig &config)
    -> std::optional<JSON>;

/// @ingroup oauth
/// Build a dynamic client registration error response body (RFC 7591
/// Section 3.2.2). The error is the wire error code, and the description is
/// emitted only when present. The caller serializes the object as the
/// `application/json` response body. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto body{sourcemeta::core::oauth_make_registration_error_response(
///     sourcemeta::core::oauth_error_code(
///         sourcemeta::core::OAuthRegistrationError::InvalidClientMetadata),
///     "The grant types are inconsistent with the response types")};
/// assert(body.at("error").to_string() == "invalid_client_metadata");
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_make_registration_error_response(const std::string_view error,
                                       const std::string_view error_description)
    -> JSON;

/// @ingroup oauth
/// Whether the registered grant types and response types are mutually
/// consistent (RFC 7591 Section 2.1): the `authorization_code` grant pairs with
/// the `code` response type and the `implicit` grant with the `token` response
/// type. Only the explicitly registered lists are compared, so a document that
/// omits either list registers no value that could contradict the other, and
/// the server surfaces an inconsistency as `invalid_client_metadata` (RFC 7591
/// Section 3.2.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto document{sourcemeta::core::parse_json(R"JSON({
///   "grant_types": [ "authorization_code" ],
///   "response_types": [ "code" ]
/// })JSON")};
/// const auto metadata{
///     sourcemeta::core::OAuthClientMetadata::from(std::move(document))};
/// assert(metadata.has_value());
/// assert(sourcemeta::core::oauth_registration_grant_response_consistent(
///     metadata.value()));
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_registration_grant_response_consistent(
    const OAuthClientMetadata &metadata) -> bool;

/// @ingroup oauth
/// Flatten the claims of a trusted software statement onto a registration
/// record (RFC 7591 Section 3.1.1), returning false when the record or the
/// claims are not a JSON object. A statement claim takes precedence over a
/// directly supplied value of the same name, so it overwrites it, while the
/// JSON Web Token structural claims (RFC 7519 Section 4.1) and the statement
/// member itself are left out since they describe the statement rather than the
/// client. Pass the claims a trusted statement was verified to carry. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto record{sourcemeta::core::parse_json(R"JSON({
///   "client_name": "Requested Name"
/// })JSON")};
/// const auto claims{sourcemeta::core::parse_json(R"JSON({
///   "iss": "https://statement-issuer.example",
///   "client_name": "Attested Name"
/// })JSON")};
/// sourcemeta::core::oauth_apply_software_statement_claims(record, claims);
/// assert(record.at("client_name").to_string() == "Attested Name");
/// assert(!record.defines("iss"));
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_apply_software_statement_claims(JSON &metadata, const JSON &claims)
    -> bool;

/// @ingroup oauth
/// The values an authorization server assigns to a client at registration
/// (RFC 7591 Section 3.2.1) and for its management (RFC 7592 Section 3), each a
/// non-owning view. The client identifier is REQUIRED, a secret and its
/// expiry are emitted together, and the management pair is emitted when
/// registration management is offered.
struct OAuthClientRegistrationResult {
  /// The issued client identifier (RFC 7591 Section 3.2.1), REQUIRED.
  std::string_view client_id;
  /// The issued client secret, present for a confidential client (RFC 7591
  /// Section 3.2.1).
  std::string_view client_secret;
  /// The time the client identifier was issued (RFC 7591 Section 3.2.1).
  std::optional<std::chrono::seconds> client_id_issued_at{};
  /// The time the client secret expires, zero meaning it never expires,
  /// REQUIRED alongside a secret (RFC 7591 Section 3.2.1).
  std::optional<std::chrono::seconds> client_secret_expires_at{};
  /// The registration access token for managing the registration, REQUIRED
  /// together with the management location when management is offered (RFC 7592
  /// Section 3).
  std::string_view registration_access_token;
  /// The registration management location, REQUIRED together with the access
  /// token when management is offered (RFC 7592 Section 3).
  std::string_view registration_client_uri;
};

/// @ingroup oauth
/// Build a dynamic client registration response body (RFC 7591 Section 3.2.1
/// and RFC 7592 Section 3) by returning the registered metadata with the
/// server-assigned values overlaid, or no value when the client identifier is
/// empty, only one of the secret and its expiry is given, a time is negative,
/// only one of the management access token and location is given, or the
/// management location is not a valid URI. The server-assigned members come
/// only from the assigned values, never from the accepted record. The caller
/// serializes the object as the `application/json` response body. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto metadata{sourcemeta::core::parse_json(R"JSON({
///   "redirect_uris": [ "https://client.example.org/callback" ]
/// })JSON")};
/// sourcemeta::core::OAuthClientRegistrationResult result;
/// result.client_id = "s6BhdRkqt3";
/// const auto body{
///     sourcemeta::core::oauth_make_registration_response(metadata, result)};
/// assert(body.has_value());
/// assert(body.value().at("client_id").to_string() == "s6BhdRkqt3");
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_make_registration_response(const JSON &metadata,
                                 const OAuthClientRegistrationResult &result)
    -> std::optional<JSON>;

/// @ingroup oauth
/// Build a dynamic client registration update request body (RFC 7592
/// Section 2.2), a full replacement of the client's metadata that MUST carry
/// the current client identifier and MAY carry the current secret to match
/// against, returning no value when the client identifier is empty or a URL
/// field is not a valid URI. Every metadata field is emitted as in a
/// registration request, and the four server-assigned members are excluded
/// since the request MUST NOT carry them. An omitted field is a request to
/// delete it, since the update replaces rather than augments. The caller
/// serializes the object as the `application/json` request body. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 1> redirect_uris{
///     {"https://client.example.org/callback"}};
/// sourcemeta::core::OAuthClientRegistrationConfig config;
/// config.redirect_uris = redirect_uris;
/// config.client_name = "My New Example";
/// const auto body{sourcemeta::core::oauth_make_registration_update_request(
///     config, "s6BhdRkqt3", "")};
/// assert(body.has_value());
/// assert(body.value().at("client_id").to_string() == "s6BhdRkqt3");
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_make_registration_update_request(
    const OAuthClientRegistrationConfig &config,
    const std::string_view client_id, const std::string_view client_secret)
    -> std::optional<JSON>;

} // namespace sourcemeta::core

#endif
