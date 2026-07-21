#include <sourcemeta/core/oauth_error.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto oauth_error_code(const OAuthAuthorizationError error) noexcept
    -> std::string_view {
  switch (error) {
    case OAuthAuthorizationError::InvalidRequest:
      return "invalid_request";
    case OAuthAuthorizationError::UnauthorizedClient:
      return "unauthorized_client";
    case OAuthAuthorizationError::AccessDenied:
      return "access_denied";
    case OAuthAuthorizationError::UnsupportedResponseType:
      return "unsupported_response_type";
    case OAuthAuthorizationError::InvalidScope:
      return "invalid_scope";
    case OAuthAuthorizationError::ServerError:
      return "server_error";
    case OAuthAuthorizationError::TemporarilyUnavailable:
      return "temporarily_unavailable";
  }

  std::unreachable();
}

auto oauth_error_code(const OAuthTokenError error) noexcept
    -> std::string_view {
  switch (error) {
    case OAuthTokenError::InvalidRequest:
      return "invalid_request";
    case OAuthTokenError::InvalidClient:
      return "invalid_client";
    case OAuthTokenError::InvalidGrant:
      return "invalid_grant";
    case OAuthTokenError::UnauthorizedClient:
      return "unauthorized_client";
    case OAuthTokenError::UnsupportedGrantType:
      return "unsupported_grant_type";
    case OAuthTokenError::InvalidScope:
      return "invalid_scope";
    case OAuthTokenError::AuthorizationPending:
      return "authorization_pending";
    case OAuthTokenError::SlowDown:
      return "slow_down";
    case OAuthTokenError::AccessDenied:
      return "access_denied";
    case OAuthTokenError::ExpiredToken:
      return "expired_token";
    case OAuthTokenError::InvalidTarget:
      return "invalid_target";
    case OAuthTokenError::InvalidDPoPProof:
      return "invalid_dpop_proof";
    case OAuthTokenError::UseDPoPNonce:
      return "use_dpop_nonce";
    case OAuthTokenError::UnsupportedTokenType:
      return "unsupported_token_type";
  }

  std::unreachable();
}

auto oauth_error_code(const OAuthBearerError error) noexcept
    -> std::string_view {
  switch (error) {
    case OAuthBearerError::InvalidRequest:
      return "invalid_request";
    case OAuthBearerError::InvalidToken:
      return "invalid_token";
    case OAuthBearerError::InsufficientScope:
      return "insufficient_scope";
    case OAuthBearerError::InvalidDPoPProof:
      return "invalid_dpop_proof";
    case OAuthBearerError::UseDPoPNonce:
      return "use_dpop_nonce";
  }

  std::unreachable();
}

auto oauth_error_code(const OAuthRegistrationError error) noexcept
    -> std::string_view {
  switch (error) {
    case OAuthRegistrationError::InvalidRedirectURI:
      return "invalid_redirect_uri";
    case OAuthRegistrationError::InvalidClientMetadata:
      return "invalid_client_metadata";
    case OAuthRegistrationError::InvalidSoftwareStatement:
      return "invalid_software_statement";
    case OAuthRegistrationError::UnapprovedSoftwareStatement:
      return "unapproved_software_statement";
  }

  std::unreachable();
}

auto to_oauth_authorization_error(const std::string_view code) noexcept
    -> std::optional<OAuthAuthorizationError> {
  if (code == "invalid_request") {
    return OAuthAuthorizationError::InvalidRequest;
  } else if (code == "unauthorized_client") {
    return OAuthAuthorizationError::UnauthorizedClient;
  } else if (code == "access_denied") {
    return OAuthAuthorizationError::AccessDenied;
  } else if (code == "unsupported_response_type") {
    return OAuthAuthorizationError::UnsupportedResponseType;
  } else if (code == "invalid_scope") {
    return OAuthAuthorizationError::InvalidScope;
  } else if (code == "server_error") {
    return OAuthAuthorizationError::ServerError;
  } else if (code == "temporarily_unavailable") {
    return OAuthAuthorizationError::TemporarilyUnavailable;
  } else {
    return std::nullopt;
  }
}

