#ifndef SOURCEMETA_CORE_OIDC_DISCOVERY_H_
#define SOURCEMETA_CORE_OIDC_DISCOVERY_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Derive the OpenID Provider Configuration well-known URL from an issuer
/// identifier, returning no value when the issuer is not a valid identifier
/// (OpenID Connect Discovery 1.0 Section 4.1). Unlike the RFC 8414 form, the
/// well-known string is appended after the issuer path rather than inserted
/// before it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto url{sourcemeta::core::oidc_discovery_url("https://example.com")};
/// assert(url.has_value());
/// assert(url.value() ==
///        "https://example.com/.well-known/openid-configuration");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_discovery_url(const std::string_view issuer)
    -> std::optional<std::string>;

/// @ingroup oidc
/// A WebFinger issuer discovery request (OpenID Connect Discovery 1.0
/// Section 2), owning the normalized resource and the request URL.
struct OIDCWebFingerRequest {
  /// The normalized resource identifier, an `acct:` URI or an https URL (OpenID
  /// Connect Discovery 1.0 Section 2.1).
  std::string resource;
  /// The WebFinger request URL to retrieve (OpenID Connect Discovery 1.0
  /// Section 2).
  std::string url;
};

/// @ingroup oidc
/// Normalize a user-supplied identifier and build its WebFinger issuer
/// discovery request, returning no value when the identifier has no host
/// (OpenID Connect Discovery 1.0 Section 2, 2.1). An `acct:` identifier and an
/// https URL are kept as the resource, a bare `user@host` becomes an `acct:`
/// URI, and any other input is treated as an https URL. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::oidc_webfinger_request(
///     "acct:joe@example.com")};
/// assert(request.has_value());
/// assert(request.value().url.starts_with(
///     "https://example.com/.well-known/webfinger"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_webfinger_request(const std::string_view identifier)
    -> std::optional<OIDCWebFingerRequest>;

/// @ingroup oidc
/// Extract the issuer from a WebFinger JSON Resource Descriptor, the `href` of
/// the link whose `rel` is the OpenID Connect issuer relation and whose value
/// is a valid https issuer identifier, returning no value when none is present
/// (OpenID Connect Discovery 1.0 Section 2). The returned view borrows from the
/// descriptor, which must outlive it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto jrd{sourcemeta::core::parse_json(R"JSON({"links":[{
///     "rel":"http://openid.net/specs/connect/1.0/issuer",
///     "href":"https://example.com"}]})JSON")};
/// const auto issuer{sourcemeta::core::oidc_webfinger_issuer(jrd)};
/// assert(issuer.has_value());
/// assert(issuer.value() == "https://example.com");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_webfinger_issuer(const JSON &descriptor)
    -> std::optional<std::string_view>;

} // namespace sourcemeta::core

#endif
