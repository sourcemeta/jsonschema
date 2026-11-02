#ifndef SOURCEMETA_CORE_OAUTH_AUTHORIZATION_H_
#define SOURCEMETA_CORE_OAUTH_AUTHORIZATION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/oauth_profile.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// A single query or form parameter as a name and value pair. Both members are
/// non-owning views that must outlive any use of this parameter. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
///
/// const sourcemeta::core::OAuthParameter parameter{"resource",
///                                                  "https://api.example"};
/// ```
struct OAuthParameter {
  /// The parameter name.
  std::string_view name;
  /// The parameter value, still in its raw unescaped form.
  std::string_view value;
};

/// @ingroup oauth
/// A non-owning view of the parameters of an authorization request (RFC 6749
/// Section 4.1.1). Every field borrows from the caller and must outlive any use
/// of this struct. An empty scalar field is treated as absent and is not
/// emitted, and the spans carry the repeatable and extension parameters. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationRequest request;
/// request.client_id = "s6BhdRkqt3";
/// request.state = "xyz";
/// std::string url;
/// sourcemeta::core::oauth_build_authorization_url(
///     "https://server.example/authorize", request, url);
/// assert(url ==
///        "https://server.example/authorize?response_type=code"
///        "&client_id=s6BhdRkqt3&state=xyz");
/// ```
struct OAuthAuthorizationRequest {
  /// The client identifier (RFC 6749 Section 2.2).
  std::string_view client_id;
  /// The redirection endpoint the response is returned to (RFC 6749
  /// Section 3.1.2).
  std::string_view redirect_uri;
  /// The space-delimited requested scope (RFC 6749 Section 3.3).
  std::string_view scope;
  /// The opaque cross-site request forgery token echoed back on the response
  /// (RFC 6749 Section 10.12).
  std::string_view state;
  /// The PKCE code challenge (RFC 7636 Section 4.3).
  std::string_view code_challenge;
  /// The PKCE code challenge method, emitted only alongside a challenge
  /// (RFC 7636 Section 4.3).
  std::string_view code_challenge_method;
  /// The reference to a pushed authorization request (RFC 9126 Section 4).
  std::string_view request_uri;
  /// The JWK thumbprint of the DPoP proof public key (RFC 9449 Section 10).
  std::string_view dpop_jkt;
  /// The response type, a space-delimited set that defaults to `code` when
  /// unset and is otherwise honored by the authorization URL and pushed request
  /// builders alike, surfaced on parse so a server can tell a missing value
  /// from one it does not understand (RFC 6749 Section 3.1.1). It is placed
  /// after the older members so an existing positional initializer keeps
  /// populating them.
  std::string_view response_type;
  /// The repeatable resource indicators, each an absolute URI without a
  /// fragment (RFC 8707 Section 2).
  std::span<const OAuthParameter> resources;
  /// The extension parameters, such as an OpenID Connect `nonce`, emitted
  /// verbatim after the known parameters.
  std::span<const OAuthParameter> extra;
};

/// @ingroup oauth
/// Build an authorization request URL by appending the query to the endpoint
/// (RFC 6749 Section 4.1.1). The `response_type` defaults to `code` and is
/// honored when set, every value is percent-escaped, an existing query on the
/// endpoint is honored, and the code challenge method is emitted only when a
/// challenge is present. The sink is appended to and never cleared. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationRequest request;
/// request.client_id = "s6BhdRkqt3";
/// request.redirect_uri = "https://client.example/cb";
/// std::string url;
/// sourcemeta::core::oauth_build_authorization_url(
///     "https://server.example/authorize", request, url);
/// assert(url ==
///        "https://server.example/authorize?response_type=code"
///        "&client_id=s6BhdRkqt3&redirect_uri=https%3A%2F%2Fclient.example%2Fcb");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_url(const std::string_view endpoint,
                                   const OAuthAuthorizationRequest &request,
                                   std::string &sink) -> void;

