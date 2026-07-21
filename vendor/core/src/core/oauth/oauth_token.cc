#include <sourcemeta/core/oauth_token.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "oauth_decode.h"
#include "oauth_encode.h"

#include <chrono>      // std::chrono::seconds
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t
#include <functional>  // std::function
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_ACCESS_TOKEN{JSON::Object::hash("access_token"sv)};
const auto HASH_TOKEN_TYPE{JSON::Object::hash("token_type"sv)};
const auto HASH_EXPIRES_IN{JSON::Object::hash("expires_in"sv)};
const auto HASH_REFRESH_TOKEN{JSON::Object::hash("refresh_token"sv)};
const auto HASH_SCOPE{JSON::Object::hash("scope"sv)};
const auto HASH_ERROR{JSON::Object::hash("error"sv)};
const auto HASH_ERROR_DESCRIPTION{JSON::Object::hash("error_description"sv)};
const auto HASH_ERROR_URI{JSON::Object::hash("error_uri"sv)};

auto string_member(const JSON &data, const JSON::StringView name,
                   const JSON::Object::hash_type hash)
    -> std::optional<std::string_view> {
  if (!data.is_object()) {
    return std::nullopt;
  }

  const auto *member{data.try_at(name, hash)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  return std::string_view{member->to_string()};
}

auto oauth_append_resources(SecureString &sink,
                            const std::span<const OAuthParameter> resources)
    -> void {
  for (const auto &resource : resources) {
    oauth_append_form_parameter(sink, resource.name, resource.value);
  }
}

auto assign_token_scalar(const std::string_view value, SecureString &storage,
                         bool &seen, std::string_view &field) -> bool {
  // RFC 6749 Section 3.2: "Parameters sent without a value MUST be treated as
  // if they were omitted", so an empty occurrence neither counts as a duplicate
  // nor marks the parameter present
  if (value.empty()) {
    return true;
  }

  // RFC 6749 Section 3.2: a recognized parameter must not appear twice at the
  // token endpoint
  if (seen) {
    return false;
  }

  seen = true;
  return oauth_form_decode_into_secure(value, storage, field);
}

} // namespace

