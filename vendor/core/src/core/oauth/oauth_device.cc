#include <sourcemeta/core/oauth_device.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>

#include "oauth_encode.h"
#include "oauth_json.h"

#include <array>       // std::array
#include <chrono>      // std::chrono::seconds, std::chrono::steady_clock
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t, std::uint8_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

// RFC 8628 Section 6.1: the recommended base-20 user code alphabet, the
// uppercase letters A to Z with the vowels and Y removed so a code cannot form
// a word
constexpr std::string_view USER_CODE_ALPHABET{"BCDFGHJKLMNPQRSTVWXZ"};
constexpr std::chrono::seconds DEFAULT_INTERVAL{5};

const auto HASH_DEVICE_CODE{JSON::Object::hash("device_code"sv)};
const auto HASH_USER_CODE{JSON::Object::hash("user_code"sv)};
const auto HASH_VERIFICATION_URI{JSON::Object::hash("verification_uri"sv)};
const auto HASH_VERIFICATION_URI_COMPLETE{
    JSON::Object::hash("verification_uri_complete"sv)};
const auto HASH_EXPIRES_IN{JSON::Object::hash("expires_in"sv)};
const auto HASH_INTERVAL{JSON::Object::hash("interval"sv)};

auto normalize_user_code(const std::string_view value) -> SecureString {
  SecureString result;
  result.reserve(value.size());
  for (const auto character : value) {
    if (!is_alphanum(character)) {
      continue;
    }

    // The alphabet is uppercase, so a lowercase ASCII letter is folded up
    if (character >= 'a' && character <= 'z') {
      result.push_back(static_cast<char>(character - ('a' - 'A')));
    } else {
      result.push_back(character);
    }
  }

  return result;
}

} // namespace

auto oauth_build_device_authorization_request(
    const std::string_view client_id, const std::string_view scope,
    const std::span<const OAuthParameter> resources, std::string &sink)
    -> void {
  if (!client_id.empty()) {
    oauth_append_form_parameter(sink, "client_id", client_id);
  }

  if (!scope.empty()) {
    oauth_append_form_parameter(sink, "scope", scope);
  }

  for (const auto &resource : resources) {
    oauth_append_form_parameter(sink, resource.name, resource.value);
  }
}

auto oauth_build_token_request_device(
    const std::string_view device_code,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void {
  oauth_append_form_parameter(sink, "grant_type",
                              "urn:ietf:params:oauth:grant-type:device_code");
  oauth_append_form_parameter(sink, "device_code", device_code);
  for (const auto &resource : resources) {
    oauth_append_form_parameter(sink, resource.name, resource.value);
  }
}

OAuthDeviceAuthorizationResponse::OAuthDeviceAuthorizationResponse(
    const JSON &data)
    : data_{&data} {}

auto OAuthDeviceAuthorizationResponse::device_code() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "device_code"sv,
                                  HASH_DEVICE_CODE);
}

auto OAuthDeviceAuthorizationResponse::user_code() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "user_code"sv, HASH_USER_CODE);
}

auto OAuthDeviceAuthorizationResponse::verification_uri() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "verification_uri"sv,
                                  HASH_VERIFICATION_URI);
}

auto OAuthDeviceAuthorizationResponse::verification_uri_complete() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "verification_uri_complete"sv,
                                  HASH_VERIFICATION_URI_COMPLETE);
}

auto OAuthDeviceAuthorizationResponse::expires_in() const
    -> std::optional<std::chrono::seconds> {
  // RFC 8628 Section 3.2: expires_in is a REQUIRED positive lifetime, so a zero
  // or negative value is malformed and reported as absent rather than surfacing
  // as an immediate expiry to a poller
  const auto value{
      oauth_json_seconds_member(*this->data_, "expires_in"sv, HASH_EXPIRES_IN)};
  return (value.has_value() && value.value() > std::chrono::seconds{0})
             ? value
             : std::nullopt;
}

auto OAuthDeviceAuthorizationResponse::interval() const
    -> std::chrono::seconds {
  // RFC 8628 Section 3.2: absent interval means the default, and a non-positive
  // interval is malformed, so it also falls back to the default rather than
  // driving overly aggressive polling
  const auto value{
      oauth_json_seconds_member(*this->data_, "interval"sv, HASH_INTERVAL)};
  return (value.has_value() && value.value() > std::chrono::seconds{0})
             ? value.value()
             : DEFAULT_INTERVAL;
}

auto OAuthDeviceAuthorizationResponse::data() const -> const JSON & {
  return *this->data_;
}

OAuthDevicePoller::OAuthDevicePoller(
    const std::chrono::seconds interval, const std::chrono::seconds lifetime,
    const std::chrono::steady_clock::time_point start) noexcept
    : interval_{interval > std::chrono::seconds{0} ? interval
                                                   : DEFAULT_INTERVAL},
      lifetime_{lifetime}, start_{start} {}

auto OAuthDevicePoller::interval() const noexcept -> std::chrono::seconds {
  return this->interval_;
}

