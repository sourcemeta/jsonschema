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
    // RFC 9110 §12.5.1: a candidate may itself carry media-type parameters
    // (`text/plain;format=flowed`), so the type and its parameters are split
    // apart to be matched against each media range separately
    const auto [media_type_bare, media_type_parameters] =
        http_split_entry(media_type);
    [[maybe_unused]] const auto slash{media_type_bare.find('/')};
    assert(slash != std::string_view::npos);
    assert(slash > 0);
    assert(slash < media_type_bare.size() - 1);
    assert(media_type_bare.find_first_of(" \t,;*") == std::string_view::npos);
    float best_quality{0.0F};
    std::uint8_t best_specificity{0};
    http_for_each_media_range(
        accept_header,
        [&](const std::string_view value, const std::string_view parameters,
            const float quality) noexcept -> void {
          const std::uint8_t specificity{http_media_range_specificity(
              value, parameters, media_type_bare, media_type_parameters)};
          if (specificity == 0) {
            return;
          }
          if (specificity > best_specificity ||
              (specificity == best_specificity && quality > best_quality)) {
            best_quality = quality;
            best_specificity = specificity;
          }
        });
    if (best_specificity == 0 || best_quality == 0.0F) {
      return false;
    }
  }
  return true;
}

} // namespace sourcemeta::core
