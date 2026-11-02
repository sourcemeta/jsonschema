#ifndef SOURCEMETA_CORE_OAUTH_TTL_H_
#define SOURCEMETA_CORE_OAUTH_TTL_H_

#include <algorithm> // std::max, std::min
#include <chrono>    // std::chrono::seconds
#include <optional>  // std::optional

namespace sourcemeta::core {

// Clamp an advertised freshness lifetime into the honored range, or fall back
// when the transport advertised none. The upper and lower limits are applied
// as separate comparisons, so an inverted band whose minimum exceeds its
// maximum stays well defined with the minimum taking precedence
inline auto oauth_clamp_ttl(const std::optional<std::chrono::seconds> max_age,
                            const std::chrono::seconds fallback,
                            const std::chrono::seconds minimum,
                            const std::chrono::seconds maximum)
    -> std::chrono::seconds {
  if (max_age.has_value()) {
    return std::max(minimum, std::min(max_age.value(), maximum));
  }

  return fallback;
}

} // namespace sourcemeta::core

#endif
