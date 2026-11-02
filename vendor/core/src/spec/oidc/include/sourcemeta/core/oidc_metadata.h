#ifndef SOURCEMETA_CORE_OIDC_METADATA_H_
#define SOURCEMETA_CORE_OIDC_METADATA_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth.h>

#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup oidc
/// An OpenID Provider metadata document (OpenID Connect Discovery 1.0), owning
/// its JSON. It is a superset of an OAuth authorization server metadata
/// document, validated on construction against the issuer it was retrieved for:
/// the OAuth checks apply, and in addition `jwks_uri`,
/// `subject_types_supported`, and `id_token_signing_alg_values_supported` are
/// REQUIRED and non-empty, and the signing algorithm list must include `RS256`.
/// A string accessor returns a view into the owned document, valid for the
/// lifetime of this object. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto document{sourcemeta::core::parse_json(
///     R"JSON({"issuer":"https://example.com",
///            "authorization_endpoint":"https://example.com/authorize",
///            "token_endpoint":"https://example.com/token",
///            "jwks_uri":"https://example.com/jwks",
///            "response_types_supported":["code"],
///            "subject_types_supported":["public"],
///            "id_token_signing_alg_values_supported":["RS256"]})JSON")};
/// const auto metadata{sourcemeta::core::OIDCProviderMetadata::from(
///     std::move(document), "https://example.com")};
/// assert(metadata.has_value());
/// assert(metadata.value().jwks_uri() == "https://example.com/jwks");
/// ```
class SOURCEMETA_CORE_OIDC_EXPORT OIDCProviderMetadata {
public:
  /// Construct and validate a metadata document for an expected issuer,
  /// throwing when it is invalid. The document is moved in.
  OIDCProviderMetadata(JSON &&data, const std::string_view issuer);

  /// Apply the OpenID Connect layer to a document already parsed and validated
  /// as OAuth authorization server metadata, throwing when the OpenID Connect
  /// requirements are not met. The document is moved in and the OAuth checks
  /// are not repeated.
  explicit OIDCProviderMetadata(OAuthServerMetadata &&oauth);

  /// Construct and validate a metadata document for an expected issuer,
  /// returning no value when it is invalid. The document is moved in.
  [[nodiscard]] static auto from(JSON &&data, const std::string_view issuer)
      -> std::optional<OIDCProviderMetadata>;

  /// Apply the OpenID Connect layer to a document already parsed and validated
  /// as OAuth authorization server metadata, returning no value when the
  /// OpenID Connect requirements are not met. This is what a caching resolver
  /// hands back, so lifting it costs no reparse and no repeated OAuth
  /// validation. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/oidc.h>
  /// #include <cassert>
  ///
  /// auto document{sourcemeta::core::parse_json(R"JSON({
  ///   "issuer":"https://example.com",
  ///   "jwks_uri":"https://example.com/jwks",
  ///   "response_types_supported":[ "code" ],
  ///   "subject_types_supported":[ "public" ],
  ///   "id_token_signing_alg_values_supported":[ "RS256" ]
  /// })JSON")};
  /// auto oauth{sourcemeta::core::OAuthServerMetadata::from(
  ///     std::move(document), "https://example.com")};
  /// assert(oauth.has_value());
  /// const auto metadata{
  ///     sourcemeta::core::OIDCProviderMetadata::from(std::move(oauth).value())};
  /// assert(metadata.has_value());
  /// ```
  [[nodiscard]] static auto from(OAuthServerMetadata &&oauth)
      -> std::optional<OIDCProviderMetadata>;

  /// The issuer identifier (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto issuer() const -> std::string_view;

  /// The authorization endpoint (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto authorization_endpoint() const
      -> std::optional<std::string_view>;

  /// The token endpoint (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto token_endpoint() const -> std::optional<std::string_view>;

  /// The UserInfo endpoint (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto userinfo_endpoint() const
      -> std::optional<std::string_view>;

  /// The dynamic client registration endpoint (OpenID Connect Discovery 1.0
  /// Section 3).
  [[nodiscard]] auto registration_endpoint() const
      -> std::optional<std::string_view>;

