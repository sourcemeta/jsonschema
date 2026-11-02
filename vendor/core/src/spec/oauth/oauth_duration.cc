#include <sourcemeta/core/oauth_duration.h>

#include <sourcemeta/core/json.h>

#include <chrono>   // std::chrono::seconds
#include <limits>   // std::numeric_limits
#include <optional> // std::optional, std::nullopt

namespace sourcemeta::core {

auto oauth_json_seconds_member(const JSON &data, const JSON::StringView name,
                               const JSON::Object::hash_type hash)
    -> std::optional<std::chrono::seconds> {
  if (!data.is_object()) {
    return std::nullopt;
  }

  const auto *member{data.try_at(name, hash)};
  if (member == nullptr || !member->is_integer() || member->to_integer() < 0) {
    return std::nullopt;
  }

  // A JSON integer is a signed 64 bit value, whereas a duration is only
  // promised 35 bits, so a platform whose duration is the narrower of the two
  // has to range check the value before narrowing it. Where the two are the
  // same width the comparison cannot fail, and asking for it at compile time
  // discards it rather than leaving an unreachable branch behind
  if constexpr (std::numeric_limits<JSON::Integer>::max() >
                std::numeric_limits<std::chrono::seconds::rep>::max()) {
    if (member->to_integer() >
        std::numeric_limits<std::chrono::seconds::rep>::max()) {
      return std::nullopt;
    }
  }

  return std::chrono::seconds{member->to_integer()};
}

} // namespace sourcemeta::core
