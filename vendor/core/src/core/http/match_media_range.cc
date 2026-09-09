#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_match_media_range(const std::string_view media_type,
                            std::span<const std::string_view> ranges) noexcept
    -> std::string_view {
  const auto [candidate_value, candidate_parameters] =
      http_split_entry(media_type);
  const auto candidate{http_trim_leading_ows(candidate_value)};
  if (!http_is_media_type(candidate)) {
    return {};
  }

  std::string_view result;
  std::uint8_t best_specificity{0};
  for (const auto entry : ranges) {
    const auto [range_value, range_suffix] = http_split_entry(entry);
    const auto range{http_trim_leading_ows(range_value)};
    // RFC 9110 §12.5.1: "Accept = #( media-range [ weight ] )", so the weight
    // is not part of the range, and "Recipients SHOULD process any parameter
    // named "q" as weight, regardless of parameter ordering". That section
    // further notes that "the media type registry disallows parameters named
    // "q"", so a "q" can never be a media type parameter to match against
    const auto range_parameters{http_split_media_range(range_suffix).first};
    const auto specificity{http_media_range_specificity(
        range, range_parameters, candidate, candidate_parameters)};
    // RFC 9110 §12.5.1: "If more than one media range applies to a given type,
    // the most specific reference has precedence", so an equally specific
    // later range does not displace an earlier one
    if (specificity > best_specificity) {
      best_specificity = specificity;
      result = entry;
    }
  }

  return result;
}

} // namespace sourcemeta::core