  /// The JWK Set document location, REQUIRED by OpenID Connect (OpenID Connect
  /// Discovery 1.0 Section 3).
  [[nodiscard]] auto jwks_uri() const -> std::string_view;

  /// The RP-Initiated Logout end session endpoint (OpenID Connect RP-Initiated
  /// Logout 1.0 Section 2).
  [[nodiscard]] auto end_session_endpoint() const
      -> std::optional<std::string_view>;

  /// The Session Management check session iframe (OpenID Connect Session
  /// Management 1.0 Section 2.1).
  [[nodiscard]] auto check_session_iframe() const
      -> std::optional<std::string_view>;

  /// Whether a subject identifier type is supported (OpenID Connect Discovery
  /// 1.0 Section 3).
  [[nodiscard]] auto supports_subject_type(const std::string_view value) const
      -> bool;

  /// Whether an ID Token signing algorithm is supported (OpenID Connect
  /// Discovery 1.0 Section 3).
  [[nodiscard]] auto
  supports_id_token_signing_alg(const std::string_view value) const -> bool;

  /// Whether a response type is supported (OpenID Connect Discovery 1.0
  /// Section 3).
  [[nodiscard]] auto supports_response_type(const std::string_view value) const
      -> bool;

  /// Whether a token endpoint authentication method is supported, defaulting
  /// to `client_secret_basic` when absent (OpenID Connect Discovery 1.0
  /// Section 3).
  [[nodiscard]] auto
  supports_token_endpoint_auth_method(const std::string_view value) const
      -> bool;

  /// Whether a scope is supported (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto supports_scope(const std::string_view value) const -> bool;

  /// Whether a claim is supported (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto supports_claim(const std::string_view value) const -> bool;

  /// Whether the `claims` request parameter is supported, absent meaning false
  /// (OpenID Connect Discovery 1.0 Section 3).
  [[nodiscard]] auto supports_claims_parameter() const -> bool;

  /// The underlying OAuth authorization server metadata this document is a
  /// superset of, for reaching the OAuth typed accessors.
  [[nodiscard]] auto oauth() const -> const OAuthServerMetadata &;

