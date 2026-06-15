#ifndef SOURCEMETA_CORE_CRYPTO_BIGNUM_H_
#define SOURCEMETA_CORE_CRYPTO_BIGNUM_H_

// Fixed-capacity unsigned big integer arithmetic for the reference
// signature verification backend. Capacity fits 4096-bit RSA operands
// and their double-width products. Performance and constant-time
// execution are non-goals, verification consumes only public inputs

#include <sourcemeta/core/numeric.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

using BignumDoubleWord = uint128_t;

struct Bignum {
  // Enough words for an 8192-bit product plus shifting headroom
  static constexpr std::size_t capacity{130};
  std::array<std::uint64_t, capacity> words{};
  std::size_t size{0};
};

inline auto bignum_normalize(Bignum &value) noexcept -> void {
  while (value.size > 0 && value.words[value.size - 1] == 0) {
    value.size -= 1;
  }
}

inline auto bignum_from_bytes(const std::string_view input) noexcept -> Bignum {
  Bignum result;
  std::size_t bytes_consumed{0};
  for (std::size_t index = input.size(); index > 0; --index) {
    const auto byte{static_cast<std::uint8_t>(input[index - 1])};
    const auto word_index{bytes_consumed / 8};
    if (word_index >= Bignum::capacity) {
      break;
    }

    result.words[word_index] |= static_cast<std::uint64_t>(byte)
                                << (8 * (bytes_consumed % 8));
    bytes_consumed += 1;
  }

  result.size = (bytes_consumed + 7) / 8;
  bignum_normalize(result);
  return result;
}

inline auto bignum_from_u64(const std::uint64_t value) noexcept -> Bignum {
  Bignum result;
  if (value > 0) {
    result.words[0] = value;
    result.size = 1;
  }

  return result;
}

inline auto bignum_from_hex(const std::string_view hex) noexcept -> Bignum {
  const auto nibble{[](const char character) noexcept -> std::uint8_t {
    if (character >= '0' && character <= '9') {
      return static_cast<std::uint8_t>(character - '0');
    } else if (character >= 'a' && character <= 'f') {
      return static_cast<std::uint8_t>(character - 'a' + 10);
    } else {
      return static_cast<std::uint8_t>(character - 'A' + 10);
    }
  }};

  std::string bytes;
  bytes.reserve((hex.size() + 1) / 2);

  // An odd length means the leading nibble forms a byte on its own, as if a
  // zero had been prepended
  std::size_t index{0};
  if (hex.size() % 2 != 0) {
    bytes.push_back(static_cast<char>(nibble(hex[0])));
    index = 1;
  }

  for (; index + 1 < hex.size(); index += 2) {
    bytes.push_back(
        static_cast<char>((nibble(hex[index]) << 4u) | nibble(hex[index + 1])));
  }

  return bignum_from_bytes(bytes);
}

inline auto bignum_is_zero(const Bignum &value) noexcept -> bool {
  return value.size == 0;
}

inline auto bignum_compare(const Bignum &left, const Bignum &right) noexcept
    -> int {
  if (left.size != right.size) {
    return left.size < right.size ? -1 : 1;
  }

  for (std::size_t index = left.size; index > 0; --index) {
    if (left.words[index - 1] != right.words[index - 1]) {
      return left.words[index - 1] < right.words[index - 1] ? -1 : 1;
    }
  }

  return 0;
}

inline auto bignum_bit_length(const Bignum &value) noexcept -> std::size_t {
  if (value.size == 0) {
    return 0;
  }

  auto top_word{value.words[value.size - 1]};
  std::size_t top_bits{0};
  while (top_word > 0) {
    top_word >>= 1u;
    top_bits += 1;
  }

  return ((value.size - 1) * 64) + top_bits;
}

inline auto bignum_get_bit(const Bignum &value, const std::size_t bit) noexcept
    -> bool {
  return ((value.words[bit / 64] >> (bit % 64)) & 1u) != 0;
}

// Assumes the result fits in the capacity
inline auto bignum_shift_left(const Bignum &value,
                              const std::size_t bits) noexcept -> Bignum {
  Bignum result;
  const auto word_shift{bits / 64};
  const auto bit_shift{bits % 64};
  result.size = value.size + word_shift + 1;
  if (result.size > Bignum::capacity) {
    result.size = Bignum::capacity;
  }

  for (std::size_t index = 0; index < value.size; ++index) {
    const auto destination{index + word_shift};
    if (destination >= Bignum::capacity) {
      break;
    }

    result.words[destination] |= value.words[index] << bit_shift;
    if (bit_shift > 0 && destination + 1 < Bignum::capacity) {
      result.words[destination + 1] |= value.words[index] >> (64u - bit_shift);
    }
  }

  bignum_normalize(result);
  return result;
}

// Assumes the left operand is greater than or equal to the right one
inline auto bignum_subtract_in_place(Bignum &left, const Bignum &right) noexcept
    -> void {
  std::uint64_t borrow{0};
  for (std::size_t index = 0; index < left.size; ++index) {
    const auto subtrahend{index < right.size ? right.words[index] : 0};
    const auto previous{left.words[index]};
    left.words[index] = previous - subtrahend - borrow;
    borrow = (previous < subtrahend || (borrow == 1 && previous == subtrahend))
                 ? 1
                 : 0;
  }

  bignum_normalize(left);
}

