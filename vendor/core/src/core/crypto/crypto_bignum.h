#ifndef SOURCEMETA_CORE_CRYPTO_BIGNUM_H_
#define SOURCEMETA_CORE_CRYPTO_BIGNUM_H_

// Fixed-capacity unsigned big integer arithmetic for the reference
// signature verification backend. Capacity fits 4096-bit RSA operands
// and their double-width products. Constant-time execution is not
// required, since verification consumes only public inputs

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

inline auto bignum_from_hex(const std::string_view hex) -> Bignum {
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
  const auto word{bit / 64};
  if (word >= value.size) {
    return false;
  }

  return ((value.words[word] >> (bit % 64)) & 1u) != 0;
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

inline auto bignum_shift_right(const Bignum &value,
                               const std::size_t bits) noexcept -> Bignum;

// Reduce a value modulo the modulus with Knuth's Algorithm D (TAOCP Volume 2,
// Section 4.3.1), the schoolbook long division that estimates one quotient
// word per step rather than one bit, so the cost is quadratic in the number of
// words rather than the number of bits
inline auto bignum_reduce(Bignum &value, const Bignum &modulus) noexcept
    -> void {
  if (bignum_compare(value, modulus) < 0) {
    return;
  }

  const auto divisor_words{modulus.size};

  // A single-word divisor folds the value down word by word
  if (divisor_words == 1) {
    const auto divisor{modulus.words[0]};
    BignumDoubleWord remainder{0};
    for (std::size_t index = value.size; index > 0; --index) {
      remainder = (remainder << 64u) | value.words[index - 1];
      remainder %= divisor;
    }

    value = bignum_from_u64(static_cast<std::uint64_t>(remainder));
    return;
  }

  // Normalize so the divisor's top word has its high bit set, which bounds the
  // error of each quotient word estimate to at most two
  const auto shift{static_cast<std::size_t>(
      (64u - (bignum_bit_length(modulus) % 64u)) % 64u)};
  const auto divisor{shift > 0 ? bignum_shift_left(modulus, shift) : modulus};
  auto dividend{shift > 0 ? bignum_shift_left(value, shift) : value};
  const auto dividend_words{dividend.size};
  const auto quotient_words{dividend_words - divisor_words};
  const auto top{divisor.words[divisor_words - 1]};
  const auto next{divisor.words[divisor_words - 2]};
  const BignumDoubleWord base{static_cast<BignumDoubleWord>(1) << 64u};

  for (std::size_t step = quotient_words + 1; step > 0; --step) {
    const auto offset{step - 1};

    // Estimate the quotient word from the top two words of the running value
    const auto numerator{
        (static_cast<BignumDoubleWord>(dividend.words[offset + divisor_words])
         << 64u) |
        dividend.words[offset + divisor_words - 1]};
    auto estimate{numerator / top};
    auto estimate_remainder{numerator % top};
    while (estimate >= base ||
           estimate * next > (estimate_remainder << 64u) +
                                 dividend.words[offset + divisor_words - 2]) {
      estimate -= 1;
      estimate_remainder += top;
      if (estimate_remainder >= base) {
        break;
      }
    }

    // Multiply the divisor by the estimate and subtract from the running value
    const auto quotient_word{static_cast<std::uint64_t>(estimate)};
    BignumDoubleWord carry{0};
    std::uint64_t borrow{0};
    for (std::size_t index = 0; index < divisor_words; ++index) {
      const auto product{static_cast<BignumDoubleWord>(quotient_word) *
                             divisor.words[index] +
                         carry};
      carry = product >> 64u;
      const auto subtrahend{static_cast<std::uint64_t>(product)};
      const auto current{dividend.words[offset + index]};
      const auto without_subtrahend{current - subtrahend};
      auto next_borrow{current < subtrahend ? 1u : 0u};
      const auto result_word{without_subtrahend - borrow};
      if (without_subtrahend < borrow) {
        next_borrow += 1u;
      }

      dividend.words[offset + index] = result_word;
      borrow = next_borrow;
    }

    const auto current{dividend.words[offset + divisor_words]};
    const auto subtrahend{static_cast<std::uint64_t>(carry)};
    const auto without_subtrahend{current - subtrahend};
    auto next_borrow{current < subtrahend ? 1u : 0u};
    dividend.words[offset + divisor_words] = without_subtrahend - borrow;
    if (without_subtrahend < borrow) {
      next_borrow += 1u;
    }

    // The estimate was at most one too large, so add the divisor back when the
    // subtraction borrowed past the top
    if (next_borrow != 0) {
      BignumDoubleWord add_carry{0};
      for (std::size_t index = 0; index < divisor_words; ++index) {
        const auto sum{
            static_cast<BignumDoubleWord>(dividend.words[offset + index]) +
            divisor.words[index] + add_carry};
        dividend.words[offset + index] = static_cast<std::uint64_t>(sum);
        add_carry = sum >> 64u;
      }

      dividend.words[offset + divisor_words] +=
          static_cast<std::uint64_t>(add_carry);
    }
  }

  // The remainder occupies the low words, still scaled by the normalization
  dividend.size = divisor_words;
  bignum_normalize(dividend);
  value = shift > 0 ? bignum_shift_right(dividend, shift) : dividend;
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

// Halve a value modulo an odd modulus: an even value shifts down, an odd one
// becomes even by adding the modulus first, so the result stays an integer
inline auto bignum_mod_halve(const Bignum &value,
                             const Bignum &modulus) noexcept -> Bignum {
  if ((value.words[0] & 1u) == 0) {
    return bignum_shift_right(value, 1);
  }

  return bignum_shift_right(bignum_add(value, modulus), 1);
}

// Modular inverse by the binary extended Euclidean algorithm, which needs only
// halving, subtraction, and comparison rather than the modular exponentiation
// a Fermat inverse over a prime modulus would spend. The modulus must be odd.
// Returns zero when the value has no inverse, which is when it shares a factor
// with the modulus or reduces to zero
inline auto bignum_mod_inverse(const Bignum &value,
                               const Bignum &modulus) noexcept -> Bignum {
  const auto one{bignum_from_u64(1)};
  auto first{value};
  bignum_reduce(first, modulus);
  auto second{modulus};
  auto first_coefficient{one};
  Bignum second_coefficient;

  while (bignum_compare(first, one) != 0 && bignum_compare(second, one) != 0) {
    // A side reaching zero means the greatest common divisor exceeds one, so no
    // inverse exists. Stopping here also keeps the halving below from spinning
    // forever on a zero value
    if (bignum_is_zero(first) || bignum_is_zero(second)) {
      return {};
    }

    while ((first.words[0] & 1u) == 0) {
      first = bignum_shift_right(first, 1);
      first_coefficient = bignum_mod_halve(first_coefficient, modulus);
    }

    while ((second.words[0] & 1u) == 0) {
      second = bignum_shift_right(second, 1);
      second_coefficient = bignum_mod_halve(second_coefficient, modulus);
    }

    if (bignum_compare(first, second) >= 0) {
      bignum_subtract_in_place(first, second);
      first_coefficient =
          bignum_mod_subtract(first_coefficient, second_coefficient, modulus);
    } else {
      bignum_subtract_in_place(second, first);
      second_coefficient =
          bignum_mod_subtract(second_coefficient, first_coefficient, modulus);
    }
  }

  return bignum_compare(first, one) == 0 ? first_coefficient
                                         : second_coefficient;
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
