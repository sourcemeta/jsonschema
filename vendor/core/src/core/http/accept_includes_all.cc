#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cassert>          // assert
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <string_view>      // std::string_view

namespace sourcemeta::core {

// NOLINTNEXTLINE(bugprone-exception-escape)
auto http_accept_includes_all(
    const std::string_view accept_header,
    const std::initializer_list<std::string_view> media_types) noexcept
    -> bool {
  if (http_trim_leading_ows(accept_header).empty()) {
    return true;
  }
  for (const auto media_type : media_types) {
    assert(!media_type.empty());
    [[maybe_unused]] const auto slash{media_type.find('/')};
    assert(slash != std::string_view::npos);
    assert(slash > 0);
    assert(slash < media_type.size() - 1);
    assert(media_type.find_first_of(" \t,;*") == std::string_view::npos);
    float best_quality{0.0f};
    std::uint8_t best_specificity{0};
    http_for_each_accept_entry(
        accept_header,
        [&](const std::string_view value,
            const float quality) noexcept -> void {
          const std::uint8_t specificity{
              http_media_specificity(value, media_type)};
          if (specificity == 0) {
            return;
          }
          if (specificity > best_specificity ||
              (specificity == best_specificity && quality > best_quality)) {
            best_quality = quality;
            best_specificity = specificity;
          }
        });
    if (best_specificity == 0 || best_quality == 0.0f) {
      return false;
    }
  }
  return true;
}

} // namespace sourcemeta::core