  /// The underlying document, for reaching members without a typed accessor.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  OAuthServerMetadata oauth_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @ingroup oidc
/// The configuration an OpenID Provider publishes as its metadata (OpenID
/// Connect Discovery 1.0 Section 3), a superset of the OAuth authorization
/// server configuration, each field a non-owning view. An empty scalar and a
/// zero-element array are omitted, and a capability flag is emitted only when
/// it differs from its specification default.
struct OIDCProviderMetadataConfig {
  /// The OAuth authorization server configuration this is a superset of. Its
  /// `jwks_uri` is REQUIRED for OpenID Connect.
  OAuthServerMetadataConfig base;
  /// The UserInfo endpoint (OpenID Connect Discovery 1.0 Section 3).
  std::string_view userinfo_endpoint;
  /// The RP-Initiated Logout end session endpoint (OpenID Connect RP-Initiated
  /// Logout 1.0 Section 3).
  std::string_view end_session_endpoint;
  /// The Session Management check session iframe (OpenID Connect Session
  /// Management 1.0 Section 2.1).
  std::string_view check_session_iframe;
  /// The supported subject identifier types (OpenID Connect Discovery 1.0
  /// Section 3), REQUIRED and non-empty.
  std::span<const std::string_view> subject_types_supported;
  /// The supported ID Token signing algorithms (OpenID Connect Discovery 1.0
  /// Section 3), REQUIRED, non-empty, and including `RS256`. The module never
  /// advertises `none`.
  std::span<const std::string_view> id_token_signing_alg_values_supported;
  /// The supported ID Token encryption algorithms (OpenID Connect Discovery 1.0
  /// Section 3).
  std::span<const std::string_view> id_token_encryption_alg_values_supported;
  /// The supported ID Token content encryptions (OpenID Connect Discovery 1.0
  /// Section 3).
  std::span<const std::string_view> id_token_encryption_enc_values_supported;
  /// The supported UserInfo signing algorithms (OpenID Connect Discovery 1.0
  /// Section 3).
  std::span<const std::string_view> userinfo_signing_alg_values_supported;
  /// The supported UserInfo encryption algorithms (OpenID Connect Discovery 1.0
  /// Section 3).
  std::span<const std::string_view> userinfo_encryption_alg_values_supported;
  /// The supported UserInfo content encryptions (OpenID Connect Discovery 1.0
  /// Section 3).
  std::span<const std::string_view> userinfo_encryption_enc_values_supported;
  /// The supported request object signing algorithms (OpenID Connect Discovery
  /// 1.0 Section 3).
  std::span<const std::string_view> request_object_signing_alg_values_supported;
  /// The supported request object encryption algorithms (OpenID Connect
  /// Discovery 1.0 Section 3).
  std::span<const std::string_view>
      request_object_encryption_alg_values_supported;
  /// The supported request object content encryptions (OpenID Connect Discovery
  /// 1.0 Section 3).
  std::span<const std::string_view>
      request_object_encryption_enc_values_supported;
  /// The supported claims (OpenID Connect Discovery 1.0 Section 3).
  std::span<const std::string_view> claims_supported;
  /// The supported authentication context class references (OpenID Connect
  /// Discovery 1.0 Section 3).
  std::span<const std::string_view> acr_values_supported;
  /// The supported claim types (OpenID Connect Discovery 1.0 Section 3).
  std::span<const std::string_view> claim_types_supported;
  /// The supported display values (OpenID Connect Discovery 1.0 Section 3).
  std::span<const std::string_view> display_values_supported;
  /// The supported claims locales (OpenID Connect Discovery 1.0 Section 3).
  std::span<const std::string_view> claims_locales_supported;
  /// The supported UI locales (OpenID Connect Discovery 1.0 Section 3).
  std::span<const std::string_view> ui_locales_supported;
  /// Whether the `claims` request parameter is supported, default false, so
  /// emitted only when true (OpenID Connect Discovery 1.0 Section 3).
  bool claims_parameter_supported{false};
  /// Whether the `request` request parameter is supported, default false, so
  /// emitted only when true (OpenID Connect Discovery 1.0 Section 3).
  bool request_parameter_supported{false};
  /// Whether the `request_uri` request parameter is supported, default true, so
  /// emitted only when false (OpenID Connect Discovery 1.0 Section 3).
  bool request_uri_parameter_supported{true};
  /// Whether a `request_uri` must be pre-registered, default false, so emitted
  /// only when true (OpenID Connect Discovery 1.0 Section 3).
  bool require_request_uri_registration{false};
};

/// @ingroup oidc
/// Build an OpenID Provider metadata document for the well-known endpoint
/// (OpenID Connect Discovery 1.0 Section 3), returning no value when the
/// document would be unusable: the OAuth base is unusable, the REQUIRED
/// `jwks_uri`, `subject_types_supported`, or
/// `id_token_signing_alg_values_supported` is missing or empty, the ID Token
/// signing algorithm list omits `RS256` or contains `none`, or an advertised
/// OpenID Connect endpoint is not a valid https URL. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 1> response_types{{"code"}};
/// const std::array<std::string_view, 1> subject_types{{"public"}};
/// const std::array<std::string_view, 1> id_token_algs{{"RS256"}};
/// sourcemeta::core::OIDCProviderMetadataConfig config;
/// config.base.issuer = "https://server.example";
/// config.base.authorization_endpoint = "https://server.example/authorize";
/// config.base.token_endpoint = "https://server.example/token";
/// config.base.jwks_uri = "https://server.example/jwks";
/// config.base.response_types_supported = response_types;
/// config.subject_types_supported = subject_types;
/// config.id_token_signing_alg_values_supported = id_token_algs;
/// const auto document{sourcemeta::core::oidc_make_provider_metadata(config)};
/// assert(document.has_value());
/// assert(document.value().at("issuer").to_string() ==
///        "https://server.example");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_make_provider_metadata(const OIDCProviderMetadataConfig &config)
    -> std::optional<JSON>;

} // namespace sourcemeta::core

#endif