auto to_oauth_token_error(const std::string_view code) noexcept
    -> std::optional<OAuthTokenError> {
  if (code == "invalid_request") {
    return OAuthTokenError::InvalidRequest;
  } else if (code == "invalid_client") {
    return OAuthTokenError::InvalidClient;
  } else if (code == "invalid_grant") {
    return OAuthTokenError::InvalidGrant;
  } else if (code == "unauthorized_client") {
    return OAuthTokenError::UnauthorizedClient;
  } else if (code == "unsupported_grant_type") {
    return OAuthTokenError::UnsupportedGrantType;
  } else if (code == "invalid_scope") {
    return OAuthTokenError::InvalidScope;
  } else if (code == "authorization_pending") {
    return OAuthTokenError::AuthorizationPending;
  } else if (code == "slow_down") {
    return OAuthTokenError::SlowDown;
  } else if (code == "access_denied") {
    return OAuthTokenError::AccessDenied;
  } else if (code == "expired_token") {
    return OAuthTokenError::ExpiredToken;
  } else if (code == "invalid_target") {
    return OAuthTokenError::InvalidTarget;
  } else if (code == "invalid_dpop_proof") {
    return OAuthTokenError::InvalidDPoPProof;
  } else if (code == "use_dpop_nonce") {
    return OAuthTokenError::UseDPoPNonce;
  } else if (code == "unsupported_token_type") {
    return OAuthTokenError::UnsupportedTokenType;
  } else {
    return std::nullopt;
  }
}

auto to_oauth_bearer_error(const std::string_view code) noexcept
    -> std::optional<OAuthBearerError> {
  if (code == "invalid_request") {
    return OAuthBearerError::InvalidRequest;
  } else if (code == "invalid_token") {
    return OAuthBearerError::InvalidToken;
  } else if (code == "insufficient_scope") {
    return OAuthBearerError::InsufficientScope;
  } else if (code == "invalid_dpop_proof") {
    return OAuthBearerError::InvalidDPoPProof;
  } else if (code == "use_dpop_nonce") {
    return OAuthBearerError::UseDPoPNonce;
  } else {
    return std::nullopt;
  }
}

auto to_oauth_registration_error(const std::string_view code) noexcept
    -> std::optional<OAuthRegistrationError> {
  if (code == "invalid_redirect_uri") {
    return OAuthRegistrationError::InvalidRedirectURI;
  } else if (code == "invalid_client_metadata") {
    return OAuthRegistrationError::InvalidClientMetadata;
  } else if (code == "invalid_software_statement") {
    return OAuthRegistrationError::InvalidSoftwareStatement;
  } else if (code == "unapproved_software_statement") {
    return OAuthRegistrationError::UnapprovedSoftwareStatement;
  } else {
    return std::nullopt;
  }
}

auto oauth_token_error_status(const OAuthTokenError error,
                              const bool authenticated_via_header) noexcept
    -> HTTPStatus {
  // RFC 6749 Section 5.2: "If the client attempted to authenticate via the
  // 'Authorization' request header field, the authorization server MUST respond
  // with an HTTP 401 (Unauthorized) status code and include the
  // WWW-Authenticate response header field"
  if (error == OAuthTokenError::InvalidClient && authenticated_via_header) {
    return HTTP_STATUS_UNAUTHORIZED;
  }

  return HTTP_STATUS_BAD_REQUEST;
}

auto oauth_bearer_error_status(const OAuthBearerError error) noexcept
    -> HTTPStatus {
  // RFC 6750 Section 3.1 SHOULD-level status mapping, with the DPoP resource
  // codes accompanying a 401 (RFC 9449 Section 7)
  switch (error) {
    case OAuthBearerError::InvalidRequest:
      return HTTP_STATUS_BAD_REQUEST;
    case OAuthBearerError::InvalidToken:
    case OAuthBearerError::InvalidDPoPProof:
    case OAuthBearerError::UseDPoPNonce:
      return HTTP_STATUS_UNAUTHORIZED;
    case OAuthBearerError::InsufficientScope:
      return HTTP_STATUS_FORBIDDEN;
  }

  std::unreachable();
}

} // namespace sourcemeta::core
