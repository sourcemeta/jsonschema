#ifndef SOURCEMETA_CORE_OAUTH_METADATA_H_
#define SOURCEMETA_CORE_OAUTH_METADATA_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The kind of metadata document a well-known URL points at, which selects the
/// suffix and whether it is inserted before the path or appended after it.
enum class OAuthWellKnownKind : std::uint8_t {
  /// The `oauth-authorization-server` document, inserted before the path
  /// (RFC 8414 Section 3).
  AuthorizationServer,
  /// The `oauth-protected-resource` document, inserted before the path
  /// (RFC 9728 Section 3).
  ProtectedResource,
  /// The `openid-configuration` document, inserted before the path (RFC 8414
  /// Section 5).
  OpenIDConfigurationInserted,
  /// The `openid-configuration` document, appended after the path, the legacy
  /// OpenID Connect Discovery form (RFC 8414 Section 5).
  OpenIDConfigurationAppended
};

/// @ingroup oauth
/// Derive a metadata well-known URL from an identifier and append it to the
/// sink, returning whether the identifier is well formed (RFC 8414 Section 3,
/// RFC 9728 Section 3). The identifier must use the `https` scheme and carry no
/// fragment, and no query unless it is a protected resource. A terminating
/// slash on the path is removed, and for the inserted kinds the well-known
/// string is placed between the host and the path, preserving a protected
/// resource query after it. The sink is appended to and never cleared. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// std::string url;
/// assert(sourcemeta::core::oauth_well_known_url(
///     "https://example.com/issuer1",
///     sourcemeta::core::OAuthWellKnownKind::AuthorizationServer, url));
/// assert(url ==
///        "https://example.com/.well-known/oauth-authorization-server/issuer1");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_well_known_url(const std::string_view identifier,
                          const OAuthWellKnownKind kind, std::string &sink)
    -> bool;

/// @ingroup oauth
/// Whether a URL is usable as an endpoint a client sends requests to: the https
/// scheme, compared case-insensitively per RFC 3986 Section 3.1, a non-empty
/// host, and no fragment, which RFC 6749 Section 3.1 forbids on an endpoint. A
/// query is permitted. This is the rule the metadata parsers apply, named for
/// the endpoint case rather than as a general https test, since the fragment
/// prohibition comes from the endpoint specifications and does not hold of
/// https URLs at large. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_is_endpoint_url("https://example.com/token"));
/// assert(!sourcemeta::core::oauth_is_endpoint_url("http://example.com/token"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_is_endpoint_url(const std::string_view value) -> bool;

/// @ingroup oauth
/// Whether a value is a valid protected resource identifier: the `https`
/// scheme by exact code points, a non-empty host, and no fragment, a query
/// tolerated (RFC 9728 Section 1.2, RFC 8707 Section 2). This is the rule the
/// resource metadata builder and parser apply, so a caller assembling a
/// document can pre-filter with it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_is_resource_identifier(
///     "https://api.example.com/mcp?tenant=1"));
/// assert(!sourcemeta::core::oauth_is_resource_identifier(
///     "http://api.example.com/mcp"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_is_resource_identifier(const std::string_view value) -> bool;