auto OAuthDevicePoller::expired(
    const std::chrono::steady_clock::time_point now) const noexcept -> bool {
  // A time before the start has not elapsed anything, and truncating its
  // negative sub-second difference to zero would otherwise read as expired for
  // a zero lifetime
  if (now < this->start_) {
    return false;
  }

  // Comparing the elapsed duration against the lifetime, rather than a
  // start-plus-lifetime deadline, keeps a large lifetime from overflowing
  return std::chrono::duration_cast<std::chrono::seconds>(now - this->start_) >=
         this->lifetime_;
}

auto OAuthDevicePoller::observe(const OAuthTokenError error) noexcept
    -> OAuthDevicePollDecision {
  switch (error) {
    case OAuthTokenError::SlowDown:
      // RFC 8628 Section 3.5: slow_down permanently increases the interval by
      // five seconds, saturating rather than overflowing a maximal interval
      if (this->interval_ <
          std::chrono::seconds{
              std::numeric_limits<std::chrono::seconds::rep>::max() - 5}) {
        this->interval_ += std::chrono::seconds{5};
      } else {
        this->interval_ = std::chrono::seconds{
            std::numeric_limits<std::chrono::seconds::rep>::max()};
      }

      return OAuthDevicePollDecision::Continue;
    case OAuthTokenError::AuthorizationPending:
      return OAuthDevicePollDecision::Continue;
    case OAuthTokenError::UseDPoPNonce:
      // RFC 9449 Section 8: the server requires a nonce, because the proof
      // carried none or a stale one, and now supplies it, so the proof is
      // re-minted with the nonce and the poll retried without growing the
      // interval
      return OAuthDevicePollDecision::RetryWithNonce;
    case OAuthTokenError::AccessDenied:
      return OAuthDevicePollDecision::Denied;
    case OAuthTokenError::ExpiredToken:
      return OAuthDevicePollDecision::Expired;
    default:
      return OAuthDevicePollDecision::Error;
  }
}

auto oauth_make_device_authorization_response(
    const std::string_view device_code, const std::string_view user_code,
    const std::string_view verification_uri,
    const std::string_view verification_uri_complete,
    const std::chrono::seconds expires_in, const std::chrono::seconds interval)
    -> std::optional<JSON> {
  // RFC 8628 Section 3.2: device_code, user_code, verification_uri, and a
  // positive expires_in lifetime are REQUIRED, so a missing one yields no
  // document rather than an unusable response
  if (device_code.empty() || user_code.empty() || verification_uri.empty() ||
      expires_in <= std::chrono::seconds{0}) {
    return std::nullopt;
  }

  auto response{JSON::make_object()};
  response.assign_assume_new("device_code", JSON{device_code},
                             HASH_DEVICE_CODE);
  response.assign_assume_new("user_code", JSON{user_code}, HASH_USER_CODE);
  response.assign_assume_new("verification_uri", JSON{verification_uri},
                             HASH_VERIFICATION_URI);
  if (!verification_uri_complete.empty()) {
    response.assign_assume_new("verification_uri_complete",
                               JSON{verification_uri_complete},
                               HASH_VERIFICATION_URI_COMPLETE);
  }

  response.assign_assume_new(
      "expires_in", JSON{static_cast<std::int64_t>(expires_in.count())},
      HASH_EXPIRES_IN);
  // RFC 8628 Section 3.2: the interval is emitted only when it is positive and
  // differs from the default the client would otherwise assume, so a client
  // never receives a zero or negative polling delay
  if (interval > std::chrono::seconds{0} && interval != DEFAULT_INTERVAL) {
    response.assign_assume_new(
        "interval", JSON{static_cast<std::int64_t>(interval.count())},
        HASH_INTERVAL);
  }

  return response;
}

auto oauth_device_user_code() -> std::array<char, 8> {
  std::array<char, 8> result{};
  const auto alphabet{static_cast<std::uint8_t>(USER_CODE_ALPHABET.size())};
  // Reject bytes in the biased tail so each character is drawn uniformly from
  // the alphabet, preserving the RFC 8628 Section 6.1 entropy of the code
  const auto limit{static_cast<std::uint8_t>(256U / alphabet * alphabet)};
  std::size_t filled{0};
  while (filled < result.size()) {
    std::array<std::uint8_t, 8> entropy{};
    random_bytes(entropy);
    for (const auto byte : entropy) {
      if (byte >= limit) {
        continue;
      }

      result[filled] = USER_CODE_ALPHABET[byte % alphabet];
      filled += 1;
      if (filled == result.size()) {
        break;
      }
    }
  }

  return result;
}

auto oauth_device_user_code_matches(const std::string_view presented,
                                    const std::string_view stored) -> bool {
  const auto normalized_presented{normalize_user_code(presented)};
  const auto normalized_stored{normalize_user_code(stored)};
  // A code has content, so an empty normalized value on either side, from an
  // empty or punctuation-only input, never matches, since secure_equals treats
  // two empty strings as equal
  if (normalized_presented.empty() || normalized_stored.empty()) {
    return false;
  }

  return secure_equals(normalized_presented, normalized_stored);
}

} // namespace sourcemeta::core
