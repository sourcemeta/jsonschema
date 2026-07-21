#ifndef SOURCEMETA_CORE_OAUTH_CLIENT_AUTHENTICATION_H_
#define SOURCEMETA_CORE_OAUTH_CLIENT_AUTHENTICATION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>

#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// Append an HTTP `Basic` authentication credential (RFC 6749 Section 2.3.1) to
/// the sink, for use as an `Authorization` header value. The client identifier
/// and secret are each percent-encoded, joined with a colon, and Base64
/// encoded. The credential is secret, so the sink is a wiping string, and it is
/// appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString header;
/// sourcemeta::core::oauth_client_secret_basic("s6BhdRkqt3", "gX1fBat3bV",
///                                             header);
/// assert(header == "Basic czZCaGRSa3F0MzpnWDFmQmF0M2JW");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_client_secret_basic(const std::string_view client_id,
                               const std::string_view client_secret,
                               SecureString &sink) -> void;

/// @ingroup oauth
/// Append the `client_secret_post` client authentication parameters (RFC 6749
/// Section 2.3.1) to a token request body. Both the identifier and the secret
/// are emitted, so this composes into the same sink as a grant builder. The
/// body carries a secret, so the sink is a wiping string, and it is appended to
/// and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_client_secret_post("id", "secret", body);
/// assert(body == "client_id=id&client_secret=secret");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_client_secret_post(const std::string_view client_id,
                              const std::string_view client_secret,
                              SecureString &sink) -> void;

/// @ingroup oauth
/// Append a public client identification parameter (RFC 6749 Section 3.2.1) to
/// a token request body, emitting the identifier alone with no secret. This
/// composes into the same sink as a grant builder. The sink is appended to and
/// never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_client_id_only("s6BhdRkqt3", body);
/// assert(body == "client_id=s6BhdRkqt3");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_client_id_only(const std::string_view client_id, SecureString &sink)
    -> void;

/// @ingroup oauth
/// The client authentication mechanism a token request presented (RFC 6749
/// Section 2.3). `Public` is a bare client identifier with no secret, which is
/// identification rather than an authentication mechanism (RFC 6749
/// Section 3.2.1).
enum class OAuthClientAuthenticationMethod : std::uint8_t {
  /// No client authentication and no identifier were presented.
  None,
  /// An HTTP `Basic` credential (RFC 6749 Section 2.3.1).
  Basic,
  /// A `client_secret` in the request body (RFC 6749 Section 2.3.1).
  Post,
  /// A bare `client_id` in the request body, identifying a public client
  /// (RFC 6749 Section 3.2.1).
  Public,
  /// A `client_assertion` and its type (RFC 7521 Section 4.2).
  Assertion
};

/// @ingroup oauth
/// The client credentials a token request presented, each a non-owning view
/// into the request or the decode arena (RFC 6749 Section 2.3). An absent field
/// is an empty view.
struct OAuthClientCredentials {
  /// The mechanism the request presented.
  OAuthClientAuthenticationMethod method;
  /// The client identifier.
  std::string_view client_id;
  /// The client secret, present only for `Basic` and `Post`.
  std::string_view client_secret;
  /// The assertion type, present only for `Assertion` (RFC 7521 Section 4.2).
  std::string_view assertion_type;
  /// The assertion, present only for `Assertion` (RFC 7521 Section 4.2).
  std::string_view assertion;
};

/// @ingroup oauth
/// Parse the client authentication a token request presented, from the
/// `Authorization` header value and the request body, into the credentials
/// (RFC 6749 Section 2.3). Returns whether the presentation is well formed. It
/// is malformed when more than one mechanism is presented (RFC 6749
/// Section 2.3, RFC 7521 Section 4.2.1), when a `Basic` username conflicts with
/// a body `client_id` (RFC 6749 Section 5.2), when the `Basic` credential is
/// not a canonical Base64 of a colon-separated pair, or when only one of the
/// assertion parameters is present. The caller chooses the error code for a
/// rejection, which is `invalid_client` when a collision involves the assertion
/// mechanism (RFC 7521 Section 4.2.1) and `invalid_request` otherwise (RFC 6749
/// Section 5.2). The decoded `Basic` credential is secret, so the arena is a
/// wiping string, which the caller owns, should clear between independent
/// parses, and must not alias the header or body inputs. This detects the
/// mechanism and extracts the credentials, but does
/// not verify the secret, which the caller does against its stored value in
/// constant time, nor that an `Assertion` `client_id` identifies the same
/// client as the assertion (RFC 7521 Section 4.2), which the caller checks when
/// it verifies the assertion. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString storage;
/// sourcemeta::core::OAuthClientCredentials credentials;
/// assert(sourcemeta::core::oauth_parse_client_authentication(
///     "Basic czZCaGRSa3F0MzpnWDFmQmF0M2JW", "", storage, credentials));
/// assert(credentials.method ==
///        sourcemeta::core::OAuthClientAuthenticationMethod::Basic);
/// assert(credentials.client_id == "s6BhdRkqt3");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_parse_client_authentication(const std::string_view authorization,
                                       const std::string_view body,
                                       SecureString &storage,
                                       OAuthClientCredentials &credentials)
    -> bool;

} // namespace sourcemeta::core

#endif