/// @ingroup oauth
/// Whether a value is a valid authorization server issuer identifier: the
/// `https` scheme by exact code points, a non-empty host, and no query or
/// fragment (RFC 8414 Section 2). This is the rule the metadata builders
/// apply to `issuer` and to `authorization_servers` entries, so a caller
/// assembling a document can pre-filter with it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_is_issuer_identifier(
///     "https://auth.example.com/tenant"));
/// assert(!sourcemeta::core::oauth_is_issuer_identifier(
///     "https://auth.example.com/?tenant=1"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_is_issuer_identifier(const std::string_view value) -> bool;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup oauth
/// An authorization server metadata document (RFC 8414), owning its JSON. The
/// document is validated on construction against the issuer it was retrieved
/// for: the issuer must match by exact code points and be a valid issuer
/// identifier, `response_types_supported` must be present and non-empty, and an
/// authentication method that needs a signing algorithm list must carry a
/// non-empty one without `none`. Accessors apply the specification defaults,
/// and any member without a typed accessor is reachable through the underlying
/// document. A string accessor returns a view into the owned document, valid
/// for the lifetime of this object, so a view taken before the object is moved
/// from must not be used afterward. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto document{sourcemeta::core::parse_json(
///     R"JSON({"issuer":"https://example.com",
///            "response_types_supported":["code"],
///            "authorization_endpoint":"https://example.com/authorize",
///            "token_endpoint":"https://example.com/token"})JSON")};
/// const auto metadata{sourcemeta::core::OAuthServerMetadata::from(
///     std::move(document), "https://example.com")};
/// assert(metadata.has_value());
/// assert(metadata.value().token_endpoint().value() ==
///        "https://example.com/token");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthServerMetadata {
public:
  /// Construct and validate a metadata document for an expected issuer,
  /// throwing when it is invalid. The document is moved in.
  OAuthServerMetadata(JSON &&data, const std::string_view issuer);

  /// Construct and validate a metadata document for an expected issuer,
  /// returning no value when it is invalid. The document is moved in.
  [[nodiscard]] static auto from(JSON &&data, const std::string_view issuer)
      -> std::optional<OAuthServerMetadata>;

  /// The issuer identifier (RFC 8414 Section 2).
  [[nodiscard]] auto issuer() const -> std::string_view;

  /// The authorization endpoint (RFC 8414 Section 2).
  [[nodiscard]] auto authorization_endpoint() const
      -> std::optional<std::string_view>;

  /// The token endpoint (RFC 8414 Section 2).
  [[nodiscard]] auto token_endpoint() const -> std::optional<std::string_view>;

  /// The dynamic client registration endpoint (RFC 8414 Section 2).
  [[nodiscard]] auto registration_endpoint() const
      -> std::optional<std::string_view>;

  /// The device authorization endpoint (RFC 8628 Section 4).
  [[nodiscard]] auto device_authorization_endpoint() const
      -> std::optional<std::string_view>;

  /// The token revocation endpoint (RFC 8414 Section 2).
  [[nodiscard]] auto revocation_endpoint() const
      -> std::optional<std::string_view>;

  /// The token introspection endpoint (RFC 8414 Section 2).
  [[nodiscard]] auto introspection_endpoint() const
      -> std::optional<std::string_view>;

  /// The JWK Set document location (RFC 8414 Section 2).
  [[nodiscard]] auto jwks_uri() const -> std::optional<std::string_view>;

  /// The pushed authorization request endpoint (RFC 9126 Section 5).
  [[nodiscard]] auto pushed_authorization_request_endpoint() const
      -> std::optional<std::string_view>;

  /// Whether the server accepts authorization request data only through the
  /// pushed authorization request endpoint, defaulting to false when absent
  /// (RFC 9126 Section 5).
  [[nodiscard]] auto require_pushed_authorization_requests() const -> bool;

  /// Whether the server signs authorization responses with an `iss` parameter,
  /// defaulting to false when absent (RFC 9207 Section 3).
  [[nodiscard]] auto authorization_response_iss_parameter_supported() const
      -> bool;

  /// Whether a response type is supported (RFC 8414 Section 2).
  [[nodiscard]] auto supports_response_type(const std::string_view value) const
      -> bool;

  /// Whether a grant type is supported, defaulting to the authorization code
  /// and implicit grants when absent (RFC 8414 Section 2).
  [[nodiscard]] auto supports_grant_type(const std::string_view value) const
      -> bool;

  /// Whether a PKCE code challenge method is supported, defaulting to none when
  /// absent since an omitted list means PKCE is unsupported (RFC 8414
  /// Section 2).
  [[nodiscard]] auto
  supports_code_challenge_method(const std::string_view value) const -> bool;

  /// Whether a token endpoint authentication method is supported, defaulting to
  /// `client_secret_basic` when absent (RFC 8414 Section 2).
  [[nodiscard]] auto
  supports_token_endpoint_auth_method(const std::string_view value) const
      -> bool;

  /// Whether a protected resource is listed as usable with this authorization
  /// server (RFC 9728 Section 4).
  [[nodiscard]] auto
  supports_protected_resource(const std::string_view value) const -> bool;

  /// The underlying document, for reaching members without a typed accessor.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  JSON data_;
};

