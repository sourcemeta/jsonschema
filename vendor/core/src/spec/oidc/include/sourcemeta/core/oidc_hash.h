#ifndef SOURCEMETA_CORE_OIDC_HASH_H_
#define SOURCEMETA_CORE_OIDC_HASH_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/jose.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Compute an OpenID Connect access token hash (`at_hash`) or authorization
/// code hash (`c_hash`) for a token and the ID Token signing algorithm (OpenID
/// Connect Core 1.0 Section 3.1.3.6). The token is hashed with the SHA variant
/// the algorithm is defined over, the left-most half of the digest is kept, and
/// that half is base64url-encoded without padding. The digest is selected from
/// the algorithm by table rather than by slicing its name. Returns no value for
/// `EdDSA`, whose correct digest depends on the signing curve (SHA-512 for
/// Ed25519, SHAKE256 for Ed448) that the algorithm alone does not convey. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto hash{sourcemeta::core::oidc_token_hash(
///     "ya29.CjHSA1l5WUn8xZ6HanHFzzdHdbXm-14rxnC7JHch9eFIsZkQEGoWzaYG4o7k5f6BnPLj",
///     sourcemeta::core::JWSAlgorithm::RS256)};
/// assert(hash.has_value());
/// assert(hash.value() == "piwt8oCH-K2D9pXlaS1Y-w");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_token_hash(const std::string_view token, const JWSAlgorithm algorithm)
    -> std::optional<std::string>;

/// @ingroup oidc
/// Whether an `at_hash` or `c_hash` claim matches a token under the ID Token
/// signing algorithm, comparing in constant time (OpenID Connect Core 1.0
/// Section 3.1.3.6). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_verify_token_hash(
///     "ya29.CjHSA1l5WUn8xZ6HanHFzzdHdbXm-14rxnC7JHch9eFIsZkQEGoWzaYG4o7k5f6BnPLj",
///     sourcemeta::core::JWSAlgorithm::RS256, "piwt8oCH-K2D9pXlaS1Y-w"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_verify_token_hash(const std::string_view token,
                            const JWSAlgorithm algorithm,
                            const std::string_view claim) -> bool;

} // namespace sourcemeta::core

#endif
