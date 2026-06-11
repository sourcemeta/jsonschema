#include <sourcemeta/core/crypto_fnv128.h>

#include <array>   // std::array
#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t

namespace {

constexpr std::array<char, 17> HEX_DIGITS{{'0', '1', '2', '3', '4', '5', '6',
                                           '7', '8', '9', 'a', 'b', 'c', 'd',
                                           'e', 'f', '\0'}};

// The 128-bit FNV offset basis, in two 64-bit limbs
// (draft-eastlake-fnv Section 5)
constexpr std::uint64_t OFFSET_BASIS_HIGH{0x6c62272e07bb0142ULL};
constexpr std::uint64_t OFFSET_BASIS_LOW{0x62b821756295c58dULL};

// The 128-bit FNV prime is 2^88 + 2^8 + 0x3b (draft-eastlake-fnv Section 4),
// so multiplying by it reduces to (value << 88) + (value << 8) + value * 0x3b
// modulo 2^128, computed here over two 64-bit limbs
inline constexpr auto multiply_by_prime(std::uint64_t &high,
                                        std::uint64_t &low) noexcept -> void {
  // value << 88, whose low limb is always zero
  const auto shift_88_high = low << 24u;

  // value << 8
  const auto shift_8_high = (high << 8u) | (low >> 56u);
  const auto shift_8_low = low << 8u;

  // value * 0x3b, multiplying the low limb in 32-bit halves to portably
  // capture the carry into the high limb
  const auto low_half_product = (low & 0xffffffffULL) * 0x3bULL;
  const auto high_half_product = (low >> 32u) * 0x3bULL;
  const auto small_low = low_half_product + (high_half_product << 32u);
  const auto small_carry =
      (high_half_product + (low_half_product >> 32u)) >> 32u;
  const auto small_high = (high * 0x3bULL) + small_carry;

  const auto result_low = shift_8_low + small_low;
  const auto result_carry = result_low < shift_8_low ? 1ULL : 0ULL;
  high = shift_88_high + shift_8_high + small_high + result_carry;
  low = result_low;
}

} // namespace

namespace sourcemeta::core {

auto fnv128_digest(const std::string_view input)
    -> std::array<std::uint8_t, 16> {
  auto high = OFFSET_BASIS_HIGH;
  auto low = OFFSET_BASIS_LOW;

  // FNV-1 multiplies first and XORs after (draft-eastlake-fnv Section 2)
  for (const auto character : input) {
    multiply_by_prime(high, low);
    low ^= static_cast<std::uint8_t>(character);
  }

  std::array<std::uint8_t, 16> result{};
  for (std::uint64_t index = 0u; index < 8u; ++index) {
    const auto shift = 8u * (7u - index);
    result[index] = static_cast<std::uint8_t>((high >> shift) & 0xffu);
    result[8u + index] = static_cast<std::uint8_t>((low >> shift) & 0xffu);
  }

  return result;
}

auto fnv128(const std::string_view input) -> std::string {
  const auto digest = fnv128_digest(input);
  std::string result;
  result.reserve(32);
  for (std::uint64_t index = 0u; index < 16u; ++index) {
    result.push_back(HEX_DIGITS[(digest[index] >> 4u) & 0x0fu]);
    result.push_back(HEX_DIGITS[digest[index] & 0x0fu]);
  }

  return result;
}

auto fnv128(const std::string_view input, std::ostream &output) -> void {
  const auto result = fnv128(input);
  output.write(result.data(), static_cast<std::streamsize>(result.size()));
}

} // namespace sourcemeta::core