/// @ingroup oauth
/// A protected resource metadata document (RFC 9728), owning its JSON. The
/// document is validated on construction against the resource it was retrieved
/// for: `resource` must be present, a valid resource identifier (an https URL
/// with no fragment, a query tolerated), and identical by code points to the
/// expected resource (RFC 9728 Section 3.3). Pass the resource identifier the
/// well-known URL was derived from, or, for the `WWW-Authenticate`
/// `resource_metadata` flow, the URL the request was made to (the Section 3.3
/// second check). Only the plain JSON members are read, so a `signed_metadata`
/// statement is not processed, which RFC 9728 Section 2.2 permits by making
/// its precedence conditional on a consumer that "supports signed metadata".
/// A string accessor returns a view into the owned document, valid for the
/// lifetime of this object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto document{sourcemeta::core::parse_json(R"JSON({
///   "resource": "https://api.example",
///   "authorization_servers": [ "https://auth.example" ]
/// })JSON")};
/// const auto metadata{sourcemeta::core::OAuthResourceMetadata::from(
///     std::move(document), "https://api.example")};
/// assert(metadata.has_value());
/// assert(metadata.value().first_authorization_server().value() ==
///        "https://auth.example");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthResourceMetadata {
public:
  /// Construct and validate a metadata document for an expected resource,
  /// throwing when it is invalid. The document is moved in.
  OAuthResourceMetadata(JSON &&data, const std::string_view resource);

  /// Construct and validate a metadata document for an expected resource,
  /// returning no value when it is invalid. The document is moved in.
  [[nodiscard]] static auto from(JSON &&data, const std::string_view resource)
      -> std::optional<OAuthResourceMetadata>;

  /// The resource identifier (RFC 9728 Section 2).
  [[nodiscard]] auto resource() const -> std::string_view;

  /// The first authorization server that can issue tokens for the resource,
  /// which a client resolves to its metadata (RFC 9728 Section 5), or no value
  /// when none is listed.
  [[nodiscard]] auto first_authorization_server() const
      -> std::optional<std::string_view>;

  /// Whether an authorization server issuer is listed (RFC 9728 Section 2).
  [[nodiscard]] auto
  supports_authorization_server(const std::string_view value) const -> bool;

  /// The JWK Set document location for the resource's own keys (RFC 9728
  /// Section 2).
  [[nodiscard]] auto jwks_uri() const -> std::optional<std::string_view>;

  /// Whether a bearer token presentation method is listed (RFC 9728 Section 2),
  /// where an explicit empty list means none is supported. Absence is
  /// unspecified rather than unsupported, so this returns false then.
  [[nodiscard]] auto supports_bearer_method(const std::string_view value) const
      -> bool;

  /// Whether a scope is listed for the resource (RFC 9728 Section 2).
  [[nodiscard]] auto supports_scope(const std::string_view value) const -> bool;

  /// Whether the resource requires DPoP-bound access tokens, defaulting to
  /// false when absent (RFC 9728 Section 2).
  [[nodiscard]] auto dpop_bound_access_tokens_required() const -> bool;

  /// The human-readable name of the resource without a language tag (RFC 9728
  /// Section 2), reaching a language-tagged variant through the underlying
  /// document.
  [[nodiscard]] auto resource_name() const -> std::optional<std::string_view>;

  /// The developer documentation page location (RFC 9728 Section 2).
  [[nodiscard]] auto resource_documentation() const
      -> std::optional<std::string_view>;

  /// The data usage policy page location (RFC 9728 Section 2).
  [[nodiscard]] auto resource_policy_uri() const
      -> std::optional<std::string_view>;

  /// The terms of service page location (RFC 9728 Section 2).
  [[nodiscard]] auto resource_tos_uri() const
      -> std::optional<std::string_view>;

  /// Whether the resource supports mutual-TLS certificate-bound access tokens,
  /// defaulting to false when absent (RFC 9728 Section 2).
  [[nodiscard]] auto tls_client_certificate_bound_access_tokens() const -> bool;

  /// Whether a JWS algorithm is listed for signing resource responses
  /// (RFC 9728 Section 2).
  [[nodiscard]] auto
  supports_resource_signing_alg(const std::string_view value) const -> bool;

  /// Whether a JWS algorithm is listed for validating DPoP proofs (RFC 9728
  /// Section 2).
  [[nodiscard]] auto
  supports_dpop_signing_alg(const std::string_view value) const -> bool;

  /// Whether an authorization details type is listed for the resource
  /// (RFC 9728 Section 2).
  [[nodiscard]] auto
  supports_authorization_details_type(const std::string_view value) const
      -> bool;

  /// The underlying document, for reaching members without a typed accessor
  /// such as the internationalized names.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  JSON data_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @ingroup oauth
