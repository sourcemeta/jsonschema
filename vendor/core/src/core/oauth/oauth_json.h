#ifndef SOURCEMETA_CORE_OAUTH_JSON_H_
#define SOURCEMETA_CORE_OAUTH_JSON_H_

#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono::seconds
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Read a string member of a JSON object as a view into it, or no value when the
// object, the member, or its type is absent
inline auto oauth_json_string_member(const JSON &data,
                                     const JSON::StringView name,
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

// Read an integer member of a JSON object as a duration in seconds, rejecting a
// negative value as malformed, and one past the range of the duration so a
// bad lifetime or interval cannot flow to a caller as a usable or narrowed
// duration
inline auto oauth_json_seconds_member(const JSON &data,
                                      const JSON::StringView name,
                                      const JSON::Object::hash_type hash)
    -> std::optional<std::chrono::seconds> {
  if (!data.is_object()) {
    return std::nullopt;
  }

  const auto *member{data.try_at(name, hash)};
  if (member == nullptr || !member->is_integer() || member->to_integer() < 0 ||
      member->to_integer() >
          std::numeric_limits<std::chrono::seconds::rep>::max()) {
    return std::nullopt;
  }

  return std::chrono::seconds{member->to_integer()};
}

} // namespace sourcemeta::core

#endif
