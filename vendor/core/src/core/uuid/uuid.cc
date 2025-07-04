#include <sourcemeta/core/uuid.h>

#include <array>   // std::array
#include <cstdint> // std::uint8_t
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Adapted from https://stackoverflow.com/a/58467162/1641422
auto uuidv4() -> std::string {
  static std::random_device device;
  static std::mt19937 generator{device()};
  static constexpr std::string_view digits = "0123456789abcdef";
  static constexpr std::array<bool, 16> dash = {
      {false, false, false, false, true, false, true, false, true, false, true,
       false, false, false, false, false}};
  std::uniform_int_distribution<decltype(digits)::size_type> distribution(0,
                                                                          15);
  std::string result;
  result.reserve(36);
  for (bool is_dash : dash) {
    if (is_dash) {
      result += "-";
    }

    result += digits[distribution(generator)];
    result += digits[distribution(generator)];
  }

  return result;
}

} // namespace sourcemeta::core
