#ifndef SOURCEMETA_CORE_OAUTH_ERROR_H_
#define SOURCEMETA_CORE_OAUTH_ERROR_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/http.h>

#include <cstdint>     // std::uint8_t
#include <exception>   // std::exception
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The error codes an authorization endpoint returns in its redirect
/// (RFC 6749 Section 4.1.2.1).
enum class OAuthAuthorizationError : std::uint8_t {
  /// The request is missing a parameter, includes an invalid value, or is
  /// otherwise malformed.
  InvalidRequest,
  /// The client is not authorized to request a code using this method.
  UnauthorizedClient,
  /// The resource owner or authorization server denied the request.
  AccessDenied,
  /// The authorization server does not support this response type.
  UnsupportedResponseType,
  /// The requested scope is invalid, unknown, or malformed.
  InvalidScope,
  /// The authorization server encountered an unexpected condition.
  ServerError,
  /// The authorization server is temporarily unable to handle the request.
  TemporarilyUnavailable
};

/// @ingroup oauth
/// The error codes a token endpoint returns (RFC 6749 Section 5.2), extended
/// with the codes the device grant, resource indicators, token exchange, DPoP,
/// and token revocation add to the same endpoint family.
enum class OAuthTokenError : std::uint8_t {
  /// The request is missing a parameter, includes an unsupported value, or is
  /// otherwise malformed (RFC 6749 Section 5.2).
  InvalidRequest,
  /// Client authentication failed (RFC 6749 Section 5.2).
  InvalidClient,
  /// The grant or refresh token is invalid, expired, revoked, or mismatched
  /// (RFC 6749 Section 5.2).
  InvalidGrant,
  /// The authenticated client is not authorized to use this grant type
  /// (RFC 6749 Section 5.2).
  UnauthorizedClient,
  /// The grant type is not supported by the authorization server (RFC 6749
  /// Section 5.2).
  UnsupportedGrantType,
  /// The requested scope is invalid, unknown, or exceeds the grant (RFC 6749
  /// Section 5.2).
  InvalidScope,
  /// The device authorization is still pending user approval (RFC 8628
  /// Section 3.5).
  AuthorizationPending,
  /// The client is polling too frequently and must slow down (RFC 8628
  /// Section 3.5).
  SlowDown,
  /// The end user denied the device authorization request (RFC 8628
  /// Section 3.5).
  AccessDenied,
  /// The device code expired before the user approved it (RFC 8628
  /// Section 3.5).
  ExpiredToken,
  /// The requested resource or audience is invalid or unknown (RFC 8707
  /// Section 2, RFC 8693 Section 2.2.2).
  InvalidTarget,
  /// The DPoP proof is missing or invalid (RFC 9449 Section 5).
  InvalidDPoPProof,
  /// The authorization server requires the client to use a DPoP nonce
  /// (RFC 9449 Section 8).
  UseDPoPNonce,
  /// The token type is not supported by the revocation endpoint (RFC 7009
  /// Section 2.2.1).
  UnsupportedTokenType
};

/// @ingroup oauth
/// The error codes a protected resource returns in its `WWW-Authenticate`
/// challenge (RFC 6750 Section 3.1), shared by the `Bearer` and `DPoP`
/// schemes.
enum class OAuthBearerError : std::uint8_t {
  /// The request is malformed (RFC 6750 Section 3.1).
  InvalidRequest,
  /// The access token is expired, revoked, malformed, or otherwise invalid
  /// (RFC 6750 Section 3.1).
  InvalidToken,
  /// The token does not carry the scope the request requires (RFC 6750
  /// Section 3.1).
  InsufficientScope,
  /// The DPoP proof accompanying the request is missing or invalid, returned
  /// only under the `DPoP` scheme (RFC 9449 Section 7.1).
  InvalidDPoPProof,
  /// The request must be retried with a DPoP proof that carries a nonce the
  /// resource server supplies, returned only under the `DPoP` scheme (RFC 9449
  /// Section 9).
  UseDPoPNonce
};

/// @ingroup oauth
/// The error codes a dynamic client registration endpoint returns (RFC 7591
/// Section 3.2.2).
enum class OAuthRegistrationError : std::uint8_t {
  /// A redirect URI in the request is invalid.
  InvalidRedirectURI,
  /// A field in the client metadata is invalid.
  InvalidClientMetadata,
  /// The software statement is invalid.
  InvalidSoftwareStatement,
  /// The software statement is not approved by the authorization server.
  UnapprovedSoftwareStatement
};

