#ifndef SOURCEMETA_CORE_OAUTH_PAR_H_
#define SOURCEMETA_CORE_OAUTH_PAR_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_authorization.h>

#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <functional>  // std::function
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// Append a pushed authorization request body (RFC 9126 Section 2.1) to the
/// sink. Every authorization request parameter is emitted except the
/// `request_uri`, which a pushed request MUST NOT carry, and the `client_id`,
/// which client authentication supplies, in the body for a public client or the
/// `client_secret_post` method and in the `Authorization` header for the
/// `client_secret_basic` method. The `response_type` defaults to the
/// authorization code flow. The body may carry a client secret, so the sink is
/// a wiping string, appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::OAuthAuthorizationRequest request;
/// request.redirect_uri = "https://client.example/cb";
/// request.scope = "read";
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_par_request(request, body);
/// sourcemeta::core::oauth_client_id_only("s6BhdRkqt3", body);
/// assert(std::string_view{body}.starts_with("response_type=code"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_par_request(const OAuthAuthorizationRequest &request,
                             SecureString &sink) -> void;

/// @ingroup oauth
/// Append the front-channel authorization request URL (RFC 9126 Section 4) to
/// the sink, the reference form carrying only the `client_id` and the
/// `request_uri` obtained from the pushed authorization request endpoint.
/// Unlike a full authorization request it emits no `response_type`, since the
/// pushed request holds it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <string>
/// #include <cassert>
///
/// std::string url;
/// sourcemeta::core::oauth_build_par_authorization_url(
///     "https://server.example/authorize", "s6BhdRkqt3",
///     "urn:ietf:params:oauth:request_uri:6esc", url);
/// assert(url ==
///        "https://server.example/authorize?client_id=s6BhdRkqt3&request_uri="
///        "urn%3Aietf%3Aparams%3Aoauth%3Arequest_uri%3A6esc");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_par_authorization_url(const std::string_view endpoint,
                                       const std::string_view client_id,
                                       const std::string_view request_uri,
                                       std::string &sink) -> void;

/// @ingroup oauth
/// A non-owning view over a pushed authorization request response (RFC 9126
/// Section 2.2) held in a caller-owned JSON value, which must outlive the view.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"JSON({
///   "request_uri": "urn:ietf:params:oauth:request_uri:6esc",
///   "expires_in": 60
/// })JSON")};
/// const sourcemeta::core::OAuthPARResponse response{document};
/// assert(response.request_uri().value() ==
///        "urn:ietf:params:oauth:request_uri:6esc");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthPARResponse {
public:
  /// Construct a view over a pushed authorization request response, which is
  /// borrowed.
  explicit OAuthPARResponse(const JSON &data);

  /// The request URI to use at the authorization endpoint (RFC 9126
  /// Section 2.2).
  [[nodiscard]] auto request_uri() const -> std::optional<std::string_view>;
  /// The lifetime of the request URI, no value when non-positive (RFC 9126
  /// Section 2.2).
  [[nodiscard]] auto expires_in() const -> std::optional<std::chrono::seconds>;

  /// The underlying document.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  const JSON *data_;
};

/// @ingroup oauth
/// Parse a pushed authorization request body (RFC 9126 Section 2.1) into the
/// result, returning whether it is well formed. The parse follows the
/// authorization request rules, and additionally rejects a `request_uri`
/// parameter, which a pushed request MUST NOT provide, and every other
/// parameter, such as the client authentication ones, is passed to the
/// callback. The body may carry a client secret, so it is decoded into a wiping
/// arena the caller owns and reuses. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString storage;
/// sourcemeta::core::OAuthAuthorizationRequest request;
/// assert(sourcemeta::core::oauth_parse_par_request(
///     "response_type=code&client_id=s6BhdRkqt3", storage, request,
///     [](std::string_view, std::string_view) {}));
/// assert(request.response_type == "code");
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_parse_par_request(
    const std::string_view body, SecureString &storage,
    OAuthAuthorizationRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool;

/// @ingroup oauth
/// Mint a request URI reference for a pushed authorization request response
/// (RFC 9126 Section 2.2), the `urn:ietf:params:oauth:request_uri` form with a
/// cryptographically strong random reference so a value cannot be guessed
/// (RFC 9126 Section 7.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto request_uri{sourcemeta::core::oauth_par_request_uri()};
/// assert(request_uri.starts_with("urn:ietf:params:oauth:request_uri:"));
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_par_request_uri()
    -> std::string;

/// @ingroup oauth
/// Build a pushed authorization request response document (RFC 9126
/// Section 2.2), returning no value when the request URI is empty or the
/// lifetime is non-positive, which is a REQUIRED positive value. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <chrono>
/// #include <cassert>
///
/// const auto response{sourcemeta::core::oauth_make_par_response(
///     "urn:ietf:params:oauth:request_uri:6esc", std::chrono::seconds{60})};
/// assert(response.value().at("expires_in").to_integer() == 60);
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_make_par_response(const std::string_view request_uri,
                        const std::chrono::seconds expires_in)
    -> std::optional<JSON>;

/// @ingroup oauth
/// Reconcile the two ways a DPoP key is communicated at the pushed
/// authorization request endpoint (RFC 9449 Section 10.1): the `dpop_jkt`
/// parameter and the thumbprint of a verified DPoP header proof. Returns the
/// effective thumbprint to bind the authorization code to, an empty view when
/// neither is present, or no value when both are present and disagree, which
/// the server MUST reject. The returned view borrows from the arguments. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_par_dpop_binding("abc", "abc").value() ==
///        "abc");
/// assert(!sourcemeta::core::oauth_par_dpop_binding("abc", "xyz").has_value());
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_par_dpop_binding(const std::string_view dpop_jkt,
                       const std::optional<std::string_view> proof_thumbprint)
    -> std::optional<std::string_view>;

} // namespace sourcemeta::core

#endif
