#include <sourcemeta/core/http.h>
#include <sourcemeta/core/time.h>

#include <chrono>      // std::chrono::system_clock
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_from_date(const std::string_view value) noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  if (const auto result{from_imf_fixdate(value)}; result.has_value()) {
    return result;
  }
  if (const auto result{from_rfc850_date(value)}; result.has_value()) {
    return result;
  }
  return from_asctime(value);
}

} // namespace sourcemeta::core
