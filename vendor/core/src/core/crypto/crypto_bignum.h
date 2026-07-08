#ifndef SOURCEMETA_CORE_CRYPTO_BIGNUM_H_
#define SOURCEMETA_CORE_CRYPTO_BIGNUM_H_

// Fixed-capacity unsigned big integer arithmetic for the reference signature
// backend. Capacity fits 4096-bit RSA operands and their double-width products.
// The verification paths consume only public inputs and stay variable time; the
// signing paths use the constant-time layer below (fixed-width multiply,
// Barrett reduction, masked select and inverse) on their secret operands. Only
// the Barrett context precompute stays variable time, and it touches the public
// modulus alone

#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/text.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <optional>    // std::optional
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

// Overwrite the whole word buffer of a big integer that held secret material,
// including the words past the current size that intermediate operations wrote,
// so it does not linger in freed memory. The volatile access stops the compiler
// from eliding the write as a dead store
inline auto secure_zero(Bignum &value) noexcept -> void {
  auto *pointer{reinterpret_cast<volatile unsigned char *>(value.words.data())};
  for (std::size_t index{0}; index < sizeof(value.words); index += 1) {
    pointer[index] = 0;
  }

  value.size = 0;
}

// Overwrite the referenced big integer when leaving the current scope, so a
// secret value a local holds is wiped across every return path without
// threading a manual call through each one
struct SecureBignumScope {
  explicit SecureBignumScope(Bignum &value) noexcept : target{value} {}
  SecureBignumScope(const SecureBignumScope &) = delete;
  auto operator=(const SecureBignumScope &) -> SecureBignumScope & = delete;
  SecureBignumScope(SecureBignumScope &&) = delete;
  auto operator=(SecureBignumScope &&) -> SecureBignumScope & = delete;
  ~SecureBignumScope() { secure_zero(this->target); }
  Bignum &target;
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
  // An odd length is decoded as if a zero nibble had been prepended
  const auto bytes{hex_to_bytes(hex, true)};
  if (!bytes.has_value()) {
    return Bignum{};
  }

