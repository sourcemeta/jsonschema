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

} // namespace sourcemeta::core

#endif
