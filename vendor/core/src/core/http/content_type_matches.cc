#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_content_type_matches(const std::string_view content_type_header,
                               const std::string_view media_type) noexcept
    -> bool {
  const auto bare{
      http_trim_leading_ows(http_split_entry(content_type_header).first)};
  return http_iequals_ascii(bare, media_type);
}

} // namespace sourcemeta::core
