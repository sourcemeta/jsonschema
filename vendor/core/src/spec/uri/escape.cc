#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto URI::escape(const std::string_view input, const bool maybe_encoded)
    -> std::string {
  std::string result;
  URI::escape(input, result, maybe_encoded);
  return result;
}

auto URI::unescape(const std::string_view input) -> std::string {
  std::string result{input};
  uri_unescape_all_inplace(result);
  return result;
}

} // namespace sourcemeta::core
