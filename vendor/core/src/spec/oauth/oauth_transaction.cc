#include <sourcemeta/core/oauth_transaction.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/oauth_pkce.h>
#include <sourcemeta/core/oauth_random.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto oauth_transaction_mint() -> OAuthTransactionSecrets {
  return {.state = oauth_random_token(),
          .code_verifier = oauth_pkce_verifier()};
}

auto oauth_transaction_check(const OAuthTransaction &transaction,
                             const OAuthAuthorizationResponse &response,
                             const OAuthIssuerSupport issuer_support,
                             const std::string_view received_uri,
                             std::string_view &code)
    -> std::optional<OAuthCallbackError> {
  // RFC 6749 Section 10.12: the state binds the callback to this user agent, so
  // it is checked first and in constant time to avoid leaking a match position,
  // even though the value itself is not confidential
  if (response.state.empty() ||
      !secure_equals(response.state, transaction.state)) {
    return OAuthCallbackError::State;
  }

  // RFC 8252 Section 8.10: when the caller knows the URI the response arrived
  // on, it must be the one the request was sent with
  if (!received_uri.empty() && received_uri != transaction.redirect_uri) {
    return OAuthCallbackError::ReceivedURI;
  }

  // RFC 9207 Section 2.4: the issuer defends against a mix-up attack
  switch (issuer_support) {
    case OAuthIssuerSupport::Supported:
      if (response.iss.empty() || response.iss != transaction.issuer) {
        return OAuthCallbackError::Issuer;
      }

      break;
    case OAuthIssuerSupport::Unknown:
      if (!response.iss.empty() && response.iss != transaction.issuer) {
        return OAuthCallbackError::Issuer;
      }

      break;
    case OAuthIssuerSupport::NotSupported:
      break;
  }

  // RFC 6749 Section 4.1.2.1: an authenticated error response is a decline, and
  // it is distinguished from a missing code only after the checks above pass
  if (!response.error.empty()) {
    return OAuthCallbackError::Declined;
  }

  if (response.code.empty()) {
    return OAuthCallbackError::MissingCode;
  }

  code = response.code;
  return std::nullopt;
}

} // namespace sourcemeta::core