/// @ingroup oauth
/// Parse the query of an authorization request at the authorization server
/// (RFC 6749 Section 4.1.1) into the result, returning whether it is well
/// formed. Each recognized value is form-decoded, borrowing from the input when
/// it carries no escape and otherwise from the storage arena, which the caller
/// owns and reuses across parses. A duplicated parameter is a failure (RFC 6749
/// Section 3.1), and `code_challenge_method` defaults to `plain` when a
/// challenge is present without one (RFC 7636 Section 4.3). Every repeatable
/// `resource` and every unrecognized parameter is passed to the callback with
/// its decoded value rather than stored on the result, so the caller collects
/// them. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// std::string storage;
/// sourcemeta::core::OAuthAuthorizationRequest request;
/// assert(sourcemeta::core::oauth_parse_authorization_request(
///     "response_type=code&client_id=s6BhdRkqt3", storage, request,
///     [](std::string_view, std::string_view) {}));
/// assert(request.response_type == "code");
/// assert(request.client_id == "s6BhdRkqt3");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_parse_authorization_request(
    const std::string_view query, std::string &storage,
    OAuthAuthorizationRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool;

/// @ingroup oauth
/// Whether a redirection URI a client presents matches one it registered
/// (RFC 6749 Section 3.1.2.3), by an exact byte comparison with one exception.
/// When the registered URI is a loopback redirect, an `http` URI whose host is
/// literally `127.0.0.1` or `[::1]`, only the port may differ (RFC 8252
/// Section 7.3). The strict profile keeps the OAuth 2.1 rule that `localhost`
/// is not a loopback host (RFC 8252 Section 8.3). No URI is constructed, so the
/// inputs are compared as given. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_redirect_uri_matches(
///     "http://127.0.0.1:49152/cb", "http://127.0.0.1:51004/cb",
///     sourcemeta::core::OAuthProfile::Strict));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_redirect_uri_matches(const std::string_view registered,
                                const std::string_view presented,
                                const OAuthProfile profile) -> bool;

/// @ingroup oauth
/// Whether a URI scheme is a private-use scheme suitable for a native app
/// redirect, a reverse-domain name that must contain at least one period
/// (RFC 8252 Section 7.1 and Section 8.4). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_is_private_use_scheme("com.example.app"));
/// assert(!sourcemeta::core::oauth_is_private_use_scheme("myapp"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_is_private_use_scheme(const std::string_view scheme) noexcept
    -> bool;

/// @ingroup oauth
/// A non-owning view of an authorization response returned on the redirection
/// endpoint (RFC 6749 Section 4.1.2). Each field borrows from the parsed query
/// or the decode arena, so both must outlive the view, and an absent parameter
/// is an empty view. A present `error` marks a failure response (RFC 6749
/// Section 4.1.2.1).
struct OAuthAuthorizationResponse {
  /// The authorization code (RFC 6749 Section 4.1.2).
  std::string_view code;
  /// The state value echoed from the request (RFC 6749 Section 4.1.2).
  std::string_view state;
  /// The issuer identifier of the authorization server (RFC 9207 Section 2).
  std::string_view iss;
  /// The error code of a failure response (RFC 6749 Section 4.1.2.1).
  std::string_view error;
  /// The human-readable error description of a failure response (RFC 6749
  /// Section 4.1.2.1).
  std::string_view error_description;
  /// The URI of a human-readable error page for a failure response (RFC 6749
  /// Section 4.1.2.1).
  std::string_view error_uri;
};

/// @ingroup oauth
/// Parse the query of an authorization response (RFC 6749 Section 4.1.2) into
/// the result, returning whether the query is well formed. Each recognized
/// value is form-decoded (RFC 6749 Appendix B), borrowing from the input when
/// it carries no escape and otherwise from the storage arena, which the caller
/// owns and which is reused across parses. A duplicated recognized parameter is
/// a failure, and an unrecognized parameter is ignored. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// std::string storage;
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// assert(sourcemeta::core::oauth_parse_authorization_response(
///     "code=SplxlOBeZQQYbYS6WxSbIA&state=xyz", storage, response));
/// assert(response.code == "SplxlOBeZQQYbYS6WxSbIA");
/// assert(response.state == "xyz");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_parse_authorization_response(const std::string_view query,
                                        std::string &storage,
                                        OAuthAuthorizationResponse &result)
    -> bool;

