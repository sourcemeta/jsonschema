#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "grammar.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::int8_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto URI::escape(const std::string_view input, std::string &output,
                 const bool maybe_encoded) -> void {
  output.reserve(output.size() + input.size() * 3);
  if (!maybe_encoded) {
    for (const auto character : input) {
      if (uri_is_unreserved(character)) {
        output += character;
      } else {
        uri_percent_encode_byte(output, static_cast<unsigned char>(character));
      }
    }

    return;
  }

  // Treat the input as possibly already encoded, decoding each octet before
  // re-encoding so that a valid escape survives and a needlessly encoded
  // unreserved octet is decoded
  for (std::size_t position = 0; position < input.size();) {
    const auto high{input[position] == URI_PERCENT &&
                            position + 2 < input.size()
                        ? hex_digit_value(input[position + 1])
                        : static_cast<std::int8_t>(-1)};
    const auto low{high < 0 ? static_cast<std::int8_t>(-1)
                            : hex_digit_value(input[position + 2])};

    unsigned char byte{};
    if (low < 0) {
      byte = static_cast<unsigned char>(input[position]);
      position += 1;
    } else {
      byte = static_cast<unsigned char>((high << 4) | low);
      position += 3;
    }

    if (uri_is_unreserved(static_cast<char>(byte))) {
      output += static_cast<char>(byte);
    } else {
      uri_percent_encode_byte(output, byte);
    }
  }
}

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