  return bignum_from_bytes(bytes.value());
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

// The same read without the size-dependent early return, so the signing ladders
// do not reveal the secret scalar's length through the branch. The bit index is
// bounded by the public curve size, so the guard is on public data
inline auto bignum_get_bit_fixed(const Bignum &value,
                                 const std::size_t bit) noexcept -> bool {
  const auto word{bit / 64};
  return word < Bignum::capacity &&
         ((value.words[word] >> (bit % 64)) & 1u) != 0;
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

// A branch-free select, so a secret condition does not steer control flow. The
// whole capacity is blended so the running size does not leak the condition
inline auto bignum_conditional_select(const bool condition,
                                      const Bignum &when_true,
                                      const Bignum &when_false) noexcept
    -> Bignum {
  const std::uint64_t mask{std::uint64_t{0} -
                           static_cast<std::uint64_t>(condition)};
  Bignum result;
  for (std::size_t index = 0; index < Bignum::capacity; ++index) {
    result.words[index] =
        (when_true.words[index] & mask) | (when_false.words[index] & ~mask);
  }

  const std::size_t size_mask{std::size_t{0} -
                              static_cast<std::size_t>(condition)};
  result.size = (when_true.size & size_mask) | (when_false.size & ~size_mask);
  return result;
}

// Fixed-width subtraction over the given number of words, returning the final
// borrow. It always visits every word, so its timing does not depend on where
// the operands' significant words fall the way the size-driven routines above
// do
inline auto bignum_subtract_fixed(const Bignum &left, const Bignum &right,
                                  const std::size_t words, Bignum &out) noexcept
    -> std::uint64_t {
  std::uint64_t borrow{0};
  for (std::size_t index = 0; index < words; ++index) {
    const auto left_word{left.words[index]};
    const auto right_word{right.words[index]};
    const auto without_right{left_word - right_word};
    const std::uint64_t borrow_from_right{left_word < right_word ? 1u : 0u};
    const auto result_word{without_right - borrow};
    const std::uint64_t borrow_from_previous{without_right < borrow ? 1u : 0u};
    out.words[index] = result_word;
    borrow = borrow_from_right | borrow_from_previous;
  }

  out.size = words;
  return borrow;
}

// Multiply visiting exactly the given word counts, so timing does not reveal
// where either operand's significant words fall
inline auto bignum_multiply_fixed(const Bignum &left, const Bignum &right,
                                  const std::size_t left_words,
                                  const std::size_t right_words) noexcept
    -> Bignum {
  Bignum result;
  for (std::size_t left_index = 0; left_index < left_words; ++left_index) {
    std::uint64_t carry{0};
    for (std::size_t right_index = 0; right_index < right_words;
         ++right_index) {
      const auto destination{left_index + right_index};
      const auto product{static_cast<BignumDoubleWord>(left.words[left_index]) *
                             right.words[right_index] +
                         result.words[destination] + carry};
      result.words[destination] = static_cast<std::uint64_t>(product);
      carry = static_cast<std::uint64_t>(product >> 64u);
    }

    result.words[left_index + right_words] = carry;
  }

  result.size = left_words + right_words;
  return result;
}

// A word-granular right shift dropping the low words, and its counterpart that
// keeps them, for the word-aligned truncations the Barrett reduction needs
inline auto bignum_drop_low_words(const Bignum &value,
                                  const std::size_t words) noexcept -> Bignum {
  Bignum result;
  for (std::size_t index = 0; index + words < Bignum::capacity; ++index) {
    result.words[index] = value.words[index + words];
  }

  result.size = value.size > words ? value.size - words : 0;
  return result;
}

inline auto bignum_keep_low_words(const Bignum &value,
                                  const std::size_t words) noexcept -> Bignum {
  Bignum result;
  for (std::size_t index = 0; index < words; ++index) {
    result.words[index] = value.words[index];
  }

  result.size = words;
  return result;
}

// Quotient of a division, for building a public reduction constant only, so a
// plain bit-by-bit long division is enough
inline auto bignum_divide(const Bignum &numerator,
                          const Bignum &denominator) noexcept -> Bignum {
  Bignum quotient;
  Bignum remainder;
  const auto bits{bignum_bit_length(numerator)};
  for (std::size_t index = bits; index > 0; --index) {
    remainder = bignum_shift_left(remainder, 1);
    if (bignum_get_bit(numerator, index - 1)) {
      remainder.words[0] |= 1u;
      if (remainder.size == 0) {
        remainder.size = 1;
      }
    }

    if (bignum_compare(remainder, denominator) >= 0) {
      bignum_subtract_in_place(remainder, denominator);
      quotient.words[(index - 1) / 64] |= std::uint64_t{1}
                                          << ((index - 1) % 64);
      if (quotient.size < ((index - 1) / 64) + 1) {
        quotient.size = ((index - 1) / 64) + 1;
      }
    }
  }

  return quotient;
}

// Precomputed constants for Barrett reduction modulo a fixed modulus. Built
// from the public modulus alone, so the setup itself need not be constant time
struct BarrettContext {
  Bignum modulus;
  std::size_t words;
  Bignum factor;
};

inline auto barrett_context(const Bignum &modulus) noexcept -> BarrettContext {
  BarrettContext context;
  context.modulus = modulus;
  context.words = modulus.size;
  const auto power{bignum_shift_left(bignum_from_u64(1), 128 * context.words)};
  context.factor = bignum_divide(power, modulus);
  return context;
}

// If the value is at least the modulus over the given width, subtract it. The
// choice is a masked select rather than a branch
inline auto bignum_conditional_subtract(const Bignum &value,
                                        const Bignum &modulus,
                                        const std::size_t words) noexcept
    -> Bignum {
  Bignum reduced;
  const auto borrow{bignum_subtract_fixed(value, modulus, words, reduced)};
  return bignum_conditional_select(borrow == 0, reduced, value);
}

// Reduce a value below the square of the modulus down to the modulus in
// constant time (Handbook of Applied Cryptography Algorithm 14.42). The
// estimate is at most two too small, so three masked subtractions always finish
// the reduction
inline auto barrett_reduce(const Bignum &value,
                           const BarrettContext &context) noexcept -> Bignum {
  const auto width{context.words};
  const auto high{bignum_drop_low_words(value, width - 1)};
  const auto estimate{
      bignum_multiply_fixed(high, context.factor, width + 1, width + 1)};
  const auto quotient{bignum_drop_low_words(estimate, width + 1)};
  const auto value_low{bignum_keep_low_words(value, width + 1)};
  const auto product{
      bignum_multiply_fixed(quotient, context.modulus, width + 1, width)};
  const auto product_low{bignum_keep_low_words(product, width + 1)};
  Bignum remainder;
  bignum_subtract_fixed(value_low, product_low, width + 1, remainder);
  remainder =
      bignum_conditional_subtract(remainder, context.modulus, width + 1);
  remainder =
      bignum_conditional_subtract(remainder, context.modulus, width + 1);
  remainder =
      bignum_conditional_subtract(remainder, context.modulus, width + 1);
  remainder.size = width;
  return remainder;
}

inline auto field_mod_multiply_ct(const Bignum &left, const Bignum &right,
                                  const BarrettContext &context) noexcept
    -> Bignum {
  return barrett_reduce(
      bignum_multiply_fixed(left, right, context.words, context.words),
      context);
}

inline auto field_add_ct(const Bignum &left, const Bignum &right,
                         const BarrettContext &context) noexcept -> Bignum {
  const auto width{context.words};
  Bignum sum;
  std::uint64_t carry{0};
  for (std::size_t index = 0; index < width; ++index) {
    const auto total{static_cast<BignumDoubleWord>(left.words[index]) +
                     right.words[index] + carry};
    sum.words[index] = static_cast<std::uint64_t>(total);
    carry = static_cast<std::uint64_t>(total >> 64u);
  }

  sum.words[width] = carry;
  sum.size = width + 1;
  auto reduced{bignum_conditional_subtract(sum, context.modulus, width + 1)};
  reduced.size = width;
  return reduced;
}

inline auto field_subtract_ct(const Bignum &left, const Bignum &right,
                              const BarrettContext &context) noexcept
    -> Bignum {
  const auto width{context.words};
  Bignum difference;
  const auto borrow{bignum_subtract_fixed(left, right, width, difference)};
  Bignum wrapped;
  std::uint64_t carry{0};
  for (std::size_t index = 0; index < width; ++index) {
    const auto total{static_cast<BignumDoubleWord>(difference.words[index]) +
                     context.modulus.words[index] + carry};
    wrapped.words[index] = static_cast<std::uint64_t>(total);
    carry = static_cast<std::uint64_t>(total >> 64u);
  }

  wrapped.size = width;
  auto result{bignum_conditional_select(borrow != 0, wrapped, difference)};
  result.size = width;
  return result;
}

// Fermat inverse over the field in constant time. The exponent is the public
// modulus minus two, so its bit pattern reveals nothing secret, and the field
// multiplications underneath do not depend on the value being inverted. The
// modulus must be prime
inline auto field_inverse_ct(const Bignum &value,
                             const BarrettContext &context) noexcept -> Bignum {
  auto exponent{context.modulus};
  bignum_subtract_in_place(exponent, bignum_from_u64(2));
  Bignum result;
  result.words[0] = 1;
  result.size = context.words;
  const auto base{barrett_reduce(value, context)};
  const auto exponent_bits{bignum_bit_length(exponent)};
  for (std::size_t index = exponent_bits; index > 0; --index) {
    result = field_mod_multiply_ct(result, result, context);
    if (bignum_get_bit(exponent, index - 1)) {
      result = field_mod_multiply_ct(result, base, context);
    }
  }

  return result;
}

// Modular exponentiation for a secret exponent (the RSA private key), in
// constant time. Unlike the inverse above, the exponent is secret, so the
// ladder runs a fixed number of steps fixed by the public modulus and blends
// the per-bit multiply with a masked select rather than a branch. The modulus
// need not be prime
inline auto bignum_mod_exp_ct(const Bignum &base, const Bignum &exponent,
                              const BarrettContext &context) noexcept
    -> Bignum {
  Bignum result;
  result.words[0] = 1;
  result.size = context.words;
  const auto reduced_base{barrett_reduce(base, context)};
  const auto exponent_bits{bignum_bit_length(context.modulus)};
  for (std::size_t index = exponent_bits; index > 0; --index) {
    result = field_mod_multiply_ct(result, result, context);
    const auto product{field_mod_multiply_ct(result, reduced_base, context)};
    result = bignum_conditional_select(
        bignum_get_bit_fixed(exponent, index - 1), product, result);
  }

  return result;
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
