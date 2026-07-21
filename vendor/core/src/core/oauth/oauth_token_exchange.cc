#include <sourcemeta/core/oauth_token_exchange.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include "oauth_encode.h"
#include "oauth_json.h"

#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_ISSUED_TOKEN_TYPE{JSON::Object::hash("issued_token_type"sv)};

auto oauth_append_repeated(SecureString &sink, const std::string_view name,
                           const std::span<const std::string_view> values)
    -> void {
  for (const auto value : values) {
    oauth_append_form_parameter(sink, name, value);
  }
}

} // namespace

auto oauth_token_exchange_valid(const OAuthTokenExchangeRequest &request)
    -> bool {
  // RFC 8693 Section 2.1: the subject token and its type are REQUIRED, and the
  // actor token and its type are used together or not at all
  return !request.subject_token.empty() &&
         !request.subject_token_type.empty() &&
         request.actor_token.empty() == request.actor_token_type.empty();
}

auto oauth_build_token_request_exchange(
    const OAuthTokenExchangeRequest &request, SecureString &sink) -> bool {
  if (!oauth_token_exchange_valid(request)) {
    return false;
  }

  oauth_append_form_parameter(
      sink, "grant_type", "urn:ietf:params:oauth:grant-type:token-exchange");
  oauth_append_form_parameter(sink, "subject_token", request.subject_token);
  oauth_append_form_parameter(sink, "subject_token_type",
                              request.subject_token_type);
  if (!request.actor_token.empty()) {
    oauth_append_form_parameter(sink, "actor_token", request.actor_token);
    oauth_append_form_parameter(sink, "actor_token_type",
                                request.actor_token_type);
  }

  if (!request.requested_token_type.empty()) {
    oauth_append_form_parameter(sink, "requested_token_type",
                                request.requested_token_type);
  }

  if (!request.scope.empty()) {
    oauth_append_form_parameter(sink, "scope", request.scope);
  }

  // RFC 8693 Section 2.1 and RFC 8707 Section 2: each audience and resource is
  // emitted under its fixed parameter name, so the caller supplies only values
  oauth_append_repeated(sink, "audience", request.audiences);
  oauth_append_repeated(sink, "resource", request.resources);
  return true;
}

auto oauth_issued_token_type(const JSON &response)
    -> std::optional<std::string_view> {
  return oauth_json_string_member(response, "issued_token_type"sv,
                                  HASH_ISSUED_TOKEN_TYPE);
}

} // namespace sourcemeta::core
