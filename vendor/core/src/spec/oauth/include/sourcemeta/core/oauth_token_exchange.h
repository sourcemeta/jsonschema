#ifndef SOURCEMETA_CORE_OAUTH_TOKEN_EXCHANGE_H_
#define SOURCEMETA_CORE_OAUTH_TOKEN_EXCHANGE_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The `access_token` token type identifier (RFC 8693 Section 3).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_ACCESS_TOKEN{
    "urn:ietf:params:oauth:token-type:access_token"};
/// @ingroup oauth
/// The `refresh_token` token type identifier (RFC 8693 Section 3).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_REFRESH_TOKEN{
    "urn:ietf:params:oauth:token-type:refresh_token"};
/// @ingroup oauth
/// The `id_token` token type identifier (RFC 8693 Section 3).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_ID_TOKEN{
    "urn:ietf:params:oauth:token-type:id_token"};
/// @ingroup oauth
/// The SAML 1.1 token type identifier (RFC 8693 Section 3).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_SAML1{
    "urn:ietf:params:oauth:token-type:saml1"};
/// @ingroup oauth
/// The SAML 2.0 token type identifier (RFC 8693 Section 3).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_SAML2{
    "urn:ietf:params:oauth:token-type:saml2"};
/// @ingroup oauth
/// The JWT token type identifier (RFC 8693 Section 3).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_JWT{
    "urn:ietf:params:oauth:token-type:jwt"};

/// @ingroup oauth
/// The parameters of a token exchange request (RFC 8693 Section 2.1). Every
/// field is a non-owning view, an empty scalar is omitted, and the spans carry
/// the repeatable audience and resource indicator values, each emitted under
/// its fixed parameter name.
struct OAuthTokenExchangeRequest {
  /// The security token that represents the subject, REQUIRED (RFC 8693
  /// Section 2.1).
  std::string_view subject_token;
  /// The type of the subject token, REQUIRED (RFC 8693 Section 2.1).
  std::string_view subject_token_type;
  /// The security token that represents the acting party, emitted only
  /// together with its type (RFC 8693 Section 2.1).
  std::string_view actor_token;
  /// The type of the actor token, emitted only together with the actor token.
  std::string_view actor_token_type;
  /// The requested type for the issued token (RFC 8693 Section 2.1).
  std::string_view requested_token_type;
  /// The space-delimited requested scope (RFC 6749 Section 3.3).
  std::string_view scope;
  /// The repeatable logical audience names of the target service, each emitted
  /// as an `audience` parameter (RFC 8693 Section 2.1).
  std::span<const std::string_view> audiences;
  /// The repeatable resource indicators, each an absolute URI without a
  /// fragment emitted as a `resource` parameter (RFC 8707 Section 2).
  std::span<const std::string_view> resources;
};

/// @ingroup oauth
/// Whether a token exchange request is well formed (RFC 8693 Section 2.1): the
/// subject token and its type are present, and the actor token and its type are
/// either both present or both absent. A malformed request is rejected as
/// `invalid_request` (RFC 8693 Section 2.2.2), so a server applies this after
/// collecting the parameters. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::OAuthTokenExchangeRequest request;
/// request.subject_token = "eyJ...";
/// request.subject_token_type =
///     sourcemeta::core::OAUTH_TOKEN_TYPE_ACCESS_TOKEN;
/// assert(sourcemeta::core::oauth_token_exchange_valid(request));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_token_exchange_valid(const OAuthTokenExchangeRequest &request)
    -> bool;

/// @ingroup oauth
/// Append a token exchange request body (RFC 8693 Section 2.1) to the sink,
/// returning whether it is well formed. The `grant_type` is the token exchange
/// URN, the subject token and its type are required, the actor token and its
/// type are emitted only together, and the repeatable audiences and resources
/// follow. No `client_id` is emitted, so the caller composes a client
/// authentication builder into the same sink. The body carries a secret token,
/// so the sink is a wiping string, and it is appended to and never cleared. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::OAuthTokenExchangeRequest request;
/// request.subject_token = "accVkjcJyb4BWCxGsndESCJQbdFMogUC5PbRDqceLTC";
/// request.subject_token_type =
///     sourcemeta::core::OAUTH_TOKEN_TYPE_ACCESS_TOKEN;
/// sourcemeta::core::SecureString body;
/// assert(sourcemeta::core::oauth_build_token_request_exchange(request, body));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_token_request_exchange(
    const OAuthTokenExchangeRequest &request, SecureString &sink) -> bool;

/// @ingroup oauth
/// The type of the token a token exchange response issued (RFC 8693
/// Section 2.2.1), or no value when absent. The result borrows from the
/// response, which must outlive it. It is REQUIRED on a successful response, so
/// its absence marks a malformed one. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"JSON({
///   "access_token": "eyJ...", "token_type": "Bearer",
///   "issued_token_type": "urn:ietf:params:oauth:token-type:access_token"
/// })JSON")};
/// assert(sourcemeta::core::oauth_issued_token_type(document).value() ==
///        sourcemeta::core::OAUTH_TOKEN_TYPE_ACCESS_TOKEN);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_issued_token_type(const JSON &response)
    -> std::optional<std::string_view>;

} // namespace sourcemeta::core

#endif
