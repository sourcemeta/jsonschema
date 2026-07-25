#ifndef SOURCEMETA_CORE_OIDC_SUBJECT_H_
#define SOURCEMETA_CORE_OIDC_SUBJECT_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// The subject identifier type an OpenID Provider assigns (OpenID Connect Core
/// 1.0 Section 8). A public subject is shared across clients, while a pairwise
/// subject is distinct per sector so clients cannot correlate the end user.
enum class OIDCSubjectType : std::uint8_t {
  /// The same subject value is returned to every client (OpenID Connect Core
  /// 1.0 Section 8).
  Public,
  /// A distinct subject value is returned to each sector (OpenID Connect Core
  /// 1.0 Section 8).
  Pairwise
};

/// @ingroup oidc
/// The wire name of a subject identifier type (OpenID Connect Core 1.0
/// Section 8). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_subject_type_name(
///            sourcemeta::core::OIDCSubjectType::Pairwise) == "pairwise");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_subject_type_name(const OIDCSubjectType type) noexcept
    -> std::string_view;

/// @ingroup oidc
/// Map a subject identifier type name to its value, returning no value for an
/// unrecognized name (OpenID Connect Core 1.0 Section 8). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oidc_subject_type("public").has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto to_oidc_subject_type(const std::string_view name) noexcept
    -> std::optional<OIDCSubjectType>;

/// @ingroup oidc
/// Derive a pairwise subject identifier from a sector identifier and a local
/// account identifier, keyed by a confidential OpenID Provider secret (OpenID
/// Connect Core 1.0 Section 8.1). The `provider_secret` must be kept secret, as
/// a public value would let account identifiers be enumerated from the pairwise
/// subjects. The value is a keyed HMAC-SHA256 over the sector and account,
/// base64url-encoded, so it is stable per client, distinct per sector,
/// non-reversible, and well under the 255 character limit. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto subject{sourcemeta::core::oidc_pairwise_subject(
///     "client.example", "user-1", "provider-secret")};
/// assert(!subject.empty());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_pairwise_subject(const std::string_view sector_identifier,
                           const std::string_view local_account_identifier,
                           const std::string_view provider_secret)
    -> std::string;

} // namespace sourcemeta::core

#endif