/// The configuration an authorization server publishes as its metadata
/// (RFC 8414 Section 2), each field a non-owning view. An empty scalar and a
/// zero-element array are omitted, since RFC 8414 Section 3.2 forbids a
/// zero-element array in the response.
struct OAuthServerMetadataConfig {
  /// The issuer identifier (RFC 8414 Section 2), REQUIRED.
  std::string_view issuer;
  /// The authorization endpoint (RFC 8414 Section 2).
  std::string_view authorization_endpoint;
  /// The token endpoint (RFC 8414 Section 2).
  std::string_view token_endpoint;
  /// The dynamic client registration endpoint (RFC 8414 Section 2).
  std::string_view registration_endpoint;
  /// The JWK Set document location (RFC 8414 Section 2).
  std::string_view jwks_uri;
  /// The supported response types (RFC 8414 Section 2), REQUIRED and non-empty.
  std::span<const std::string_view> response_types_supported;
  /// The supported grant types (RFC 8414 Section 2).
  std::span<const std::string_view> grant_types_supported;
  /// The supported PKCE code challenge methods (RFC 8414 Section 2, RFC 7636).
  std::span<const std::string_view> code_challenge_methods_supported;
  /// The supported token endpoint authentication methods (RFC 8414 Section 2).
  std::span<const std::string_view> token_endpoint_auth_methods_supported;
  /// The supported JWS algorithms for the `private_key_jwt` and
  /// `client_secret_jwt` token endpoint authentication methods (RFC 8414
  /// Section 2). REQUIRED and must exclude `none` when either of those methods
  /// is advertised.
  std::span<const std::string_view>
      token_endpoint_auth_signing_alg_values_supported;
  /// The supported scopes (RFC 8414 Section 2).
  std::span<const std::string_view> scopes_supported;
  /// The pushed authorization request endpoint (RFC 9126 Section 5). New
  /// members are kept at the end so an existing positional initializer keeps
  /// mapping to the older fields.
  std::string_view pushed_authorization_request_endpoint;
  /// Whether authorization request data is accepted only through the pushed
  /// authorization request endpoint, emitted only when true (RFC 9126
  /// Section 5).
  bool require_pushed_authorization_requests{false};
  /// The protected resources usable with this authorization server, each a
  /// valid resource identifier (RFC 9728 Section 4).
  std::span<const std::string_view> protected_resources;
};

/// @ingroup oauth
/// Build an authorization server metadata document for the well-known endpoint
/// (RFC 8414 Section 2), returning no value when the document would be
/// unusable: the issuer is not a valid issuer identifier, the required response
/// types are empty, the required authorization endpoint or (unless only the
/// implicit grant is offered) token endpoint is missing, any advertised
/// endpoint or JWK Set location is not a valid https URL, the signing algorithm
/// list contains `none`, or a JWT token endpoint authentication method is
/// advertised without a non-empty signing algorithm list. Each present scalar
/// and each non-empty array is emitted, and a zero-element array is omitted
/// (RFC 8414 Section 3.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 1> response_types{{"code"}};
/// sourcemeta::core::OAuthServerMetadataConfig config;
/// config.issuer = "https://server.example";
/// config.response_types_supported = response_types;
/// const auto document{sourcemeta::core::oauth_make_server_metadata(config)};
/// assert(document.has_value());
/// assert(document.value().at("issuer").to_string() ==
///        "https://server.example");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_make_server_metadata(const OAuthServerMetadataConfig &config)
    -> std::optional<JSON>;

