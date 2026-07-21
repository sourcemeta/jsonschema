#include <sourcemeta/core/oauth_introspection.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include "oauth_encode.h"
#include "oauth_json.h"

#include <chrono>      // std::chrono::seconds
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_ACTIVE{JSON::Object::hash("active"sv)};
const auto HASH_SCOPE{JSON::Object::hash("scope"sv)};
const auto HASH_CLIENT_ID{JSON::Object::hash("client_id"sv)};
const auto HASH_USERNAME{JSON::Object::hash("username"sv)};
const auto HASH_TOKEN_TYPE{JSON::Object::hash("token_type"sv)};
const auto HASH_SUB{JSON::Object::hash("sub"sv)};
const auto HASH_ISS{JSON::Object::hash("iss"sv)};
const auto HASH_JTI{JSON::Object::hash("jti"sv)};
const auto HASH_EXP{JSON::Object::hash("exp"sv)};
const auto HASH_IAT{JSON::Object::hash("iat"sv)};
const auto HASH_NBF{JSON::Object::hash("nbf"sv)};

} // namespace

auto oauth_build_introspection_request(const std::string_view token,
                                       const std::string_view token_type_hint,
                                       SecureString &sink) -> void {
  oauth_append_form_parameter(sink, "token", token);
  if (!token_type_hint.empty()) {
    oauth_append_form_parameter(sink, "token_type_hint", token_type_hint);
  }
}

OAuthIntrospectionResponse::OAuthIntrospectionResponse(const JSON &data)
    : data_{&data}, active_{[&data]() -> bool {
        if (!data.is_object()) {
          return false;
        }

        // RFC 7662 Section 2.2: active is a REQUIRED boolean, so a missing or
        // non-boolean value is treated as inactive rather than trusted
        const auto *member{data.try_at("active"sv, HASH_ACTIVE)};
        return member != nullptr && member->is_boolean() &&
               member->to_boolean();
      }()} {}

auto OAuthIntrospectionResponse::active() const noexcept -> bool {
  return this->active_;
}

auto OAuthIntrospectionResponse::scope() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "scope"sv, HASH_SCOPE);
}

auto OAuthIntrospectionResponse::client_id() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "client_id"sv, HASH_CLIENT_ID);
}

auto OAuthIntrospectionResponse::username() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "username"sv, HASH_USERNAME);
}

auto OAuthIntrospectionResponse::token_type() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "token_type"sv,
                                  HASH_TOKEN_TYPE);
}

auto OAuthIntrospectionResponse::subject() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "sub"sv, HASH_SUB);
}

auto OAuthIntrospectionResponse::issuer() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "iss"sv, HASH_ISS);
}

auto OAuthIntrospectionResponse::jti() const
    -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "jti"sv, HASH_JTI);
}

auto OAuthIntrospectionResponse::expiration() const
    -> std::optional<std::chrono::seconds> {
  return oauth_json_seconds_member(*this->data_, "exp"sv, HASH_EXP);
}

auto OAuthIntrospectionResponse::issued_at() const
    -> std::optional<std::chrono::seconds> {
  return oauth_json_seconds_member(*this->data_, "iat"sv, HASH_IAT);
}

auto OAuthIntrospectionResponse::not_before() const
    -> std::optional<std::chrono::seconds> {
  return oauth_json_seconds_member(*this->data_, "nbf"sv, HASH_NBF);
}

auto OAuthIntrospectionResponse::data() const -> const JSON & {
  return *this->data_;
}

auto oauth_make_introspection_inactive() -> JSON {
  auto response{JSON::make_object()};
  response.assign_assume_new("active", JSON{false}, HASH_ACTIVE);
  return response;
}

} // namespace sourcemeta::core
