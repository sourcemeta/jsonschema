#include <sourcemeta/core/crypto_uuid.h>

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#include <string_view> // std::string_view

namespace sourcemeta::core {

// See RFC 9562 Section 5.4
// Format: xxxxxxxx-xxxx-4xxx-Nxxx-xxxxxxxxxxxx
// where 4 is the version and N is the variant (8, 9, a, or b)
auto uuidv4() -> std::string {
  static std::random_device device;
  static std::mt19937 generator{device()};
  static constexpr std::string_view digits = "0123456789abcdef";
  static constexpr std::string_view variant_digits = "89ab";
  static constexpr std::array<bool, 16> dash = {
      {false, false, false, false, true, false, true, false, true, false, true,
       false, false, false, false, false}};
  std::uniform_int_distribution<decltype(digits)::size_type> distribution(0,
                                                                          15);
  std::uniform_int_distribution<decltype(variant_digits)::size_type>
      variant_distribution(0, 3);
  std::string result;
  result.reserve(36);
  for (std::size_t index = 0; index < dash.size(); ++index) {
    if (dash[index]) {
      result += '-';
    }

    // RFC 9562 Section 5.4: version bits (48-51) must be 0b0100
    if (index == 6) {
      result += '4';
      // RFC 9562 Section 5.4: variant bits (64-65) must be 0b10
    } else if (index == 8) {
      result += variant_digits[variant_distribution(generator)];
    } else {
      result += digits[distribution(generator)];
    }

    result += digits[distribution(generator)];
  }

  return result;
}

} // namespace sourcemeta::core