/// @ingroup oauth
/// The wire code for an authorization endpoint error (RFC 6749
/// Section 4.1.2.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_error_code(
///            sourcemeta::core::OAuthAuthorizationError::AccessDenied) ==
///        "access_denied");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_error_code(const OAuthAuthorizationError error) noexcept
    -> std::string_view;

/// @ingroup oauth
/// The wire code for a token endpoint error (RFC 6749 Section 5.2 and its
/// extensions). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_error_code(
///            sourcemeta::core::OAuthTokenError::InvalidGrant) ==
///        "invalid_grant");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_error_code(const OAuthTokenError error) noexcept -> std::string_view;

/// @ingroup oauth
/// The wire code for a protected resource challenge error (RFC 6750
/// Section 3.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_error_code(
///            sourcemeta::core::OAuthBearerError::InvalidToken) ==
///        "invalid_token");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_error_code(const OAuthBearerError error) noexcept
    -> std::string_view;

/// @ingroup oauth
/// The wire code for a dynamic client registration error (RFC 7591
/// Section 3.2.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_error_code(
///            sourcemeta::core::OAuthRegistrationError::InvalidRedirectURI) ==
///        "invalid_redirect_uri");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_error_code(const OAuthRegistrationError error) noexcept
    -> std::string_view;

/// @ingroup oauth
/// Map an authorization endpoint error code to its value, returning no value
/// for an unrecognized code (RFC 6749 Section 4.1.2.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oauth_authorization_error("invalid_scope")
///            .has_value());
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto to_oauth_authorization_error(const std::string_view code) noexcept
    -> std::optional<OAuthAuthorizationError>;

/// @ingroup oauth
/// Map a token endpoint error code to its value, returning no value for an
/// unrecognized code (RFC 6749 Section 5.2 and its extensions). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oauth_token_error("slow_down").has_value());
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto to_oauth_token_error(const std::string_view code) noexcept
    -> std::optional<OAuthTokenError>;

/// @ingroup oauth
/// Map a protected resource challenge error code to its value, returning no
/// value for an unrecognized code (RFC 6750 Section 3.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oauth_bearer_error("invalid_token").has_value());
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto to_oauth_bearer_error(const std::string_view code) noexcept
    -> std::optional<OAuthBearerError>;

/// @ingroup oauth
/// Map a dynamic client registration error code to its value, returning no
/// value for an unrecognized code (RFC 7591 Section 3.2.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oauth_registration_error("invalid_redirect_uri")
///            .has_value());
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto to_oauth_registration_error(const std::string_view code) noexcept
    -> std::optional<OAuthRegistrationError>;

/// @ingroup oauth
/// The HTTP status for a token endpoint error response. It is 400 Bad Request
/// in general, but 401 Unauthorized for a client authentication failure when
/// the client authenticated through the `Authorization` header, since that
/// response must then carry a `WWW-Authenticate` challenge (RFC 6749
/// Section 5.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_token_error_status(
///            sourcemeta::core::OAuthTokenError::InvalidClient, true).code ==
///        401);
/// assert(sourcemeta::core::oauth_token_error_status(
///            sourcemeta::core::OAuthTokenError::InvalidGrant, true).code ==
///        400);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_token_error_status(const OAuthTokenError error,
                              const bool authenticated_via_header) noexcept
    -> HTTPStatus;

/// @ingroup oauth
/// The HTTP status a protected resource returns for a bearer error, as the
/// SHOULD-level recommendation of RFC 6750 Section 3.1: 400 for a malformed
/// request, 401 for an invalid token, and 403 for insufficient scope. The DPoP
/// resource codes accompany a 401 (RFC 9449 Section 7). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_bearer_error_status(
///            sourcemeta::core::OAuthBearerError::InsufficientScope).code ==
///        403);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_bearer_error_status(const OAuthBearerError error) noexcept
    -> HTTPStatus;

#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup oauth
/// An error that occurs when parsing an invalid authorization server or
/// protected resource metadata document.
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthMetadataParseError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid OAuth metadata document";
  }
};

/// @ingroup oauth
/// An error that occurs when parsing an invalid dynamic client registration
/// request or response.
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthRegistrationParseError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid OAuth client registration document";
  }
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
