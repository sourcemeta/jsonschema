#ifndef SOURCEMETA_CORE_OIDC_CLAIMS_H_
#define SOURCEMETA_CORE_OIDC_CLAIMS_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <functional>  // std::function
#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Whether a name is one of the OpenID Connect standard claims (OpenID Connect
/// Core 1.0 Section 5.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_is_standard_claim("email"));
/// assert(!sourcemeta::core::oidc_is_standard_claim("custom_claim"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_is_standard_claim(const std::string_view name) noexcept -> bool;

/// @ingroup oidc
/// Invoke the callback with each standard claim that the space-delimited scopes
/// request (OpenID Connect Core 1.0 Section 5.4). The `openid` scope maps to
/// `sub`, which is always returned (Section 5.3.2), and `profile`, `email`,
/// `address`, and `phone` map to their claim sets. A claim requested by more
/// than one scope is reported once. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// bool has_email{false};
/// sourcemeta::core::oidc_scope_to_claims(
///     "openid email", [&has_email](std::string_view claim) {
///       has_email = has_email || claim == "email";
///     });
/// assert(has_email);
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_scope_to_claims(const std::string_view scopes,
                          const std::function<void(std::string_view)> &on_claim)
    -> void;

/// @ingroup oidc
/// The standard scope that requests a claim, or no value when no
/// claim-requesting scope carries it (OpenID Connect Core 1.0 Section 5.4). The
/// `sub` claim maps to `openid`, which always returns it (Section 5.3.2), and a
/// scope name is never invented from a non-standard claim name. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_claim_to_scope("email").value() == "email");
/// assert(!sourcemeta::core::oidc_claim_to_scope("groups").has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claim_to_scope(const std::string_view claim) noexcept
    -> std::optional<std::string_view>;

/// @ingroup oidc
/// A claim requested through the `claims` request parameter (OpenID Connect
/// Core 1.0 Section 5.5).
struct OIDCClaimRequest {
  /// The requested claim name.
  std::string_view name;
  /// Whether the claim is essential (OpenID Connect Core 1.0 Section 5.5.1).
  bool essential{false};
  /// A specific value the claim is requested to have, or no value (OpenID
  /// Connect Core 1.0 Section 5.5.1).
  const JSON *value{nullptr};
  /// A set of values the claim is requested to have one of, in order of
  /// preference (OpenID Connect Core 1.0 Section 5.5.1).
  std::span<const JSON> values{};
};

/// @ingroup oidc
/// Build a `claims` request parameter object requesting the given claims for
/// the UserInfo endpoint and the ID Token (OpenID Connect Core 1.0
/// Section 5.5). An essential claim is requested with `{"essential":true}`, and
/// a voluntary claim with `null`. A target with no claims is omitted. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
///
/// const std::array<sourcemeta::core::OIDCClaimRequest, 1> userinfo{
///     {{.name = "email", .essential = true}}};
/// const auto document{
///     sourcemeta::core::oidc_build_claims_parameter(userinfo, {})};
/// assert(document.defines("userinfo"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_build_claims_parameter(
    const std::span<const OIDCClaimRequest> userinfo_claims,
    const std::span<const OIDCClaimRequest> id_token_claims) -> JSON;

/// @ingroup oidc
/// Whether a `claims` request parameter requests a claim for a target member,
/// which is `userinfo` or `id_token` (OpenID Connect Core 1.0 Section 5.5). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto claims{sourcemeta::core::parse_json(
///     R"JSON({"id_token":{"auth_time":{"essential":true}}})JSON")};
/// assert(sourcemeta::core::oidc_claims_parameter_requests(
///     claims, "id_token", "auth_time"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claims_parameter_requests(const JSON &claims,
                                    const std::string_view target,
                                    const std::string_view claim) -> bool;

/// @ingroup oidc
/// Whether a `claims` request parameter marks a claim as essential for a target
/// member, which is `userinfo` or `id_token` (OpenID Connect Core 1.0
/// Section 5.5.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto claims{sourcemeta::core::parse_json(
///     R"JSON({"id_token":{"auth_time":{"essential":true}}})JSON")};
/// assert(sourcemeta::core::oidc_claims_parameter_is_essential(
///     claims, "id_token", "auth_time"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claims_parameter_is_essential(const JSON &claims,
                                        const std::string_view target,
                                        const std::string_view claim) -> bool;

