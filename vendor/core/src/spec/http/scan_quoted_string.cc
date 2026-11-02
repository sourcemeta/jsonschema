#include <sourcemeta/core/http_syntax.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_scan_quoted_string(const std::string_view input,
                             const std::size_t position, std::string &storage,
                             std::string_view &value)
    -> std::optional<std::size_t> {
  if (position >= input.size() || input[position] != '"') {
    return std::nullopt;
  }

  // RFC 9110 Section 5.6.4: quoted text and the escaped octet of a quoted-pair
  // both admit what a field value admits, less the double quote and the
  // backslash, and those two are what this scan is tracking anyway
  const std::size_t content_start{position + 1};
  std::size_t scan{content_start};
  bool escaped{false};
  while (scan < input.size()) {
    const char character{input[scan]};
    if (character == '\\') {
      if (scan + 1 >= input.size() || !http_is_field_octet(input[scan + 1])) {
        return std::nullopt;
      }

      escaped = true;
      scan += 2;
      continue;
    }

    if (character == '"') {
      if (!escaped) {
        value = input.substr(content_start, scan - content_start);
      } else {
        const std::size_t base{storage.size()};
        storage.reserve(base + (scan - content_start));
        std::size_t index{content_start};
        while (index < scan) {
          if (input[index] == '\\') {
            storage.push_back(input[index + 1]);
            index += 2;
          } else {
            storage.push_back(input[index]);
            index += 1;
          }
        }

        value = std::string_view{storage}.substr(base);
      }

      return scan + 1;
    }

    if (!http_is_field_octet(character)) {
      return std::nullopt;
    }

    scan += 1;
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
