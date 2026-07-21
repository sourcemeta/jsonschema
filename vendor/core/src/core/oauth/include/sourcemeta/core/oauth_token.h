#ifndef SOURCEMETA_CORE_OAUTH_TOKEN_H_
#define SOURCEMETA_CORE_OAUTH_TOKEN_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_authorization.h>

#include <chrono>      // std::chrono::seconds
#include <functional>  // std::function
#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// Append an authorization code grant token request body (RFC 6749
/// Section 4.1.3) to the sink. The `code` is required, the `redirect_uri` is
/// emitted only when the authorization request carried one, and the
/// `code_verifier` is emitted only when PKCE is in use (RFC 7636 Section 4.5).
/// No `client_id` is emitted, so the caller composes a client authentication
/// builder into the same sink. The body carries secrets, so the sink is a
/// wiping string, and it is appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_token_request_code(
///     "SplxlOBeZQQYbYS6WxSbIA", "https://client.example/cb", "", {}, body);
/// assert(body == "grant_type=authorization_code&code=SplxlOBeZQQYbYS6WxSbIA"
///                "&redirect_uri=https%3A%2F%2Fclient.example%2Fcb");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_token_request_code(
    const std::string_view code, const std::string_view redirect_uri,
    const std::string_view code_verifier,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void;

/// @ingroup oauth
/// Append a refresh token grant token request body (RFC 6749 Section 6) to the
/// sink. The `refresh_token` is required, and the requested `scope` is emitted
/// only when present and must not exceed the original grant. No `client_id` is
/// emitted, so the caller composes a client authentication builder into the
/// same sink. The body carries secrets, so the sink is a wiping string, and it
/// is appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_token_request_refresh("tGzv3JOkF0XG5Qx2TlKWIA",
///                                                     "read", {}, body);
/// assert(body == "grant_type=refresh_token"
///                "&refresh_token=tGzv3JOkF0XG5Qx2TlKWIA&scope=read");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_token_request_refresh(
    const std::string_view refresh_token, const std::string_view scope,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void;

/// @ingroup oauth
/// Append a client credentials grant token request body (RFC 6749
/// Section 4.4.2) to the sink. The requested `scope` is emitted only when
/// present. No `client_id` is emitted, so the caller composes a client
/// authentication builder into the same sink. The sink is appended to and never
/// cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_token_request_client_credentials("read", {},
///                                                                body);
/// assert(body == "grant_type=client_credentials&scope=read");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_token_request_client_credentials(
    const std::string_view scope,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void;

/// @ingroup oauth
/// A non-owning view of a token request at the authorization server (RFC 6749
/// Section 4.1.3, Section 4.4.2, Section 6). Every field borrows from the input
/// or the decode arena, and an absent parameter is an empty view. The grant
/// type is surfaced as its raw value so a server can tell a missing grant from
/// an unsupported one.
struct OAuthTokenRequest {
  /// The grant type (RFC 6749 Section 4.1.3).
  std::string_view grant_type;
  /// The authorization code, for the authorization code grant (RFC 6749
  /// Section 4.1.3).
  std::string_view code;
  /// The redirection endpoint echoed for the authorization code grant (RFC 6749
  /// Section 4.1.3), surfaced unconditionally.
  std::string_view redirect_uri;
  /// The PKCE code verifier (RFC 7636 Section 4.5).
  std::string_view code_verifier;
  /// The refresh token, for the refresh grant (RFC 6749 Section 6).
  std::string_view refresh_token;
  /// The space-delimited requested scope (RFC 6749 Section 3.3).
  std::string_view scope;
};

/// @ingroup oauth
/// Parse the body of a token request at the authorization server (RFC 6749
/// Section 4.1.3) into the result, returning whether it is well formed. Each
/// recognized value is form-decoded into the storage arena and viewed there.
/// The body carries the request's secrets, the authorization code, the code
/// verifier, the refresh token, and the client authentication parameters, so
/// the arena is a wiping string, which the caller owns, should clear between
/// independent parses, and must outlive the result. A duplicated recognized
/// parameter is a failure (RFC 6749 Section 3.2). Every repeatable `resource`
/// and `audience` and every unrecognized parameter, including the client
/// authentication parameters, is passed to the callback with its decoded value
/// rather than stored on the result. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString storage;
/// sourcemeta::core::OAuthTokenRequest request;
/// assert(sourcemeta::core::oauth_parse_token_request(
///     "grant_type=authorization_code&code=SplxlOBeZQQYbYS6WxSbIA", storage,
///     request, [](std::string_view, std::string_view) {}));
/// assert(request.grant_type == "authorization_code");
/// assert(request.code == "SplxlOBeZQQYbYS6WxSbIA");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_parse_token_request(
    const std::string_view body, SecureString &storage,
    OAuthTokenRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool;

