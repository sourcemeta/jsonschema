#ifndef SOURCEMETA_CORE_OIDC_REGISTRATION_H_
#define SOURCEMETA_CORE_OIDC_REGISTRATION_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth.h>

#include <chrono>      // std::chrono::seconds
#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup oidc
/// An OpenID Connect client registration document (OpenID Connect Dynamic
/// Client Registration 1.0), owning its JSON. It is a superset of an OAuth
/// client registration document, validated on construction: the OAuth checks
/// apply, and in addition `redirect_uris` is REQUIRED and non-empty, which
/// OpenID Connect tightens from the OAuth OPTIONAL. A string accessor returns a
/// view into the owned document, valid for the lifetime of this object. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// auto document{sourcemeta::core::parse_json(
///     R"JSON({"redirect_uris":["https://client.example/cb"],
///            "subject_type":"public"})JSON")};
/// const auto metadata{
///     sourcemeta::core::OIDCClientMetadata::from(std::move(document))};
/// assert(metadata.has_value());
/// assert(metadata.value().application_type() == "web");
/// ```
class SOURCEMETA_CORE_OIDC_EXPORT OIDCClientMetadata {
public:
  /// Construct and validate a client registration document, throwing when it is
  /// invalid. The document is moved in.
  explicit OIDCClientMetadata(JSON &&data);

  /// Construct and validate a client registration document, returning no value
  /// when it is invalid. The document is moved in.
  [[nodiscard]] static auto from(JSON &&data)
      -> std::optional<OIDCClientMetadata>;

  /// Whether a redirection URI is registered (OpenID Connect Dynamic Client
  /// Registration 1.0 Section 2).
  [[nodiscard]] auto has_redirect_uri(const std::string_view value) const
      -> bool;

  /// The application type, defaulting to `web` (OpenID Connect Dynamic Client
  /// Registration 1.0 Section 2).
  [[nodiscard]] auto application_type() const -> std::string_view;

  /// The subject identifier type (OpenID Connect Dynamic Client Registration
  /// 1.0 Section 2).
  [[nodiscard]] auto subject_type() const -> std::optional<std::string_view>;

  /// The sector identifier URI for pairwise subjects (OpenID Connect Dynamic
  /// Client Registration 1.0 Section 2).
  [[nodiscard]] auto sector_identifier_uri() const
      -> std::optional<std::string_view>;

  /// The ID Token signing algorithm, defaulting to `RS256` (OpenID Connect
  /// Dynamic Client Registration 1.0 Section 2).
  [[nodiscard]] auto id_token_signed_response_alg() const -> std::string_view;

  /// The ID Token encryption algorithm (OpenID Connect Dynamic Client
  /// Registration 1.0 Section 2).
  [[nodiscard]] auto id_token_encrypted_response_alg() const
      -> std::optional<std::string_view>;

  /// The UserInfo signing algorithm (OpenID Connect Dynamic Client Registration
  /// 1.0 Section 2).
  [[nodiscard]] auto userinfo_signed_response_alg() const
      -> std::optional<std::string_view>;

  /// The default maximum authentication age (OpenID Connect Dynamic Client
  /// Registration 1.0 Section 2).
  [[nodiscard]] auto default_max_age() const
      -> std::optional<std::chrono::seconds>;

  /// Whether the `auth_time` claim is always required, defaulting to false
  /// (OpenID Connect Dynamic Client Registration 1.0 Section 2).
  [[nodiscard]] auto require_auth_time() const -> bool;

  /// The URI a third party uses to initiate login (OpenID Connect Dynamic
  /// Client Registration 1.0 Section 2).
  [[nodiscard]] auto initiate_login_uri() const
      -> std::optional<std::string_view>;

  /// Whether a post-logout redirection URI is registered (OpenID Connect
  /// RP-Initiated Logout 1.0 Section 3).
  [[nodiscard]] auto
  has_post_logout_redirect_uri(const std::string_view value) const -> bool;

  /// The underlying OAuth client registration this document is a superset of.
  [[nodiscard]] auto oauth() const -> const OAuthClientMetadata &;

  /// The underlying document, for reaching members without a typed accessor.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  OAuthClientMetadata oauth_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @ingroup oidc
/// Whether a fetched `sector_identifier_uri` document, a JSON array of
/// redirection URIs, contains every registered redirection URI (OpenID Connect
/// Dynamic Client Registration 1.0 Section 5). The OpenID Provider performs
/// this check at registration before accepting a pairwise client. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const auto document{sourcemeta::core::parse_json(
///     R"JSON(["https://client.example/cb"])JSON")};
/// const std::array<std::string_view, 1> registered{
///     {"https://client.example/cb"}};
/// assert(sourcemeta::core::oidc_sector_identifier_contains(document,
///                                                          registered));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_sector_identifier_contains(
    const JSON &sector_document,
    const std::span<const std::string_view> redirect_uris) -> bool;

} // namespace sourcemeta::core

#endif
