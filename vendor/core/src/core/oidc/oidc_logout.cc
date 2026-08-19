#include <sourcemeta/core/oidc_logout.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/uri.h>

#include "oidc_verify.h"

#include <chrono>      // std::chrono::system_clock
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_SUB{JSON::Object::hash("sub"sv)};
constexpr auto HASH_SID{JSON::Object::hash("sid"sv)};
constexpr auto HASH_JTI{JSON::Object::hash("jti"sv)};
constexpr auto HASH_EVENTS{JSON::Object::hash("events"sv)};

constexpr std::string_view BACKCHANNEL_LOGOUT_EVENT{
    "http://schemas.openid.net/event/backchannel-logout"};
constexpr auto HASH_BACKCHANNEL_LOGOUT_EVENT{
    JSON::Object::hash(BACKCHANNEL_LOGOUT_EVENT)};

// The query is opened lazily on the first present parameter, so an all-empty
// request leaves the endpoint untouched rather than appending a dangling
// separator, and an endpoint that already carries a query is continued rather
// than opened again
auto append_logout_parameter(std::string &sink, const std::string_view endpoint,
                             const std::string_view name,
                             const std::string_view value, bool &query_opened)
    -> void {
  if (value.empty()) {
    return;
  }

  if (!query_opened) {
    if (!endpoint.contains('?')) {
      sink.push_back('?');
    } else if (sink.back() != '?' && sink.back() != '&') {
      sink.push_back('&');
    }

    query_opened = true;
  }

  URI::append_query_parameter(sink, name, value);
}

} // namespace

auto oidc_build_logout_url(const std::string_view end_session_endpoint,
                           const OIDCLogoutRequest &request, std::string &sink)
    -> void {
  sink.append(end_session_endpoint);

  bool query_opened{false};
  append_logout_parameter(sink, end_session_endpoint, "id_token_hint",
                          request.id_token_hint, query_opened);
  append_logout_parameter(sink, end_session_endpoint, "logout_hint",
                          request.logout_hint, query_opened);
  append_logout_parameter(sink, end_session_endpoint, "client_id",
                          request.client_id, query_opened);
  append_logout_parameter(sink, end_session_endpoint,
                          "post_logout_redirect_uri",
                          request.post_logout_redirect_uri, query_opened);
  append_logout_parameter(sink, end_session_endpoint, "state", request.state,
                          query_opened);
  append_logout_parameter(sink, end_session_endpoint, "ui_locales",
                          request.ui_locales, query_opened);
}

auto oidc_validate_logout_token(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view issuer, const std::string_view client_id,
    const std::chrono::system_clock::time_point now,
    const JWTClockSkew clock_skew) -> bool {
  // OpenID Connect Back-Channel Logout 1.0 Section 2.6 step: the algorithm is
  // pinned and the key selected by identifier, never taken from the header
  // alone, and alg none is never allowed
  if (!oidc_verify_selected_signature(token, keys, allowed_algorithms)) {
    return false;
  }

  // OpenID Connect Back-Channel Logout 1.0 Section 2.4: the typ header SHOULD
  // be logout+jwt, so it is validated only when present
  if (token.type().has_value() && !token.has_type("logout+jwt")) {
    return false;
  }

  // OpenID Connect Back-Channel Logout 1.0 Section 2.6: iss and aud as for an
  // ID Token, so the base JSON Web Token check runs rather than a second copy
  // of those rules. Sharing it is also what keeps the clock skew bounded the
  // same way here as on every other path that validates a token
  if (jwt_check_claims(token, issuer, client_id, now, clock_skew).has_value()) {
    return false;
  }

  // OpenID Connect Back-Channel Logout 1.0 Section 2.4: iat is REQUIRED, which
  // the base check treats as optional since RFC 7519 does
  if (!token.issued_at().has_value()) {
    return false;
  }

  const auto &payload{token.payload()};

  // OpenID Connect Back-Channel Logout 1.0 Section 2.4: a sub, a sid, or both,
  // each a string when present
  const auto *subject{payload.try_at("sub"sv, HASH_SUB)};
  const auto *session{payload.try_at("sid"sv, HASH_SID)};
  const auto has_subject{subject != nullptr && subject->is_string()};
  const auto has_session{session != nullptr && session->is_string()};
  if (!has_subject && !has_session) {
    return false;
  }

  // The events claim is an object carrying the back-channel logout member,
  // whose value is an object
  const auto *events{payload.try_at("events"sv, HASH_EVENTS)};
  if (events == nullptr || !events->is_object()) {
    return false;
  }

  const auto *logout_event{
      events->try_at(BACKCHANNEL_LOGOUT_EVENT, HASH_BACKCHANNEL_LOGOUT_EVENT)};
  if (logout_event == nullptr || !logout_event->is_object()) {
    return false;
  }

  // A logout token MUST NOT contain a nonce, which prevents it being confused
  // with an ID Token, and a jti string is REQUIRED for replay detection
  const auto *token_identifier{payload.try_at("jti"sv, HASH_JTI)};
  if (payload.defines("nonce") || token_identifier == nullptr ||
      !token_identifier->is_string()) {
    return false;
  }

  return true;
}

auto oidc_front_channel_pairing_is_valid(const std::string_view issuer,
                                         const std::string_view session_id)
    -> bool {
  // OpenID Connect Front-Channel Logout 1.0 Section 3: iss and sid are included
  // together or not at all
  return issuer.empty() == session_id.empty();
}

auto oidc_session_state(const std::string_view client_id,
                        const std::string_view origin,
                        const std::string_view provider_browser_state,
                        const std::string_view salt) -> std::string {
  // OpenID Connect Session Management 1.0 Section 4.2: the value is
  // base64url(SHA256(client_id + " " + origin + " " + op_browser_state + " " +
  // salt)) + "." + salt
  std::string message;
  message.reserve(client_id.size() + origin.size() +
                  provider_browser_state.size() + salt.size() + 3);
  message.append(client_id);
  message.push_back(' ');
  message.append(origin);
  message.push_back(' ');
  message.append(provider_browser_state);
  message.push_back(' ');
  message.append(salt);

  const auto digest{sha256_digest(message)};
  std::string result{base64url_encode(digest)};
  result.push_back('.');
  result.append(salt);
  return result;
}

} // namespace sourcemeta::core
