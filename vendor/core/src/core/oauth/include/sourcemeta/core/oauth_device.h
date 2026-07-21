#ifndef SOURCEMETA_CORE_OAUTH_DEVICE_H_
#define SOURCEMETA_CORE_OAUTH_DEVICE_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_authorization.h>
#include <sourcemeta/core/oauth_error.h>

#include <array>       // std::array
#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

/// @ingroup oauth
/// Append a device authorization request body (RFC 8628 Section 3.1) to the
/// sink. The `client_id` is required for a public client, the `scope` is
/// emitted when present, and the repeatable resources follow. This request has
/// no secret, so the sink is an ordinary string, appended to and never cleared.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string>
///
/// std::string body;
/// sourcemeta::core::oauth_build_device_authorization_request(
///     "1406020730", "read", {}, body);
/// assert(body == "client_id=1406020730&scope=read");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_device_authorization_request(
    const std::string_view client_id, const std::string_view scope,
    const std::span<const OAuthParameter> resources, std::string &sink) -> void;

/// @ingroup oauth
/// Append a device grant token request body (RFC 8628 Section 3.4) to the sink.
/// The `device_code` is required, and the repeatable resources follow. No
/// `client_id` is emitted, so the caller composes a client authentication
/// builder into the same sink. The device code is a credential, so the sink is
/// a wiping string, appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_token_request_device(
///     "GmRhmhcxhwAzkoEqiMEg_DnyEysNkuNhszIySk9eS", {}, body);
/// assert(body == "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3A"
///                "device_code"
///                "&device_code=GmRhmhcxhwAzkoEqiMEg_DnyEysNkuNhszIySk9eS");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_token_request_device(
    const std::string_view device_code,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void;

/// @ingroup oauth
/// A non-owning view over a device authorization response (RFC 8628
/// Section 3.2) held in a caller-owned JSON value, which must outlive the view.
/// A client keeps displaying the user code even when it uses the complete
/// verification URI (RFC 8628 Section 3.3.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"JSON({
///   "device_code": "GmRh", "user_code": "WDJB-MJHT",
///   "verification_uri": "https://example.com/device", "expires_in": 1800
/// })JSON")};
/// const sourcemeta::core::OAuthDeviceAuthorizationResponse response{document};
/// assert(response.user_code().value() == "WDJB-MJHT");
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthDeviceAuthorizationResponse {
public:
  /// Construct a view over a device authorization response, which is borrowed.
  explicit OAuthDeviceAuthorizationResponse(const JSON &data);

  /// The device verification code (RFC 8628 Section 3.2).
  [[nodiscard]] auto device_code() const -> std::optional<std::string_view>;
  /// The end-user verification code (RFC 8628 Section 3.2).
  [[nodiscard]] auto user_code() const -> std::optional<std::string_view>;
  /// The end-user verification URI (RFC 8628 Section 3.2).
  [[nodiscard]] auto verification_uri() const
      -> std::optional<std::string_view>;
  /// The verification URI with the user code included (RFC 8628 Section 3.2).
  [[nodiscard]] auto verification_uri_complete() const
      -> std::optional<std::string_view>;
  /// The lifetime of the device and user codes, no value when non-positive
  /// (RFC 8628 Section 3.2).
  [[nodiscard]] auto expires_in() const -> std::optional<std::chrono::seconds>;
  /// The minimum polling interval, defaulting to five seconds when absent
  /// (RFC 8628 Section 3.2).
  [[nodiscard]] auto interval() const -> std::chrono::seconds;

  /// The underlying document.
  [[nodiscard]] auto data() const -> const JSON &;

private:
  const JSON *data_;
};

/// @ingroup oauth
/// What a device flow client does after a token endpoint error (RFC 8628
/// Section 3.5).
enum class OAuthDevicePollDecision : std::uint8_t {
  /// Keep polling at the current interval.
  Continue,
  /// The request needs a fresh DPoP nonce, so the client re-issues the proof
  /// with the server-supplied nonce and polls again without growing the
  /// interval (RFC 9449 Section 8).
  RetryWithNonce,
  /// The end user denied the request.
  Denied,
  /// The codes expired before approval.
  Expired,
  /// A terminal error other than pending, slow down, denial, expiry, or a
  /// nonce requirement.
  Error
};

