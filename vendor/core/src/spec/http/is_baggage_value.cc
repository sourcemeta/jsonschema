#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace {

// The octet production runs in five alternatives, a single character and four
// ranges, which its own comment explains as the US-ASCII characters other than
// the control ones, whitespace, the double quote, the comma, the semicolon and
// the backslash. The four gaps between those alternatives are where the last
// four of those sit
// https://www.w3.org/TR/baggage/#definition
auto is_baggage_octet(const char character) noexcept -> bool {
  const auto byte{static_cast<unsigned char>(character)};
  return byte >= 0x21 && byte <= 0x7E && byte != '"' && byte != ',' &&
         byte != ';' && byte != '\\';
}

} // namespace

namespace sourcemeta::core {

auto http_is_baggage_value(const std::string_view value) noexcept -> bool {
  for (std::size_t index{0}; index < value.size(); ++index) {
    if (!is_baggage_octet(value[index])) {
      return false;
    }

    // The percent is an octet like any other as far as the grammar is
    // concerned, so this is stricter than the grammar on the strength of the
    // prose: "The percent code point (U+0025) MUST be percent-encoded"
    // https://www.w3.org/TR/baggage/#value
    if (value[index] == '%') {
      if (!is_percent_triplet(value, index)) {
        return false;
      }

      index += 2;
    }
  }

  return true;
}

} // namespace sourcemeta::core
