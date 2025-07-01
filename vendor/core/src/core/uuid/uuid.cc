#include <sourcemeta/core/uuid.h>

#include <cstdint> // std::uint8_t
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Adapted from https://stackoverflow.com/a/58467162/1641422
auto uuidv4() -> std::string {
  static std::random_device device;
  static std::mt19937 generator{device()};
  static constexpr std::string_view digits = "0123456789abcdef";
  static constexpr bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0,
                                  1, 0, 1, 0, 0, 0, 0, 0};
  std::uniform_int_distribution<decltype(digits)::size_type> distribution(0,
                                                                          15);
  std::string result;
  result.reserve(36);
  for (std::uint8_t index = 0; index < 16; index++) {
    if (dash[index]) {
      result += "-";
    }

    result += digits[distribution(generator)];
    result += digits[distribution(generator)];
  }

  return result;
}

} // namespace sourcemeta::core
