#ifndef SOURCEMETA_CORE_OIDC_ERROR_H_
#define SOURCEMETA_CORE_OIDC_ERROR_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <cstdint>     // std::uint8_t
#include <exception>   // std::exception
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup oidc
/// An error that occurs when parsing an invalid OpenID Provider metadata
/// document.
class SOURCEMETA_CORE_OIDC_EXPORT OIDCMetadataParseError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid OpenID Provider metadata document";
  }
};

/// @ingroup oidc
/// An error that occurs when parsing an invalid OpenID Connect client
/// registration document.
class SOURCEMETA_CORE_OIDC_EXPORT OIDCRegistrationParseError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid OpenID Connect client registration "
           "document";
  }
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @ingroup oidc
/// The error codes an authentication endpoint returns in addition to the OAuth
/// authorization error codes (OpenID Connect Core 1.0 Section 3.1.2.6).
enum class OIDCAuthenticationError : std::uint8_t {
  /// The authentication request cannot be completed without end user
  /// interaction, returned when `prompt=none` was requested.
  InteractionRequired,
  /// The authentication request cannot be completed without end user
  /// authentication, returned when `prompt=none` was requested.
  LoginRequired,
  /// The end user must select a session, returned when `prompt=none` was
  /// requested and more than one session is available.
  AccountSelectionRequired,
  /// The authentication request cannot be completed without end user consent,
  /// returned when `prompt=none` was requested.
  ConsentRequired,
  /// The `request_uri` is invalid or unreachable.
  InvalidRequestURI,
  /// The request object is invalid.
  InvalidRequestObject,
  /// The provider does not support the `request` parameter.
  RequestNotSupported,
  /// The provider does not support the `request_uri` parameter.
  RequestURINotSupported,
  /// The provider does not support the `registration` parameter.
  RegistrationNotSupported
};

/// @ingroup oidc
/// The wire code for an authentication endpoint error (OpenID Connect Core 1.0
/// Section 3.1.2.6). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_error_code(
///            sourcemeta::core::OIDCAuthenticationError::LoginRequired) ==
///        "login_required");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_error_code(const OIDCAuthenticationError error) noexcept
    -> std::string_view;

/// @ingroup oidc
/// Map an authentication endpoint error code to its value, returning no value
/// for an unrecognized code (OpenID Connect Core 1.0 Section 3.1.2.6). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oidc_authentication_error("consent_required")
///            .has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto to_oidc_authentication_error(const std::string_view code) noexcept
    -> std::optional<OIDCAuthenticationError>;

} // namespace sourcemeta::core

#endif