/// @ingroup oauth
/// A pure state machine driving the device flow polling (RFC 8628 Section 3.5).
/// It tracks the polling interval and the code lifetime, and interprets token
/// endpoint errors, without performing any I/O. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <chrono>
/// #include <cassert>
///
/// sourcemeta::core::OAuthDevicePoller poller{
///     std::chrono::seconds{5}, std::chrono::seconds{1800},
///     std::chrono::steady_clock::now()};
/// assert(poller.observe(sourcemeta::core::OAuthTokenError::SlowDown) ==
///        sourcemeta::core::OAuthDevicePollDecision::Continue);
/// assert(poller.interval() == std::chrono::seconds{10});
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthDevicePoller {
public:
  /// Construct a poller with the interval and lifetime the device authorization
  /// response advertised, from a starting time. A steady clock is used so wall
  /// clock adjustments cannot shorten or extend the lifetime. An interval of
  /// zero or less defaults to five seconds (RFC 8628 Section 3.5).
  OAuthDevicePoller(const std::chrono::seconds interval,
                    const std::chrono::seconds lifetime,
                    const std::chrono::steady_clock::time_point start) noexcept;

  /// The current polling interval, which a `slow_down` error grows.
  [[nodiscard]] auto interval() const noexcept -> std::chrono::seconds;

  /// Whether the codes have expired locally by the given time.
  [[nodiscard]] auto
  expired(const std::chrono::steady_clock::time_point now) const noexcept
      -> bool;

  /// Interpret a token endpoint error, permanently adding five seconds to the
  /// interval on `slow_down` and continuing, continuing on
  /// `authorization_pending`, retrying with a nonce on a DPoP nonce requirement
  /// (RFC 9449 Section 8), and reporting a terminal decision otherwise (RFC
  /// 8628 Section 3.5).
  auto observe(const OAuthTokenError error) noexcept -> OAuthDevicePollDecision;

private:
  std::chrono::seconds interval_;
  std::chrono::seconds lifetime_;
  std::chrono::steady_clock::time_point start_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif

/// @ingroup oauth
/// Build a device authorization response document (RFC 8628 Section 3.2),
/// returning no value when a required part is missing: an empty device code,
/// user code, or verification URI, or a non-positive `expires_in`, which is a
/// REQUIRED positive lifetime. The verification URI complete is emitted when
/// present, and the interval only when it is positive and differs from the
/// default, so a client never sees a zero or negative polling delay. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <chrono>
/// #include <cassert>
///
/// const auto
/// response{sourcemeta::core::oauth_make_device_authorization_response(
///     "GmRh", "WDJB-MJHT", "https://example.com/device", "",
///     std::chrono::seconds{1800}, std::chrono::seconds{5})};
/// assert(response.value().at("user_code").to_string() == "WDJB-MJHT");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_make_device_authorization_response(
    const std::string_view device_code, const std::string_view user_code,
    const std::string_view verification_uri,
    const std::string_view verification_uri_complete,
    const std::chrono::seconds expires_in, const std::chrono::seconds interval)
    -> std::optional<JSON>;

/// @ingroup oauth
/// Mint a device flow user code, eight characters from the RFC 8628 Section 6.1
/// recommended twenty-character alphabet, drawn with rejection sampling for a
/// uniform distribution. The code is shown to the end user, not kept secret.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto code{sourcemeta::core::oauth_device_user_code()};
/// assert(code.size() == 8);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_device_user_code() -> std::array<char, 8>;

/// @ingroup oauth
/// Whether a user code the end user typed matches a stored one, comparing after
/// discarding every non-alphanumeric character and folding to uppercase, so the
/// separators and case a user adds do not matter (RFC 8628 Section 6.1). The
/// final comparison is constant time. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_device_user_code_matches("wdjb-mjht",
///                                                         "WDJBMJHT"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_device_user_code_matches(const std::string_view presented,
                                    const std::string_view stored) -> bool;

} // namespace sourcemeta::core

#endif
