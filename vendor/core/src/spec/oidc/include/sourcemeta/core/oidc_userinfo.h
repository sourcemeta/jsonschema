#ifndef SOURCEMETA_CORE_OIDC_USERINFO_H_
#define SOURCEMETA_CORE_OIDC_USERINFO_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Build a UserInfo response object for the given subject and additional
/// claims, ensuring the REQUIRED `sub` claim is present (OpenID Connect
/// Core 1.0 Section 5.3.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto
/// claims{sourcemeta::core::parse_json(R"JSON({"email":"a@b"})JSON")}; const
/// auto document{sourcemeta::core::oidc_build_userinfo("user-1", claims)};
/// assert(document.at("sub").to_string() == "user-1");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_build_userinfo(const std::string_view subject,
                         const JSON &additional_claims) -> JSON;

/// @ingroup oidc
/// Whether a UserInfo response subject matches the ID Token subject, comparing
/// in constant time, the defence against a substituted response (OpenID Connect
/// Core 1.0 Section 5.3.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto
/// userinfo{sourcemeta::core::parse_json(R"JSON({"sub":"user-1"})JSON")};
/// assert(sourcemeta::core::oidc_userinfo_matches_subject(userinfo, "user-1"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_userinfo_matches_subject(const JSON &userinfo,
                                   const std::string_view expected_subject)
    -> bool;

/// @ingroup oidc
/// Verify a signed UserInfo response and its subject, returning the claims or
/// no value when the signature does not verify under a pinned algorithm, the
/// subject does not match the ID Token subject, or `iss` or `aud` is missing or
/// does not identify this provider and client. A signed response MUST carry
/// `iss` and `aud` (OpenID Connect Core 1.0 Section 5.3.2, errata set 2), so a
/// signed response lacking either is rejected. Checking `aud` binds the
/// response to this client, preventing one minted for another client from being
/// accepted here. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
///
/// const auto token{sourcemeta::core::JWT::from(compact)};
/// const auto keys{sourcemeta::core::JWKS::from(key_set)};
/// const std::array allowed{sourcemeta::core::JWSAlgorithm::RS256};
/// assert(token.has_value() && keys.has_value());
/// const auto claims{sourcemeta::core::oidc_verify_userinfo(
///     token.value(), keys.value(), allowed, "user-1",
///     "https://op.example", "client-id")};
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_verify_userinfo(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view expected_subject,
    const std::string_view expected_issuer,
    const std::string_view expected_client_id) -> std::optional<JSON>;

/// @ingroup oidc
/// Combine the claims a provider asserted in an ID Token with those its
/// UserInfo endpoint returned about the same person, with no result when the
/// two cannot be established to be about one person. OpenID Connect Core 1.0
/// Section 5.3.2 requires the UserInfo subject "to exactly match the `sub`
/// Claim in the ID Token; if they do not match, the UserInfo Response values
/// MUST NOT be used", and Section 2 makes that claim required of an ID Token,
/// so a missing subject on either side is refused rather than merged.
///
/// Only the ID Token is signed, so its claims stand and UserInfo only fills
/// what it left out. Three groups are exempt from that filling, because their
/// members mean nothing apart from one another:
///
/// - `email` and `email_verified`, where Section 5.1 defines the latter as
///   asserting that "the End-User's e-mail address has been verified", which
///   is the address delivered alongside it. The pair is taken whole from the
///   answer that carried an address, and an assertion arriving without one is
///   dropped rather than left to vouch for the other answer's address.
/// - `phone_number` and `phone_number_verified`, which Section 5.1 relates the
///   same way, adding that a verified number "MUST be in E.164 format".
/// - `_claim_names` and `_claim_sources`, where Section 5.6.2 makes the former
///   "references to the member names in the `_claim_sources` member", so
///   taking one from each answer would leave a reference with nothing to
///   resolve against. They are taken together or not at all, and are never
///   merged across the two answers.
///
/// A claim that is absent, null, or an empty string delivers nothing, those
/// being the shapes Section 5.3.2 names in having an unreturned claim omitted
/// rather than "present with a null or empty string value". Such a claim
/// neither delivers a subject its assertion could speak for, nor blocks the
/// other answer from filling it, nor is itself carried over. Every other
/// value, an empty array or object included, is delivered as it stands, save
/// for the aggregated members, which name and resolve nothing when empty. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto token{sourcemeta::core::parse_json(
///     R"JSON({ "sub": "u1", "email_verified": true })JSON")};
/// const auto userinfo{sourcemeta::core::parse_json(
///     R"JSON({ "sub": "u1", "email": "a@b.test" })JSON")};
/// const auto claims{sourcemeta::core::oidc_merge_claims(token, userinfo)};
/// assert(claims.has_value());
/// // The assertion vouched for no address it carried, so it does not stand
/// assert(!claims.value().defines("email_verified"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_merge_claims(const JSON &id_token_claims, const JSON &userinfo)
    -> std::optional<JSON>;

} // namespace sourcemeta::core

#endif
