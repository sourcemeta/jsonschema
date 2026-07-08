#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cassert>          // assert
#include <cstddef>          // std::size_t
#include <initializer_list> // std::initializer_list
#include <string_view>      // std::string_view

namespace {

auto language_specificity(const std::string_view range,
                          const std::string_view candidate) noexcept
    -> std::size_t {
  if (range == "*") {
    return 1;
  }
  if (sourcemeta::core::equals_ignore_case(range, candidate)) {
    return candidate.size() + 1;
  }
  if (range.size() > candidate.size() && range[candidate.size()] == '-' &&
      sourcemeta::core::equals_ignore_case(
          sourcemeta::core::http_subview(range, 0, candidate.size()),
          candidate)) {
    return candidate.size();
  }
  return 0;
}

} // namespace

namespace sourcemeta::core {

auto http_match_accept_language(
    const std::string_view accept_language_header,
    const std::initializer_list<std::string_view> candidates)
    -> std::string_view {
  if (candidates.size() == 0) {
    return {};
  }
  if (http_trim_leading_ows(accept_language_header).empty()) {
    return *candidates.begin();
  }

  std::string_view best{};
  float best_quality{0.0f};
  std::size_t best_specificity{0};
  std::size_t best_order{candidates.size()};

  std::size_t order{0};
  for (const auto candidate : candidates) {
    assert(!candidate.empty());
    assert(candidate.find_first_of(" \t,;*/") == std::string_view::npos);
    assert(candidate[0] != '-');
    assert(candidate[candidate.size() - 1] != '-');
    float candidate_quality{0.0f};
    std::size_t candidate_specificity{0};
    http_for_each_accept_entry(
        accept_language_header,
        [&](const std::string_view value, const float quality) -> void {
          const std::size_t specificity{language_specificity(value, candidate)};
          if (specificity == 0) {
            return;
          }
          // RFC 9110 Section 12.4.2: a quality of zero means "not acceptable".
          // Honour that by letting the most specific matching range govern, so
          // an explicit refusal of a specific range is not overridden by a less
          // specific range that offers a higher quality
          if (specificity > candidate_specificity ||
              (specificity == candidate_specificity &&
               quality > candidate_quality)) {
            candidate_quality = quality;
            candidate_specificity = specificity;
          }
        });
    if (candidate_quality > 0.0f &&
        (candidate_quality > best_quality ||
         (candidate_quality == best_quality &&
          candidate_specificity > best_specificity) ||
         (candidate_quality == best_quality &&
          candidate_specificity == best_specificity && order < best_order))) {
      best = candidate;
      best_quality = candidate_quality;
      best_specificity = candidate_specificity;
      best_order = order;
    }
    ++order;
  }
  return best;
}

} // namespace sourcemeta::core