/// @ingroup oauth
/// Build a successful authorization redirect at the authorization server by
/// appending the query to the client's redirection endpoint (RFC 6749
/// Section 4.1.2), returning whether it was produced. The `code` is required,
/// `state` is echoed when present, and an `iss` is emitted when present and
/// must be a valid issuer identifier (RFC 9207 Section 2). No value is produced
/// when the redirect URI contains a fragment, which a redirection endpoint must
/// not (RFC 6749 Section 3.1.2). Every value is percent-escaped, an existing
/// query on the endpoint is honored, and the sink is appended to and never
/// cleared. The redirect URI and the response fields must not alias the sink.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// response.code = "SplxlOBeZQQYbYS6WxSbIA";
/// response.state = "xyz";
/// std::string url;
/// assert(sourcemeta::core::oauth_build_authorization_redirect(
///     "https://client.example/cb", response, url));
/// assert(url == "https://client.example/cb?code=SplxlOBeZQQYbYS6WxSbIA"
///               "&state=xyz");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink) -> bool;

/// @ingroup oauth
/// Build a failed authorization redirect at the authorization server (RFC 6749
/// Section 4.1.2.1), returning whether it was produced. The `error` is
/// required, `error_description`, `error_uri`, and `state` are emitted when
/// present, and an `iss` is emitted when present and must be a valid issuer
/// identifier (RFC 9207 Section 2). This must not be called when the redirect
/// URI or client identifier failed validation, since the error is then shown to
/// the resource owner rather than redirected. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// response.state = "xyz";
/// response.error = "access_denied";
/// std::string url;
/// assert(sourcemeta::core::oauth_build_authorization_error_redirect(
///     "https://client.example/cb", response, url));
/// assert(url ==
///        "https://client.example/cb?error=access_denied&state=xyz");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_error_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink) -> bool;

/// @ingroup oauth
/// The mechanism used for returning authorization response parameters from
/// the authorization endpoint (OAuth 2.0 Multiple Response Types Section 2.1,
/// OAuth 2.0 Form Post Response Mode Section 2).
enum class OAuthResponseMode : std::uint8_t {
  /// Parameters are "encoded in the query string added to the redirect_uri
  /// when redirecting back to the Client" (OAuth 2.0 Multiple Response Types
  /// Section 2.1).
  Query,
  /// Parameters are "encoded in the fragment added to the redirect_uri when
  /// redirecting back to the Client" (OAuth 2.0 Multiple Response Types
  /// Section 2.1).
  Fragment,
  /// Parameters are "encoded as HTML form values that are auto-submitted in
  /// the User Agent" with the HTTP POST method (OAuth 2.0 Form Post Response
  /// Mode Section 2).
  FormPost
};

/// @ingroup oauth
/// Look up the default response mode of a response type (OAuth 2.0 Multiple
/// Response Types Sections 2.1, 3, 4, and 5), returning no value unless the
/// response type is registered. The response type "is compared as a
/// space-delimited list of values in which the order of values does not
/// matter" (Section 1.2). The lookup covers every registered response type,
/// including token-bearing ones whose successful response this module does
/// not encode, since requests are still built and response modes still
/// negotiated for them. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto mode{sourcemeta::core::oauth_default_response_mode("code")};
/// assert(mode.has_value());
/// assert(mode.value() == sourcemeta::core::OAuthResponseMode::Query);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_default_response_mode(const std::string_view response_type)
    -> std::optional<OAuthResponseMode>;