/// @ingroup oidc
/// The specific value a `claims` request parameter asks a claim to have for a
/// target member, which is `userinfo` or `id_token`, or no value when none is
/// requested (OpenID Connect Core 1.0 Section 5.5.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto claims{sourcemeta::core::parse_json(
///     R"JSON({"id_token":{"acr":{"value":"urn:mace:silver"}}})JSON")};
/// const auto *value{sourcemeta::core::oidc_claims_parameter_value(
///     claims, "id_token", "acr")};
/// assert(value != nullptr && value->to_string() == "urn:mace:silver");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claims_parameter_value(const JSON &claims,
                                 const std::string_view target,
                                 const std::string_view claim) -> const JSON *;

/// @ingroup oidc
/// Whether the request for a claim permits the given value for a target member,
/// which is `userinfo` or `id_token` (OpenID Connect Core 1.0 Section 5.5.1). A
/// request with neither a `value` nor a `values` constraint permits any value,
/// and a claim that is not requested permits none. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto claims{sourcemeta::core::parse_json(
///     R"JSON({"id_token":{"acr":{"values":["a","b"]}}})JSON")};
/// assert(sourcemeta::core::oidc_claims_parameter_accepts(
///     claims, "id_token", "acr", sourcemeta::core::JSON{"b"}));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claims_parameter_accepts(const JSON &claims,
                                   const std::string_view target,
                                   const std::string_view claim,
                                   const JSON &value) -> bool;

/// @ingroup oidc
/// Whether an individual claim request permits a value, where the request is
/// the member value a `claims` request parameter maps a claim name to (OpenID
/// Connect Core 1.0 Section 5.5.1). A `null` request or one with neither a
/// `value` nor a `values` constraint permits any value, comparison is JSON
/// equality over the whole value, and `essential` has no effect. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(
///     R"JSON({ "values": [ "gold", "silver" ] })JSON")};
/// assert(sourcemeta::core::oidc_claim_request_accepts(
///     request, sourcemeta::core::JSON{"gold"}));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claim_request_accepts(const JSON &request, const JSON &value) -> bool;

/// @ingroup oidc
/// Whether an individual claim request permits a claim carrying a set rather
/// than a single value, which is the shape the claims that govern access
/// actually arrive in. RFC 9068 Section 2.2.3.1 has an authorization server
/// encode `groups`, `roles` and `entitlements` as the `User` resource
/// attributes of RFC 7643 Section 4.1.2.
///
/// An array is a set the caller belongs to, so any one member satisfying the
/// request satisfies it. A member is either a primitive or a complex object
/// (RFC 7643 Section 2.4), so one list may carry both, and anything else, a
/// nested array among them, is no member shape that section defines and
/// satisfies nothing. A complex member is compared on its `value`
/// sub-attribute alone, which that section defines as "the attribute's
/// significant value", never on `display`, which is "a human-readable name,
/// primarily used for display purposes" and so would let whoever can rename a
/// group grant access. A member carrying no `value` satisfies nothing.
///
/// RFC 7643 Section 2.5 holds an unassigned attribute, the null value, and the
/// empty array to be equivalent in state, so none of them carries membership.
/// An empty array, a null claim, a null member, and a complex member whose
/// `value` is null all satisfy nothing, even where the request constrains
/// nothing and would otherwise permit any value.
///
/// Any other value is compared as `oidc_claim_request_accepts` compares it.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto request{sourcemeta::core::parse_json(
///     R"JSON({ "value": "e9e30dba" })JSON")};
/// const auto groups{sourcemeta::core::parse_json(
///     R"JSON([ { "value": "e9e30dba", "display": "Tour Guides" } ])JSON")};
/// assert(sourcemeta::core::oidc_claim_request_accepts_multi_valued(
///     request, groups));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_claim_request_accepts_multi_valued(const JSON &request,
                                             const JSON &value) -> bool;

} // namespace sourcemeta::core

#endif
