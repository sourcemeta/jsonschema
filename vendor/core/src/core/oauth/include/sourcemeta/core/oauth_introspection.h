#ifndef SOURCEMETA_CORE_OAUTH_INTROSPECTION_H_
#define SOURCEMETA_CORE_OAUTH_INTROSPECTION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono::seconds
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

/// @ingroup oauth
/// Append a token introspection request body (RFC 7662 Section 2.1) to the
/// sink. The `token` is required, and the `token_type_hint` is emitted only
/// when present. The authorization the endpoint requires is supplied
/// separately, since RFC 7662 Section 2.1 mandates it but does not fix the
/// method. The token is secret, so the sink is a wiping string, and it is
/// appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_introspection_request(
///     "mF_9.B5f-4.1JqM", "", body);
/// assert(body == "token=mF_9.B5f-4.1JqM");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_introspection_request(const std::string_view token,
                                       const std::string_view token_type_hint,
                                       SecureString &sink) -> void;

/// @ingroup oauth
/// A non-owning view over a token introspection response (RFC 7662 Section 2.2)
/// held in a caller-owned JSON value, which must outlive the view. The `active`
/// state is computed once at construction, since it is the one per-request hot
/// accessor. A response must not be cached past its `exp` (RFC 7662 Section 4).
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(
///     R"JSON({"active":true,"scope":"read","client_id":"abc"})JSON")};
/// const sourcemeta::core::OAuthIntrospectionResponse response{document};
/// assert(response.active());
/// assert(response.scope().value() == "read");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthIntrospectionResponse {
public:
  /// Construct a view over an introspection response document, which is
  /// borrowed.
  explicit OAuthIntrospectionResponse(const JSON &data);

  /// Whether the token is active (RFC 7662 Section 2.2), computed at
  /// construction. A missing or non-boolean `active` is treated as inactive.
  [[nodiscard]] auto active() const noexcept -> bool;

  /// The space-delimited scope (RFC 7662 Section 2.2), or no value when absent.
  [[nodiscard]] auto scope() const -> std::optional<std::string_view>;
  /// The client identifier the token was issued to (RFC 7662 Section 2.2).
  [[nodiscard]] auto client_id() const -> std::optional<std::string_view>;
  /// The resource owner username (RFC 7662 Section 2.2).
  [[nodiscard]] auto username() const -> std::optional<std::string_view>;
  /// The token type, which is `DPoP` for a DPoP-bound token (RFC 7662
  /// Section 2.2, RFC 9449 Section 6.2).
  [[nodiscard]] auto token_type() const -> std::optional<std::string_view>;
  /// The subject of the token (RFC 7662 Section 2.2).
  [[nodiscard]] auto subject() const -> std::optional<std::string_view>;
  /// The issuer of the token (RFC 7662 Section 2.2).
  [[nodiscard]] auto issuer() const -> std::optional<std::string_view>;
  /// The identifier of the token (RFC 7662 Section 2.2).
  [[nodiscard]] auto jti() const -> std::optional<std::string_view>;
  /// The expiration time (RFC 7662 Section 2.2), or no value when absent.
  [[nodiscard]] auto expiration() const -> std::optional<std::chrono::seconds>;
  /// The issuance time (RFC 7662 Section 2.2).
  [[nodiscard]] auto issued_at() const -> std::optional<std::chrono::seconds>;
  /// The not-before time (RFC 7662 Section 2.2).
  [[nodiscard]] auto not_before() const -> std::optional<std::chrono::seconds>;

  /// The underlying document, for reaching members without a typed accessor
  /// such as `aud`, `may_act`, or `cnf`.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  const JSON *data_;
  bool active_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif

/// @ingroup oauth
/// Build the minimal inactive introspection response `{ "active": false }`
/// (RFC 7662 Section 2.2). The minimality is a recommendation, while the field
/// value is the requirement, so a caller may add members. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto response{sourcemeta::core::oauth_make_introspection_inactive()};
/// assert(!response.at("active").to_boolean());
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_make_introspection_inactive() -> JSON;

} // namespace sourcemeta::core

#endif