/// @ingroup oauth
/// A non-owning view over a token endpoint success response (RFC 6749
/// Section 5.1) held in a caller-owned JSON value. The value must outlive the
/// view, and the token accessors return views into it, which are secret and
/// must not be logged. Extension members such as an OpenID Connect `id_token`
/// are reached through the underlying document. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(
///     R"JSON({"access_token":"2YotnFZFEjr1zCsicMWpAA","token_type":"Bearer",
///            "expires_in":3600})JSON")};
/// const sourcemeta::core::OAuthTokenResponse response{document};
/// assert(response.is_bearer_token_type());
/// assert(response.access_token().value() == "2YotnFZFEjr1zCsicMWpAA");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthTokenResponse {
public:
  /// Construct a view over a token response document, which is borrowed.
  explicit OAuthTokenResponse(const JSON &data);

  /// The issued access token (RFC 6749 Section 5.1), or no value when absent.
  [[nodiscard]] auto access_token() const -> std::optional<std::string_view>;

  /// The access token type (RFC 6749 Section 5.1), or no value when absent.
  [[nodiscard]] auto token_type() const -> std::optional<std::string_view>;

  /// Whether the token type is `Bearer`, matched case insensitively (RFC 6749
  /// Section 5.1).
  [[nodiscard]] auto is_bearer_token_type() const -> bool;

  /// The access token lifetime in seconds (RFC 6749 Section 5.1), or no value
  /// when absent or not a non-negative integer.
  [[nodiscard]] auto expires_in() const -> std::optional<std::chrono::seconds>;

  /// The issued refresh token (RFC 6749 Section 5.1), or no value when absent.
  [[nodiscard]] auto refresh_token() const -> std::optional<std::string_view>;

  /// The granted scope (RFC 6749 Section 5.1), or no value when absent.
  [[nodiscard]] auto scope() const -> std::optional<std::string_view>;

  /// Whether the granted scope contains a value, comparing the space-delimited
  /// unordered set (RFC 6749 Section 3.3).
  [[nodiscard]] auto has_scope(const std::string_view value) const -> bool;

  /// The underlying document, for reaching extension members such as an OpenID
  /// Connect `id_token`.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  const JSON *data_;
};

/// @ingroup oauth
/// The token a server grants, for building a token endpoint success response
/// (RFC 6749 Section 5.1). Every field borrows from the caller, an empty scalar
/// is omitted, and the two scope fields decide whether a scope is emitted.
struct OAuthTokenGrant {
  /// The issued access token (RFC 6749 Section 5.1).
  std::string_view access_token;
  /// The token type, such as `Bearer` (RFC 6749 Section 7.1).
  std::string_view token_type;
  /// The lifetime of the access token, omitted when absent (RFC 6749
  /// Section 5.1).
  std::optional<std::chrono::seconds> expires_in;
  /// The issued refresh token, omitted when absent (RFC 6749 Section 5.1).
  std::string_view refresh_token;
  /// The granted scope (RFC 6749 Section 3.3). A scope has at least one token,
  /// so an empty granted scope is not representable and is not emitted. A
  /// server that would grant no scopes denies the request instead of issuing a
  /// token with an empty scope.
  std::string_view scope;
  /// The scope the client requested, used to decide whether the granted scope
  /// must be emitted (RFC 6749 Section 5.1).
  std::string_view requested_scope;
};

/// @ingroup oauth
/// Build a token endpoint success response document (RFC 6749 Section 5.1). The
/// access token and token type are always emitted, the lifetime and refresh
/// token when present, and the scope only when it differs from the requested
/// scope, which is when it is REQUIRED (RFC 6749 Section 5.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::OAuthTokenGrant grant;
/// grant.access_token = "2YotnFZFEjr1zCsicMWpAA";
/// grant.token_type = "Bearer";
/// const auto response{sourcemeta::core::oauth_make_token_response(grant)};
/// assert(response.at("token_type").to_string() == "Bearer");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_make_token_response(const OAuthTokenGrant &grant) -> JSON;

/// @ingroup oauth
/// Build a token endpoint error response document (RFC 6749 Section 5.2). The
/// error code is always emitted, and the description and URI when present. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto response{sourcemeta::core::oauth_make_token_error_response(
///     "invalid_grant", "", "")};
/// assert(response.at("error").to_string() == "invalid_grant");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_make_token_error_response(const std::string_view error,
                                     const std::string_view error_description,
                                     const std::string_view error_uri) -> JSON;

} // namespace sourcemeta::core

#endif