inline auto bignum_reduce(Bignum &value, const Bignum &modulus) noexcept
    -> void {
  const auto modulus_bits{bignum_bit_length(modulus)};
  while (bignum_compare(value, modulus) >= 0) {
    const auto value_bits{bignum_bit_length(value)};
    auto shift{value_bits - modulus_bits};
    auto shifted{bignum_shift_left(modulus, shift)};
    if (bignum_compare(shifted, value) > 0) {
      shift -= 1;
      shifted = bignum_shift_left(modulus, shift);
    }

    bignum_subtract_in_place(value, shifted);
  }
}

// Assumes both operands fit in half the capacity
inline auto bignum_multiply(const Bignum &left, const Bignum &right) noexcept
    -> Bignum {
  Bignum result;
  result.size = left.size + right.size;
  if (result.size > Bignum::capacity) {
    result.size = Bignum::capacity;
  }

  for (std::size_t left_index = 0; left_index < left.size; ++left_index) {
    std::uint64_t carry{0};
    for (std::size_t right_index = 0; right_index < right.size; ++right_index) {
      const auto destination{left_index + right_index};
      if (destination >= Bignum::capacity) {
        break;
      }

      const auto product{static_cast<BignumDoubleWord>(left.words[left_index]) *
                             right.words[right_index] +
                         result.words[destination] + carry};
      result.words[destination] = static_cast<std::uint64_t>(product);
      carry = static_cast<std::uint64_t>(product >> 64u);
    }

    const auto carry_destination{left_index + right.size};
    if (carry_destination < Bignum::capacity) {
      result.words[carry_destination] += carry;
    }
  }

  bignum_normalize(result);
  return result;
}

inline auto bignum_mod_exp(const Bignum &base, const Bignum &exponent,
                           const Bignum &modulus) noexcept -> Bignum {
  Bignum result;
  result.words[0] = 1;
  result.size = 1;

  auto reduced_base{base};
  bignum_reduce(reduced_base, modulus);

  const auto exponent_bits{bignum_bit_length(exponent)};
  for (std::size_t index = exponent_bits; index > 0; --index) {
    result = bignum_multiply(result, result);
    bignum_reduce(result, modulus);
    if (bignum_get_bit(exponent, index - 1)) {
      result = bignum_multiply(result, reduced_base);
      bignum_reduce(result, modulus);
    }
  }

  return result;
}

inline auto bignum_add(const Bignum &left, const Bignum &right) noexcept
    -> Bignum {
  Bignum result;
  const auto larger{left.size > right.size ? left.size : right.size};
  std::uint64_t carry{0};
  for (std::size_t index = 0; index < larger; ++index) {
    const auto first{index < left.size ? left.words[index] : 0};
    const auto second{index < right.size ? right.words[index] : 0};
    const auto sum{static_cast<BignumDoubleWord>(first) + second + carry};
    result.words[index] = static_cast<std::uint64_t>(sum);
    carry = static_cast<std::uint64_t>(sum >> 64u);
  }

  result.size = larger;
  if (carry > 0 && larger < Bignum::capacity) {
    result.words[larger] = carry;
    result.size = larger + 1;
  }

  bignum_normalize(result);
  return result;
}

inline auto bignum_shift_right(const Bignum &value,
                               const std::size_t bits) noexcept -> Bignum {
  Bignum result;
  const auto word_shift{bits / 64};
  const auto bit_shift{bits % 64};
  if (word_shift >= value.size) {
    return result;
  }

  result.size = value.size - word_shift;
  for (std::size_t index = 0; index < result.size; ++index) {
    auto word{value.words[index + word_shift] >> bit_shift};
    if (bit_shift > 0 && index + word_shift + 1 < value.size) {
      word |= value.words[index + word_shift + 1] << (64u - bit_shift);
    }

    result.words[index] = word;
  }

  bignum_normalize(result);
  return result;
}

// All modular helpers below assume their operands are already reduced to
// less than the modulus, as the elliptic curve routines guarantee

inline auto bignum_mod_add(const Bignum &left, const Bignum &right,
                           const Bignum &modulus) noexcept -> Bignum {
  auto result{bignum_add(left, right)};
  if (bignum_compare(result, modulus) >= 0) {
    bignum_subtract_in_place(result, modulus);
  }

  return result;
}

inline auto bignum_mod_subtract(const Bignum &left, const Bignum &right,
                                const Bignum &modulus) noexcept -> Bignum {
  if (bignum_compare(left, right) >= 0) {
    auto result{left};
    bignum_subtract_in_place(result, right);
    return result;
  }

  auto result{bignum_add(left, modulus)};
  bignum_subtract_in_place(result, right);
  return result;
}

inline auto bignum_mod_multiply(const Bignum &left, const Bignum &right,
                                const Bignum &modulus) noexcept -> Bignum {
  auto result{bignum_multiply(left, right)};
  bignum_reduce(result, modulus);
  return result;
}

// The modulus must be prime, so that Fermat's little theorem gives the
// inverse as the modulus-minus-two power
inline auto bignum_mod_inverse(const Bignum &value,
                               const Bignum &modulus) noexcept -> Bignum {
  auto exponent{modulus};
  bignum_subtract_in_place(exponent, bignum_from_u64(2));
  return bignum_mod_exp(value, exponent, modulus);
}

inline auto bignum_to_bytes(const Bignum &value, const std::size_t length)
    -> std::string {
  std::string result(length, '\x00');
  for (std::size_t index = 0; index < length; ++index) {
    const auto word_index{index / 8};
    if (word_index >= value.size) {
      break;
    }

    const auto byte{static_cast<std::uint8_t>(
        (value.words[word_index] >> (8 * (index % 8))) & 0xffu)};
    result[length - 1 - index] = static_cast<char>(byte);
  }

  return result;
}

} // namespace sourcemeta::core

#endif