/// @ingroup oauth
/// The configuration a protected resource publishes as its metadata (RFC 9728
/// Section 2), each field a non-owning view. An empty scalar and a
/// zero-element array are omitted, since RFC 9728 Section 3.2 forbids a
/// zero-element array in the response. The one exception is the bearer method
/// list, whose engaged empty state is emitted as an empty array, the form
/// RFC 9728 Section 2 gives for a resource that supports no bearer method,
/// distinct from the unspecified absent state.
struct OAuthResourceMetadataConfig {
  /// The resource identifier (RFC 9728 Section 2), REQUIRED.
  std::string_view resource;
  /// The authorization server issuer identifiers that can issue tokens for the
  /// resource (RFC 9728 Section 2).
  std::span<const std::string_view> authorization_servers;
  /// The JWK Set document location for the resource's own keys (RFC 9728
  /// Section 2).
  std::string_view jwks_uri;
  /// The scopes used in authorization requests for the resource (RFC 9728
  /// Section 2).
  std::span<const std::string_view> scopes_supported;
  /// The supported bearer token presentation methods (RFC 9728 Section 2),
  /// where no value omits the member and an engaged empty list advertises that
  /// no bearer method is supported.
  std::optional<std::span<const std::string_view>> bearer_methods_supported;
  /// The supported JWS algorithms for signing resource responses (RFC 9728
  /// Section 2), which must exclude `none`.
  std::span<const std::string_view> resource_signing_alg_values_supported;
  /// The human-readable name of the resource without a language tag (RFC 9728
  /// Sections 2 and 2.1), a language-tagged variant assigned by the caller on
  /// the returned document.
  std::string_view resource_name;
  /// The developer documentation page location (RFC 9728 Section 2).
  std::string_view resource_documentation;
  /// The data usage policy page location (RFC 9728 Section 2).
  std::string_view resource_policy_uri;
  /// The terms of service page location (RFC 9728 Section 2).
  std::string_view resource_tos_uri;
  /// Whether the resource supports mutual-TLS certificate-bound access tokens,
  /// emitted only when true since the default when absent is false (RFC 9728
  /// Section 2).
  bool tls_client_certificate_bound_access_tokens{false};
  /// The supported authorization details types (RFC 9728 Section 2).
  std::span<const std::string_view> authorization_details_types_supported;
  /// The supported JWS algorithms for validating DPoP proofs (RFC 9728
  /// Section 2), which must exclude `none` and the MAC algorithms a proof may
  /// never use (RFC 9449 Section 4.2).
  std::span<const std::string_view> dpop_signing_alg_values_supported;
  /// Whether the resource requires DPoP-bound access tokens, emitted only when
  /// true since the default when absent is false (RFC 9728 Section 2).
  bool dpop_bound_access_tokens_required{false};
};

/// @ingroup oauth
/// Build a protected resource metadata document for the well-known endpoint
/// (RFC 9728 Section 2), returning no value when the document would be
/// unusable: the resource is not a valid resource identifier, an authorization
/// server entry is not a valid issuer identifier, the JWK Set location is not
/// a valid https URL, a human-readable page location is not a URL, the
/// resource signing algorithm list contains `none`, or the DPoP algorithm
/// list contains `none` or a MAC algorithm. Every produced document parses
/// back through its own consumer for the same resource. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 1> servers{{"https://auth.example.com"}};
/// sourcemeta::core::OAuthResourceMetadataConfig config;
/// config.resource = "https://api.example.com";
/// config.authorization_servers = servers;
/// const auto document{sourcemeta::core::oauth_make_resource_metadata(config)};
/// assert(document.has_value());
/// assert(document.value().at("resource").to_string() ==
///        "https://api.example.com");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_make_resource_metadata(const OAuthResourceMetadataConfig &config)
    -> std::optional<JSON>;

} // namespace sourcemeta::core

#endif
