#ifndef SOURCEMETA_CORE_OAUTH_BEARER_H_
#define SOURCEMETA_CORE_OAUTH_BEARER_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// Append a `Bearer` credential (RFC 6750 Section 2.1) for an access token to
/// the sink, returning whether the token is a well-formed `b64token`. Nothing
/// is appended when it is not. The sink then holds the access token, which is
/// secret, so a caller that keeps it should use wiping storage. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// std::string header;
/// assert(sourcemeta::core::oauth_bearer_header("mF_9.B5f-4.1JqM", header));
/// assert(header == "Bearer mF_9.B5f-4.1JqM");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_bearer_header(const std::string_view token, std::string &sink)
    -> bool;

/// @ingroup oauth
/// Find the value of one authentication parameter within the challenge for a
/// scheme in a `WWW-Authenticate` header value (RFC 7235 Section 4.1),
/// returning no value when the scheme or parameter is absent, or the header is
/// malformed before the parameter is reached. The scheme and parameter name are
/// matched case insensitively, and a quoted-string value is unescaped (RFC 9110
/// Section 5.6.4). The full grammar is parsed so that adjacent challenges and
/// parameters are told apart correctly. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto realm{sourcemeta::core::oauth_challenge_parameter(
///     R"(Bearer realm="example", error="invalid_token")", "Bearer", "realm")};
/// assert(realm.has_value());
/// assert(realm.value() == "example");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_challenge_parameter(const std::string_view header,
                               const std::string_view scheme,
                               const std::string_view name)
    -> std::optional<std::string>;

/// @ingroup oauth
/// The parameters of a `WWW-Authenticate` challenge a protected resource
/// returns (RFC 6750 Section 3, RFC 9728 Section 5.1, RFC 9449 Section 7.1).
/// Every field is a non-owning view that must outlive any use of this struct,
/// and an empty field is omitted from the challenge.
struct OAuthChallenge {
  /// The protection space the credentials apply to (RFC 6750 Section 3).
  std::string_view realm;
  /// The space-delimited scope the token must carry (RFC 6750 Section 3).
  std::string_view scope;
  /// The error code (RFC 6750 Section 3.1).
  std::string_view error;
  /// The human-readable error description (RFC 6750 Section 3).
  std::string_view error_description;
  /// The URI of a human-readable error page (RFC 6750 Section 3).
  std::string_view error_uri;
  /// The URL of the protected resource metadata document (RFC 9728
  /// Section 5.1).
  std::string_view resource_metadata;
  /// The space-delimited JWS algorithms the DPoP scheme accepts (RFC 9449
  /// Section 7.1).
  std::string_view algs;
};

/// @ingroup oauth
/// Build a `WWW-Authenticate` challenge for a scheme and append it to the sink,
/// returning whether a challenge was produced (RFC 7235 Section 4.1). Each
/// present parameter is emitted as a quoted-string with the double quote and
/// backslash escaped (RFC 9110 Section 5.6.4). A challenge with no parameter
/// yields a bare scheme, which RFC 7235 Section 2.1 permits and a DPoP
/// challenge may use (RFC 9449 Section 7.1), so the `Bearer` scheme's own rule
/// that it carry at least one parameter (RFC 6750 Section 3) is the caller's
/// responsibility. Nothing is appended and false is returned only when the
/// scheme is not a token or a value carries a control character that would
/// allow header injection. Only header safety is enforced, so keeping each
/// value within the tighter character set its own attribute defines (RFC 6750
/// Section 3) is likewise the caller's responsibility. It is a pure function,
/// so a resource server builds its fixed challenge once and caches it. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OAuthChallenge challenge;
/// challenge.realm = "example";
/// challenge.error = "invalid_token";
/// std::string header;
/// sourcemeta::core::oauth_build_challenge("Bearer", challenge, header);
/// assert(header == R"(Bearer realm="example", error="invalid_token")");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_challenge(const std::string_view scheme,
                           const OAuthChallenge &challenge, std::string &sink)
    -> bool;

/// @ingroup oauth
/// Whether a set of access token claims names an audience, so that a resource
/// server accepts only a token minted for it (RFC 9068 Section 4, RFC 7662
/// Section 2.2). The claims are the payload of a JWT access token or an
/// introspection response, and the `aud` claim is honored whether it is a
/// single string or an array of strings. Each value is treated as an opaque
/// case-sensitive string (RFC 7519 Section 4.1.3) and compared by code points
/// (RFC 3986 Section 6.2.1), with no normalization. An empty audience never
/// matches. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto claims{
///     sourcemeta::core::parse_json(R"JSON({"aud":"https://api.example"})JSON")};
/// assert(sourcemeta::core::oauth_has_audience(claims, "https://api.example"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_has_audience(const JSON &claims, const std::string_view audience)
    -> bool;

/// @ingroup oauth
/// Whether a set of access token claims carries the DPoP confirmation that
/// binds the token to a proof-of-possession key, namely a `jkt` JWK thumbprint
/// (RFC 9449 Section 6.1). A resource server that receives such a token under
/// the `Bearer` scheme must reject it, since a key-bound token stripped of its
/// proof is being replayed (RFC 9449 Section 7.2). Only the DPoP confirmation
/// is detected, not other sender-constraining methods such as mutual-TLS. The
/// claims are the payload of a JWT access token or an introspection response.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto claims{sourcemeta::core::parse_json(
///     R"JSON({"cnf":{"jkt":"0ZcOCORZNYy-DWpqq30jZyJGHTN0d2HglBV3uiguA4I"}})JSON")};
/// assert(sourcemeta::core::oauth_is_dpop_bound(claims));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_is_dpop_bound(const JSON &claims) -> bool;

} // namespace sourcemeta::core

#endif
