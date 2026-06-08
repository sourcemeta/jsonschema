#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <initializer_list> // std::initializer_list
#include <string_view>      // std::string_view

namespace sourcemeta::core {

// NOLINTNEXTLINE(bugprone-exception-escape)
auto http_field_list_contains_any(
    const std::string_view header_value,
    std::initializer_list<std::string_view> tokens) noexcept -> bool {
  bool found{false};
  http_for_each_field_value(header_value, [&](const std::string_view value) {
    if (found) {
      return;
    }
    for (const auto token : tokens) {
      if (value == token) {
        found = true;
        return;
      }
    }
  });
  return found;
}

} // namespace sourcemeta::core
