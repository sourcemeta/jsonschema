#include <sourcemeta/core/oauth_revocation.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/http.h>
#include <sourcemeta/core/uri.h>

#include "oauth_decode.h"
#include "oauth_encode.h"

#include <functional>  // std::function
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

auto assign_lookup_scalar(const std::string_view value, SecureString &storage,
                          bool &seen, std::string_view &field) -> bool {
  // RFC 6749 Section 3.2: an empty value is treated as omitted, and a
  // recognized parameter must not appear twice
  if (value.empty()) {
    return true;
  }

  if (seen) {
    return false;
  }

  seen = true;
  return oauth_form_decode_into_secure(value, storage, field);
}

} // namespace

auto oauth_build_revocation_request(const std::string_view token,
                                    const std::string_view token_type_hint,
                                    SecureString &sink) -> void {
  oauth_append_form_parameter(sink, "token", token);
  if (!token_type_hint.empty()) {
    oauth_append_form_parameter(sink, "token_type_hint", token_type_hint);
  }
}

auto oauth_parse_revocation_request(
    const std::string_view body, SecureString &storage,
    OAuthTokenLookupRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool {
  result = {};
  // A single up-front reserve keeps every later decode from reallocating the
  // arena and dangling an earlier borrowed view (design convention 1)
  storage.reserve(storage.size() + body.size());
  const URI::Query parsed{body};
  bool has_token{false};
  bool has_token_type_hint{false};
  for (const auto &parameter : parsed) {
    // RFC 6749 Appendix B: the application/x-www-form-urlencoded format encodes
    // names too, so a name is decoded before it is recognized, and a malformed
    // escape fails the parse
    std::string_view name;
    if (!oauth_form_decode_into_secure(parameter.first, storage, name)) {
      return false;
    }

    const auto value{parameter.second};
    bool valid{true};
    if (name == "token") {
      valid = assign_lookup_scalar(value, storage, has_token, result.token);
    } else if (name == "token_type_hint") {
      valid = assign_lookup_scalar(value, storage, has_token_type_hint,
                                   result.token_type_hint);
    } else {
      std::string_view decoded;
      valid = oauth_form_decode_into_secure(value, storage, decoded);
      if (valid) {
        on_other(name, decoded);
      }
    }

    if (!valid) {
      return false;
    }
  }

  // RFC 7009 Section 2.1: the token is REQUIRED
  return has_token;
}

auto oauth_revocation_outcome(const HTTPStatus status) noexcept
    -> OAuthRevocationOutcome {
  if (status.code == HTTP_STATUS_OK.code) {
    return OAuthRevocationOutcome::Success;
  }

  if (status.code == HTTP_STATUS_SERVICE_UNAVAILABLE.code) {
    return OAuthRevocationOutcome::Retry;
  }

  return OAuthRevocationOutcome::Error;
}

} // namespace sourcemeta::core