auto oauth_build_token_request_code(
    const std::string_view code, const std::string_view redirect_uri,
    const std::string_view code_verifier,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void {
  oauth_append_form_parameter(sink, "grant_type", "authorization_code");
  oauth_append_form_parameter(sink, "code", code);
  if (!redirect_uri.empty()) {
    oauth_append_form_parameter(sink, "redirect_uri", redirect_uri);
  }

  if (!code_verifier.empty()) {
    oauth_append_form_parameter(sink, "code_verifier", code_verifier);
  }

  oauth_append_resources(sink, resources);
}

auto oauth_build_token_request_refresh(
    const std::string_view refresh_token, const std::string_view scope,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void {
  oauth_append_form_parameter(sink, "grant_type", "refresh_token");
  oauth_append_form_parameter(sink, "refresh_token", refresh_token);
  if (!scope.empty()) {
    oauth_append_form_parameter(sink, "scope", scope);
  }

  oauth_append_resources(sink, resources);
}

auto oauth_build_token_request_client_credentials(
    const std::string_view scope,
    const std::span<const OAuthParameter> resources, SecureString &sink)
    -> void {
  oauth_append_form_parameter(sink, "grant_type", "client_credentials");
  if (!scope.empty()) {
    oauth_append_form_parameter(sink, "scope", scope);
  }

  oauth_append_resources(sink, resources);
}

OAuthTokenResponse::OAuthTokenResponse(const JSON &data) : data_{&data} {}

auto OAuthTokenResponse::access_token() const
    -> std::optional<std::string_view> {
  return string_member(*this->data_, "access_token"sv, HASH_ACCESS_TOKEN);
}

auto OAuthTokenResponse::token_type() const -> std::optional<std::string_view> {
  return string_member(*this->data_, "token_type"sv, HASH_TOKEN_TYPE);
}

auto OAuthTokenResponse::is_bearer_token_type() const -> bool {
  const auto type{this->token_type()};
  // RFC 6749 Section 5.1: the token type value is compared case insensitively
  return type.has_value() && equals_ignore_case(type.value(), "Bearer");
}

auto OAuthTokenResponse::expires_in() const
    -> std::optional<std::chrono::seconds> {
  if (!this->data_->is_object()) {
    return std::nullopt;
  }

  const auto *member{this->data_->try_at("expires_in"sv, HASH_EXPIRES_IN)};
  if (member == nullptr || !member->is_integer()) {
    return std::nullopt;
  }

  const auto value{member->to_integer()};
  if (value < 0) {
    return std::nullopt;
  }

  return std::chrono::seconds{value};
}

auto OAuthTokenResponse::refresh_token() const
    -> std::optional<std::string_view> {
  return string_member(*this->data_, "refresh_token"sv, HASH_REFRESH_TOKEN);
}

auto OAuthTokenResponse::scope() const -> std::optional<std::string_view> {
  return string_member(*this->data_, "scope"sv, HASH_SCOPE);
}

auto OAuthTokenResponse::has_scope(const std::string_view value) const -> bool {
  const auto granted{this->scope()};
  if (!granted.has_value() || value.empty()) {
    return false;
  }

  // RFC 6749 Section 3.3: scope is a space-delimited, unordered set of tokens,
  // scanned without allocation
  const auto text{granted.value()};
  std::size_t position{0};
  while (position < text.size()) {
    const auto next{text.find(' ', position)};
    const auto token{text.substr(position, next == std::string_view::npos
                                               ? std::string_view::npos
                                               : next - position)};
    if (token == value) {
      return true;
    }

    if (next == std::string_view::npos) {
      break;
    }

    position = next + 1;
  }

  return false;
}

auto OAuthTokenResponse::data() const -> const JSON & { return *this->data_; }

auto oauth_parse_token_request(
    const std::string_view body, SecureString &storage,
    OAuthTokenRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool {
  result = {};
  // A single up-front reserve keeps every later decode from reallocating the
  // arena and dangling an earlier borrowed view (design convention 1). The
  // token endpoint body carries the request's secrets, the authorization code,
  // the PKCE verifier, the refresh token, and the client authentication
  // parameters, so the arena is a wiping string (engineering invariant 1)
  storage.reserve(storage.size() + body.size());
  const URI::Query parsed{body};
  bool has_grant_type{false};
  bool has_code{false};
  bool has_redirect_uri{false};
  bool has_code_verifier{false};
  bool has_refresh_token{false};
  bool has_scope{false};
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
    if (name == "grant_type") {
      valid = assign_token_scalar(value, storage, has_grant_type,
                                  result.grant_type);
    } else if (name == "code") {
      valid = assign_token_scalar(value, storage, has_code, result.code);
    } else if (name == "redirect_uri") {
      valid = assign_token_scalar(value, storage, has_redirect_uri,
                                  result.redirect_uri);
    } else if (name == "code_verifier") {
      valid = assign_token_scalar(value, storage, has_code_verifier,
                                  result.code_verifier);
    } else if (name == "refresh_token") {
      valid = assign_token_scalar(value, storage, has_refresh_token,
                                  result.refresh_token);
    } else if (name == "scope") {
      valid = assign_token_scalar(value, storage, has_scope, result.scope);
    } else {
      // Repeatable resource and audience indicators, and every other parameter
      // including the client authentication ones, are surfaced with their
      // decoded value rather than stored on the result
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

  return true;
}

auto oauth_make_token_response(const OAuthTokenGrant &grant) -> JSON {
  auto response{JSON::make_object()};
  // The keys are unique and their hashes are already computed, so the response
  // is assembled without a per-key hash or duplicate scan
  // RFC 6749 Section 5.1: access_token and token_type are REQUIRED
  response.assign_assume_new("access_token", JSON{grant.access_token},
                             HASH_ACCESS_TOKEN);
  response.assign_assume_new("token_type", JSON{grant.token_type},
                             HASH_TOKEN_TYPE);
  // RFC 6749 Section 5.1: the lifetime is a number of seconds, so a negative
  // duration is not representable and is omitted rather than emitted as a value
  // this module's own response reader would reject
  if (grant.expires_in.has_value() &&
      grant.expires_in.value() >= std::chrono::seconds{0}) {
    response.assign_assume_new(
        "expires_in",
        JSON{static_cast<std::int64_t>(grant.expires_in.value().count())},
        HASH_EXPIRES_IN);
  }

  if (!grant.refresh_token.empty()) {
    response.assign_assume_new("refresh_token", JSON{grant.refresh_token},
                               HASH_REFRESH_TOKEN);
  }

  // RFC 6749 Section 5.1: the scope is OPTIONAL when identical to the requested
  // scope and REQUIRED otherwise, so it is emitted only when it differs
  if (!grant.scope.empty() && grant.scope != grant.requested_scope) {
    response.assign_assume_new("scope", JSON{grant.scope}, HASH_SCOPE);
  }

  return response;
}

auto oauth_make_token_error_response(const std::string_view error,
                                     const std::string_view error_description,
                                     const std::string_view error_uri) -> JSON {
  auto response{JSON::make_object()};
  // RFC 6749 Section 5.2: error is REQUIRED
  response.assign_assume_new("error", JSON{error}, HASH_ERROR);
  if (!error_description.empty()) {
    response.assign_assume_new("error_description", JSON{error_description},
                               HASH_ERROR_DESCRIPTION);
  }

  if (!error_uri.empty()) {
    response.assign_assume_new("error_uri", JSON{error_uri}, HASH_ERROR_URI);
  }

  return response;
}

} // namespace sourcemeta::core
