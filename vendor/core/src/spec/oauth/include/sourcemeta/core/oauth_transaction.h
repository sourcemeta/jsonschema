#ifndef SOURCEMETA_CORE_OAUTH_TRANSACTION_H_
#define SOURCEMETA_CORE_OAUTH_TRANSACTION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/oauth_authorization.h>

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The secrets minted for one authorization code flow, each the base64url
/// encoding of 32 random octets. Both are secret and are serialized by the
/// caller, such as
/// into a sealed cookie, and wiped once the flow completes. The 43 bytes of
/// each are not null terminated.
struct OAuthTransactionSecrets {
  /// The cross-site request forgery token (RFC 6749 Section 10.12).
  std::array<char, 43> state;
  /// The PKCE code verifier (RFC 7636 Section 4.1).
  std::array<char, 43> code_verifier;
};

/// @ingroup oauth
/// A non-owning view of the state a client retains between an authorization
/// request and its callback. Every field borrows from the caller and must
/// outlive any use of this struct.
struct OAuthTransaction {
  /// The state sent with the request, compared against the callback.
  std::string_view state;
  /// The PKCE code verifier, carried through to the token request.
  std::string_view code_verifier;
  /// The issuer identifier the request was sent to (RFC 9207 Section 2).
  std::string_view issuer;
  /// The redirection URI the request was sent with (RFC 8252 Section 8.10).
  std::string_view redirect_uri;
};

/// @ingroup oauth
/// Whether the authorization server is known to support the `iss` response
/// parameter (RFC 9207 Section 2.4). This governs how a callback missing `iss`
/// is treated.
enum class OAuthIssuerSupport : std::uint8_t {
  /// The server supports `iss`, so a callback missing it is rejected.
  Supported,
  /// The server is known not to support `iss`, so a present `iss` is ignored.
  NotSupported,
  /// Support is unknown, so `iss` is validated only when present.
  Unknown
};

/// @ingroup oauth
/// The reason a callback was rejected, in the order the checks run. The first
/// failing check decides the outcome (RFC 9700 Section 4).
enum class OAuthCallbackError : std::uint8_t {
  /// The state is missing or does not match, a possible cross-site request
  /// forgery (RFC 6749 Section 10.12).
  State,
  /// The callback arrived on a URI other than the one the request was sent with
  /// (RFC 8252 Section 8.10).
  ReceivedURI,
  /// The `iss` is missing when required or does not match the expected issuer,
  /// a possible mix-up attack (RFC 9207 Section 2.4).
  Issuer,
  /// The authorization server returned an error rather than a code (RFC 6749
  /// Section 4.1.2.1).
  Declined,
  /// The response carries neither an error nor a code.
  MissingCode
};

/// @ingroup oauth
/// Mint the secrets for a new authorization code flow, a random `state` and a
/// PKCE code verifier (RFC 7636 Section 4.1, RFC 6749 Section 10.12). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto secrets{sourcemeta::core::oauth_transaction_mint()};
/// assert(secrets.state.size() == 43);
/// assert(secrets.code_verifier.size() == 43);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_transaction_mint() -> OAuthTransactionSecrets;

/// @ingroup oauth
/// Validate an authorization response against the retained transaction,
/// returning the authorization code through `code` on success or the first
/// failing check otherwise (RFC 9700 Section 4). The checks run in order: the
/// state in constant time, the received URI when one is supplied, the issuer,
/// whether the server declined, and finally the presence of a code. Pass an
/// empty `received_uri` to skip that check. The state check always defends
/// against cross-site request forgery. Guaranteed mix-up defense needs a
/// non-empty `received_uri` (RFC 8252 Section 8.10) or an `issuer_support` of
/// `Supported`, which makes a missing `iss` fatal (RFC 9207 Section 2.4). With
/// `Unknown` the `iss` is validated only when present, so a server that omits
/// it is not caught, and with `NotSupported` it is ignored entirely. A caller
/// talking to more than one authorization server must therefore supply one of
/// those two. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// std::string storage;
/// sourcemeta::core::OAuthAuthorizationResponse response;
/// sourcemeta::core::oauth_parse_authorization_response(
///     "code=SplxlOBeZQQYbYS6WxSbIA&state=xyz", storage, response);
/// const sourcemeta::core::OAuthTransaction transaction{
///     .state = "xyz", .code_verifier = "", .issuer = "", .redirect_uri = ""};
/// std::string_view code;
/// const auto error{sourcemeta::core::oauth_transaction_check(
///     transaction, response, sourcemeta::core::OAuthIssuerSupport::Unknown,
///     "", code)};
/// assert(!error.has_value());
/// assert(code == "SplxlOBeZQQYbYS6WxSbIA");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_transaction_check(const OAuthTransaction &transaction,
                             const OAuthAuthorizationResponse &response,
                             const OAuthIssuerSupport issuer_support,
                             const std::string_view received_uri,
                             std::string_view &code)
    -> std::optional<OAuthCallbackError>;

} // namespace sourcemeta::core

#endif
