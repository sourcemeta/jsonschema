#ifndef SOURCEMETA_CORE_OAUTH_JSON_H_
#define SOURCEMETA_CORE_OAUTH_JSON_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_duration.h>

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

} // namespace sourcemeta::core

#endif