/// @ingroup oauth
/// Check whether a response mode may encode the response of a response type,
/// as "in no case should a set of Authorization Response parameters whose
/// default Response Mode is the fragment encoding be encoded using the query
/// encoding" (OAuth 2.0 Multiple Response Types Section 7). An unknown
/// response type allows no mode. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(!sourcemeta::core::oauth_is_response_mode_allowed(
///     "id_token", sourcemeta::core::OAuthResponseMode::Query));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_is_response_mode_allowed(const std::string_view response_type,
                                    const OAuthResponseMode mode) -> bool;

/// @ingroup oauth
/// Build a successful authorization redirect at the authorization server in
/// the given response mode, encoding the parameters in the query (RFC 6749
/// Section 4.1.2) or in the fragment (OAuth 2.0 Multiple Response Types
/// Section 2.1) of the client's redirection endpoint, returning whether it was
/// produced. The form post mode produces no value, as it emits an HTML page
/// rather than a redirect. Every other behavior matches the query-only
/// builder. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// response.code = "SplxlOBeZQQYbYS6WxSbIA";
/// std::string url;
/// assert(sourcemeta::core::oauth_build_authorization_redirect(
///     "https://client.example/cb", response,
///     sourcemeta::core::OAuthResponseMode::Fragment, url));
/// assert(url == "https://client.example/cb#code=SplxlOBeZQQYbYS6WxSbIA");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, const OAuthResponseMode mode,
    std::string &sink) -> bool;

/// @ingroup oauth
/// Build a failed authorization redirect at the authorization server in the
/// given response mode (RFC 6749 Section 4.1.2.1, OAuth 2.0 Multiple Response
/// Types Section 2.1), returning whether it was produced. The form post mode
/// produces no value, as it emits an HTML page rather than a redirect. Every
/// other behavior matches the query-only builder. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// response.error = "access_denied";
/// std::string url;
/// assert(sourcemeta::core::oauth_build_authorization_error_redirect(
///     "https://client.example/cb", response,
///     sourcemeta::core::OAuthResponseMode::Fragment, url));
/// assert(url == "https://client.example/cb#error=access_denied");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_error_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, const OAuthResponseMode mode,
    std::string &sink) -> bool;

/// @ingroup oauth
/// Build the auto-submitting HTML page of a successful authorization response
/// in the form post response mode (OAuth 2.0 Form Post Response Mode
/// Section 2), returning whether it was produced. The action of the form is
/// the client's redirection endpoint, the response parameters are the hidden
/// form values, and the page carries the given title, with the same validation
/// as the redirect builder. The redirect URI, the response fields, and the
/// title must not alias the sink. When serving the page, the authorization
/// server "MUST instruct the User Agent (and any intermediaries) not to store
/// or reuse the content of the response". For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// response.code = "SplxlOBeZQQYbYS6WxSbIA";
/// std::string page;
/// assert(sourcemeta::core::oauth_build_authorization_form_post(
///     "https://client.example/cb", response, page));
/// assert(page.find("<form method=\"post\"") != std::string::npos);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_form_post(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink,
    const std::string_view title = "Submit This Form") -> bool;

/// @ingroup oauth
/// Build the auto-submitting HTML page of a failed authorization response in
/// the form post response mode (OAuth 2.0 Form Post Response Mode Section 2,
/// RFC 6749 Section 4.1.2.1), returning whether it was produced, with the same
/// validation as the error redirect builder and the page carrying the given
/// title. The redirect URI, the response fields, and the title must not alias
/// the sink. When serving the page, the authorization server "MUST instruct
/// the User Agent (and any intermediaries) not to store or reuse the content
/// of the response". For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// response.error = "access_denied";
/// std::string page;
/// assert(sourcemeta::core::oauth_build_authorization_error_form_post(
///     "https://client.example/cb", response, page));
/// assert(page.find("access_denied") != std::string::npos);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_authorization_error_form_post(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink,
    const std::string_view title = "Submit This Form") -> bool;

} // namespace sourcemeta::core

#endif
