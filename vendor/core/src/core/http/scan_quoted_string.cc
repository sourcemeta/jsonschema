#include <sourcemeta/core/http_syntax.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// RFC 9110 Section 5.6.4: qdtext and the escaped octet of a quoted-pair both
// admit HTAB, SP, the visible characters, and obs-text, so every other control
// character is rejected. The double quote and the backslash are handled by the
// caller before this check
auto http_is_quoted_content(const char character) noexcept -> bool {
  const auto byte{static_cast<unsigned char>(character)};
  return byte == 0x09 || (byte >= 0x20 && byte != 0x7f);
}

} // namespace

namespace sourcemeta::core {

auto http_scan_quoted_string(const std::string_view input,
                             const std::size_t position, std::string &storage,
                             std::string_view &value)
    -> std::optional<std::size_t> {
  if (position >= input.size() || input[position] != '"') {
    return std::nullopt;
  }

  const std::size_t content_start{position + 1};
  std::size_t scan{content_start};
  bool escaped{false};
  while (scan < input.size()) {
    const char character{input[scan]};
    if (character == '\\') {
      if (scan + 1 >= input.size() ||
          !http_is_quoted_content(input[scan + 1])) {
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

    if (!http_is_quoted_content(character)) {
      return std::nullopt;
    }

    scan += 1;
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
