#include <sourcemeta/core/oidc_error.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto oidc_error_code(const OIDCAuthenticationError error) noexcept
    -> std::string_view {
  switch (error) {
    case OIDCAuthenticationError::InteractionRequired:
      return "interaction_required";
    case OIDCAuthenticationError::LoginRequired:
      return "login_required";
    case OIDCAuthenticationError::AccountSelectionRequired:
      return "account_selection_required";
    case OIDCAuthenticationError::ConsentRequired:
      return "consent_required";
    case OIDCAuthenticationError::InvalidRequestURI:
      return "invalid_request_uri";
    case OIDCAuthenticationError::InvalidRequestObject:
      return "invalid_request_object";
    case OIDCAuthenticationError::RequestNotSupported:
      return "request_not_supported";
    case OIDCAuthenticationError::RequestURINotSupported:
      return "request_uri_not_supported";
    case OIDCAuthenticationError::RegistrationNotSupported:
      return "registration_not_supported";
  }

  std::unreachable();
}

auto to_oidc_authentication_error(const std::string_view code) noexcept
    -> std::optional<OIDCAuthenticationError> {
  if (code == "interaction_required") {
    return OIDCAuthenticationError::InteractionRequired;
  }
  if (code == "login_required") {
    return OIDCAuthenticationError::LoginRequired;
  }
  if (code == "account_selection_required") {
    return OIDCAuthenticationError::AccountSelectionRequired;
  }
  if (code == "consent_required") {
    return OIDCAuthenticationError::ConsentRequired;
  }
  if (code == "invalid_request_uri") {
    return OIDCAuthenticationError::InvalidRequestURI;
  }
  if (code == "invalid_request_object") {
    return OIDCAuthenticationError::InvalidRequestObject;
  }
  if (code == "request_not_supported") {
    return OIDCAuthenticationError::RequestNotSupported;
  }
  if (code == "request_uri_not_supported") {
    return OIDCAuthenticationError::RequestURINotSupported;
  }
  if (code == "registration_not_supported") {
    return OIDCAuthenticationError::RegistrationNotSupported;
  }
  return std::nullopt;
}

} // namespace sourcemeta::core
