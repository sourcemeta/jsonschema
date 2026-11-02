#ifndef SOURCEMETA_CORE_OIDC_PROFILE_H_
#define SOURCEMETA_CORE_OIDC_PROFILE_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

/// @ingroup oidc
/// The behavioural profile a builder or validator runs under. `Strict` allows
/// only the Authorization Code flow with PKCE, and `Legacy` additionally
/// permits the Hybrid `code id_token` flow, which is weaker and off by default
/// (OpenID Connect Core 1.0 Section 3, the module design Section 14.2). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
///
/// const auto profile{sourcemeta::core::OIDCProfile::Strict};
/// ```
enum class OIDCProfile : std::uint8_t {
  /// The default, allowing only the Authorization Code flow and requiring PKCE
  /// with the `S256` method.
  Strict,
  /// Additionally permits the Hybrid `code id_token` flow, still seen in older
  /// deployments, which requires a `nonce` on the request. Validating its ID
  /// Token still requires the caller to enable `c_hash` checking through the ID
  /// Token validation options.
  Legacy
};

} // namespace sourcemeta::core

#endif
