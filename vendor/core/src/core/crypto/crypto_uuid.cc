#include <sourcemeta/core/crypto_uuid.h>
#include <sourcemeta/core/text.h>

#include "crypto_random.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// See RFC 9562 Section 5.4
// Format: xxxxxxxx-xxxx-4xxx-Nxxx-xxxxxxxxxxxx
// where 4 is the version and N is the variant (8, 9, a, or b)
auto uuidv4() -> std::string {
  static constexpr std::string_view digits = "0123456789abcdef";
  static constexpr std::string_view variant_digits = "89ab";
  static constexpr std::array<bool, 16> dash = {
      {false, false, false, false, true, false, true, false, true, false, true,
       false, false, false, false, false}};

  std::array<std::uint8_t, 16> bytes{};
  fill_random_bytes(bytes);

  std::string result;
  result.reserve(36);
  for (std::size_t index = 0; index < dash.size(); ++index) {
    if (dash[index]) {
      result += '-';
    }

    const auto high_nibble = (bytes[index] >> 4u) & 0x0fu;
    const auto low_nibble = bytes[index] & 0x0fu;

    // RFC 9562 Section 5.4: version bits (48-51) must be 0b0100
    if (index == 6) {
      result += '4';
      // RFC 9562 Section 5.4: variant bits (64-65) must be 0b10
    } else if (index == 8) {
      result += variant_digits[high_nibble & 0x03u];
    } else {
      result += digits[high_nibble];
    }

    result += digits[low_nibble];
  }

  return result;
}

auto is_uuid_like(const std::string_view value) -> bool {
  // Layout: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (exactly 36 chars)
  if (value.size() != 36) {
    return false;
  }

  if (value[8] != '-' || value[13] != '-' || value[18] != '-' ||
      value[23] != '-') {
    return false;
  }

  for (std::string_view::size_type position{0}; position < 36; ++position) {
    if (position == 8 || position == 13 || position == 18 || position == 23) {
      continue;
    }
    if (!is_hex_digit(value[position])) {
      return false;
    }
  }

  return true;
}

} // namespace sourcemeta::core
