#ifndef SOURCEMETA_CORE_NUMERIC_BIG_COEFFICIENT_H_
#define SOURCEMETA_CORE_NUMERIC_BIG_COEFFICIENT_H_

#include <algorithm> // std::max, std::copy, std::fill
#include <array>     // std::array
#include <cstdint>   // std::int32_t, std::int64_t, std::uint32_t,
                     // std::uint64_t, std::uintptr_t, std::uint8_t
#include <cstring>   // std::memcpy
#include <string>    // std::string, std::to_string
#include <utility>   // std::pair, std::move

#include <sourcemeta/core/numeric_uint128.h>

namespace {

constexpr std::uint8_t FLAG_SIGN = 0x01;
constexpr std::uint8_t FLAG_NAN = 0x02;
constexpr std::uint8_t FLAG_SNAN = 0x04;
constexpr std::uint8_t FLAG_INFINITE = 0x08;
constexpr std::uint8_t FLAG_BIG = 0x10;
constexpr std::uint8_t FLAG_HEAP = 0x20;

constexpr std::uint8_t SPECIAL_MASK =
    FLAG_NAN | FLAG_SNAN | FLAG_INFINITE | FLAG_BIG;

constexpr std::array<std::uint64_t, 20> POWERS_OF_10 = {{
    1ULL,                    // 10^0
    10ULL,                   // 10^1
    100ULL,                  // 10^2
    1000ULL,                 // 10^3
    10000ULL,                // 10^4
    100000ULL,               // 10^5
    1000000ULL,              // 10^6
    10000000ULL,             // 10^7
    100000000ULL,            // 10^8
    1000000000ULL,           // 10^9
    10000000000ULL,          // 10^10
    100000000000ULL,         // 10^11
    1000000000000ULL,        // 10^12
    10000000000000ULL,       // 10^13
    100000000000000ULL,      // 10^14
    1000000000000000ULL,     // 10^15
    10000000000000000ULL,    // 10^16
    100000000000000000ULL,   // 10^17
    1000000000000000000ULL,  // 10^18
    10000000000000000000ULL, // 10^19
}};

constexpr std::uint64_t BASE = 1000000000000000000ULL; // 10^18
constexpr std::int32_t BASE_DIGITS = 18;
constexpr std::int64_t COMPACT_MAX =
    static_cast<std::int64_t>(BASE - 1); // 999999999999999999

class BigCoefficient {
  static constexpr std::uint32_t INLINE_CAPACITY = 4;
  std::array<std::uint64_t, INLINE_CAPACITY> inline_words;

  [[nodiscard]] auto is_inline() const -> bool {
    return this->capacity <= INLINE_CAPACITY;
  }

public:
  std::uint64_t *words{nullptr};
  std::uint32_t length{0};
  std::uint32_t capacity;

  explicit BigCoefficient(std::uint32_t requested_capacity) {
    if (requested_capacity <= INLINE_CAPACITY) {
      this->words = this->inline_words.data();
      this->capacity = INLINE_CAPACITY;
      this->inline_words.fill(0);
    } else {
      this->words = new std::uint64_t[requested_capacity]();
      this->capacity = requested_capacity;
    }
  }

  ~BigCoefficient() {
    if (!this->is_inline()) {
      delete[] this->words;
    }
  }

  BigCoefficient(BigCoefficient &&other) noexcept
      : length{other.length}, capacity{other.capacity} {
    if (other.is_inline()) {
      std::copy(other.inline_words.data(),
                other.inline_words.data() + other.length,
                this->inline_words.data());
      this->words = this->inline_words.data();
    } else {
      this->words = other.words;
      other.words = other.inline_words.data();
    }

    other.length = 0;
    other.capacity = INLINE_CAPACITY;
  }

  auto operator=(BigCoefficient &&other) noexcept -> BigCoefficient & {
    if (this != &other) {
      if (!this->is_inline()) {
        delete[] this->words;
      }

      this->length = other.length;
      this->capacity = other.capacity;
      if (other.is_inline()) {
        std::copy(other.inline_words.data(),
                  other.inline_words.data() + other.length,
                  this->inline_words.data());
        this->words = this->inline_words.data();
      } else {
        this->words = other.words;
        other.words = other.inline_words.data();
      }

      other.length = 0;
      other.capacity = INLINE_CAPACITY;
    }

    return *this;
  }

  BigCoefficient(const BigCoefficient &) = delete;
  auto operator=(const BigCoefficient &) -> BigCoefficient & = delete;

  [[nodiscard]] auto clone() const -> BigCoefficient {
    BigCoefficient result{this->length};
    result.length = this->length;
    std::copy(this->words, this->words + this->length, result.words);
    return result;
  }

  auto trim() -> void {
    while (this->length > 1 && this->words[this->length - 1] == 0) {
      this->length--;
    }
  }

  [[nodiscard]] auto is_zero() const -> bool {
    return this->length == 0 || (this->length == 1 && this->words[0] == 0);
  }

  [[nodiscard]] auto digit_count() const -> std::uint64_t {
    if (this->is_zero()) {
      return 1;
    }

    auto top_word = this->words[this->length - 1];
    std::uint64_t top_digits = 0;
    while (top_word > 0) {
      top_word /= 10;
      top_digits++;
    }
    return top_digits +
           static_cast<std::uint64_t>(this->length - 1) * BASE_DIGITS;
  }

  [[nodiscard]] auto to_string() const -> std::string {
    if (this->is_zero()) {
      return "0";
    }

    std::string result;
    auto top = this->words[this->length - 1];
    result += std::to_string(top);

    for (auto index = this->length - 1; index > 0; index--) {
      auto word_string = std::to_string(this->words[index - 1]);
      result.append(static_cast<std::size_t>(BASE_DIGITS) - word_string.size(),
                    '0');
      result += word_string;
    }

    return result;
  }

  auto strip_trailing_zeros() -> std::int32_t {
    if (this->is_zero()) {
      return 0;
    }

    std::int32_t total_stripped = 0;

    std::uint32_t zero_words = 0;
    while (zero_words < this->length && this->words[zero_words] == 0) {
      zero_words++;
    }

    if (zero_words > 0 && zero_words < this->length) {
      std::copy(this->words + zero_words, this->words + this->length,
                this->words);
      this->length -= zero_words;
      total_stripped += static_cast<std::int32_t>(zero_words) * BASE_DIGITS;
    }

    if (this->words[0] != 0) {
      while (this->words[0] % 10 == 0) {
        this->words[0] /= 10;
        total_stripped++;
      }
    }

    return total_stripped;
  }

  [[nodiscard]] auto compare(const BigCoefficient &other) const -> int {
    if (this->length != other.length) {
      return this->length < other.length ? -1 : 1;
    }

    for (auto index = this->length; index > 0; index--) {
      if (this->words[index - 1] != other.words[index - 1]) {
        return this->words[index - 1] < other.words[index - 1] ? -1 : 1;
      }
    }

    return 0;
  }

  [[nodiscard]] auto add(const BigCoefficient &other) const -> BigCoefficient {
    auto max_length = std::max(this->length, other.length);
    BigCoefficient result{max_length + 1};
    std::uint64_t carry = 0;
    for (std::uint32_t index = 0; index < max_length; index++) {
      sourcemeta::core::uint128_t sum = carry;
      if (index < this->length) {
        sum += this->words[index];
      }

      if (index < other.length) {
        sum += other.words[index];
      }

      result.words[index] = static_cast<std::uint64_t>(sum % BASE);
      carry = static_cast<std::uint64_t>(sum / BASE);
    }

    result.length = max_length;
    if (carry > 0) {
      result.words[result.length] = carry;
      result.length++;
    }

    result.trim();
    return result;
  }

  [[nodiscard]] auto subtract(const BigCoefficient &other) const
      -> BigCoefficient {
    BigCoefficient result{this->length};
    std::int64_t borrow = 0;
    for (std::uint32_t index = 0; index < this->length; index++) {
      auto left_word = static_cast<std::int64_t>(this->words[index]);
      std::int64_t right_word = 0;
      if (index < other.length) {
        right_word = static_cast<std::int64_t>(other.words[index]);
      }

      auto difference = left_word - right_word - borrow;
      if (difference < 0) {
        difference += static_cast<std::int64_t>(BASE);
        borrow = 1;
      } else {
        borrow = 0;
      }

      result.words[index] = static_cast<std::uint64_t>(difference);
    }

    result.length = this->length;
    result.trim();
    return result;
  }

  [[nodiscard]] auto multiply(const BigCoefficient &other) const
      -> BigCoefficient {
    auto result_length = this->length + other.length;
    BigCoefficient result{result_length};
    result.length = result_length;
    std::fill(result.words, result.words + result_length, 0ULL);

    for (std::uint32_t index_left = 0; index_left < this->length;
         index_left++) {
      sourcemeta::core::uint128_t carry = 0;
      for (std::uint32_t index_right = 0; index_right < other.length;
           index_right++) {
        auto position = index_left + index_right;
        auto product =
            static_cast<sourcemeta::core::uint128_t>(this->words[index_left]) *
                other.words[index_right] +
            result.words[position] + carry;
        result.words[position] = static_cast<std::uint64_t>(product % BASE);
        carry = product / BASE;
      }

      if (carry > 0) {
        result.words[index_left + other.length] +=
            static_cast<std::uint64_t>(carry);
      }
    }

    result.trim();
    return result;
  }

  [[nodiscard]] auto multiply_pow10(std::uint32_t power) const
      -> BigCoefficient {
    if (power == 0) {
      return this->clone();
    }

    auto full_words = power / BASE_DIGITS;
    auto residual = power % BASE_DIGITS;
    BigCoefficient result{this->length + full_words + 1};

    std::fill(result.words, result.words + full_words, 0ULL);
    std::copy(this->words, this->words + this->length,
              result.words + full_words);
    result.length = this->length + full_words;

    if (residual > 0) {
      auto multiplier = POWERS_OF_10[static_cast<std::uint32_t>(residual)];
      sourcemeta::core::uint128_t carry = 0;
      for (std::uint32_t index = full_words; index < result.length; index++) {
        auto product =
            static_cast<sourcemeta::core::uint128_t>(result.words[index]) *
                multiplier +
            carry;
        result.words[index] = static_cast<std::uint64_t>(product % BASE);
        carry = product / BASE;
      }

      if (carry > 0) {
        result.words[result.length] = static_cast<std::uint64_t>(carry);
        result.length++;
      }
    }

    result.trim();
    return result;
  }

  [[nodiscard]] auto divide_modulo(const BigCoefficient &divisor) const
      -> std::pair<BigCoefficient, BigCoefficient> {
    auto compare_result = this->compare(divisor);
    if (compare_result < 0) {
      BigCoefficient quotient{1};
      quotient.words[0] = 0;
      quotient.length = 1;
      return {std::move(quotient), this->clone()};
    }

    if (compare_result == 0) {
      BigCoefficient quotient{1};
      quotient.words[0] = 1;
      quotient.length = 1;
      BigCoefficient remainder{1};
      remainder.words[0] = 0;
      remainder.length = 1;
      return {std::move(quotient), std::move(remainder)};
    }

    if (divisor.length == 1) {
      BigCoefficient quotient{this->length};
      quotient.length = this->length;
      sourcemeta::core::uint128_t remainder = 0;
      for (auto index = this->length; index > 0; index--) {
        auto current = remainder * BASE + this->words[index - 1];
        quotient.words[index - 1] =
            static_cast<std::uint64_t>(current / divisor.words[0]);
        remainder = current % divisor.words[0];
      }

      quotient.trim();
      BigCoefficient remainder_big{1};
      remainder_big.words[0] = static_cast<std::uint64_t>(remainder);
      remainder_big.length = 1;
      return {std::move(quotient), std::move(remainder_big)};
    }

    auto remainder = this->clone();
    BigCoefficient quotient{this->length};
    quotient.length = this->length;
    std::fill(quotient.words, quotient.words + quotient.length, 0ULL);

    while (remainder.compare(divisor) >= 0) {
      auto remainder_top = static_cast<sourcemeta::core::uint128_t>(
          remainder.words[remainder.length - 1]);
      if (remainder.length > divisor.length) {
        auto shift = remainder.length - divisor.length;
        auto divisor_top = divisor.words[divisor.length - 1];
        auto estimate =
            static_cast<std::uint64_t>(remainder_top / (divisor_top + 1));
        if (estimate == 0) {
          estimate = 1;
        }

        BigCoefficient estimate_big{1};
        estimate_big.words[0] = estimate;
        estimate_big.length = 1;

        auto scaled = estimate_big.multiply_pow10(shift * BASE_DIGITS);
        auto product = scaled.multiply(divisor);

        if (product.compare(remainder) > 0) {
          remainder = remainder.subtract(divisor);
          quotient.words[0]++;
        } else {
          remainder = remainder.subtract(product);
          BigCoefficient estimated_quotient{shift + 1};
          std::fill(estimated_quotient.words, estimated_quotient.words + shift,
                    0ULL);
          estimated_quotient.words[shift] = estimate;
          estimated_quotient.length = shift + 1;
          quotient = quotient.add(estimated_quotient);
        }

      } else {
        remainder = remainder.subtract(divisor);
        quotient.words[0]++;
      }
    }

    quotient.trim();
    remainder.trim();
    return {std::move(quotient), std::move(remainder)};
  }

  [[nodiscard]] static auto from_uint64(std::uint64_t value) -> BigCoefficient {
    if (value < BASE) {
      BigCoefficient result{1};
      result.words[0] = value;
      result.length = 1;
      return result;
    }

    BigCoefficient result{2};
    result.words[0] = value % BASE;
    result.words[1] = value / BASE;
    result.length = 2;
    return result;
  }

  [[nodiscard]] static auto from_digits(const char *digits, std::uint32_t count)
      -> BigCoefficient {
    auto word_count = (count + BASE_DIGITS - 1) / BASE_DIGITS;
    BigCoefficient result{word_count};
    result.length = word_count;

    for (std::uint32_t word_index = 0; word_index < word_count; word_index++) {
      std::uint64_t word = 0;
      auto end_position = count - word_index * BASE_DIGITS;
      auto start_position =
          end_position > static_cast<std::uint32_t>(BASE_DIGITS)
              ? end_position - static_cast<std::uint32_t>(BASE_DIGITS)
              : 0U;
      for (auto position = start_position; position < end_position;
           position++) {
        word = word * 10 + static_cast<std::uint64_t>(digits[position] - '0');
      }
      result.words[word_index] = word;
    }

    result.trim();
    return result;
  }

  [[nodiscard]] auto to_uint128(std::int32_t exponent) const
      -> sourcemeta::core::uint128_t {
    sourcemeta::core::uint128_t value = 0;
    for (auto index = this->length; index > 0; index--) {
      value = value * BASE + this->words[index - 1];
    }

    while (exponent > 0) {
      value *= 10;
      exponent--;
    }

    return value;
  }

  struct AddSignedResult;

  [[nodiscard]] static auto add_signed(const BigCoefficient &left,
                                       const BigCoefficient &right,
                                       bool left_negative, bool right_negative)
      -> AddSignedResult;

  static auto align_exponents(BigCoefficient &left, BigCoefficient &right,
                              std::int32_t left_exponent,
                              std::int32_t right_exponent) -> void;
};

struct BigCoefficient::AddSignedResult {
  BigCoefficient coefficient;
  bool is_negative;
};

auto BigCoefficient::add_signed(const BigCoefficient &left,
                                const BigCoefficient &right, bool left_negative,
                                bool right_negative)
    -> BigCoefficient::AddSignedResult {
  if (left_negative == right_negative) {
    return {.coefficient = left.add(right), .is_negative = left_negative};
  }

  auto comparison = left.compare(right);
  if (comparison >= 0) {
    return {.coefficient = left.subtract(right), .is_negative = left_negative};
  }

  return {.coefficient = right.subtract(left), .is_negative = right_negative};
}

auto BigCoefficient::align_exponents(BigCoefficient &left,
                                     BigCoefficient &right,
                                     std::int32_t left_exponent,
                                     std::int32_t right_exponent) -> void {
  if (left_exponent > right_exponent) {
    left = left.multiply_pow10(
        static_cast<std::uint32_t>(left_exponent - right_exponent));
  } else if (right_exponent > left_exponent) {
    right = right.multiply_pow10(
        static_cast<std::uint32_t>(right_exponent - left_exponent));
  }
}

// Lemire, "Computing the number of digits of an integer even faster" (2021)
auto digit_count(std::uint64_t value) -> std::uint32_t {
  if (value == 0) {
    return 1;
  }

#if defined(_MSC_VER)
  unsigned long bit_index;
  _BitScanReverse64(&bit_index, value);
  auto const bits = static_cast<std::uint32_t>(bit_index);
#else
  auto const bits = static_cast<std::uint32_t>(63 - __builtin_clzll(value));
#endif
  auto approximation = 1 + ((bits * 77) >> 8);
  if (approximation <= 19 && value >= POWERS_OF_10[approximation]) {
    approximation++;
  }

  return approximation;
}

auto store_big_pointer(std::int64_t &coefficient, BigCoefficient &&big)
    -> void {
  auto *heap_big = new BigCoefficient{std::move(big)};
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  auto as_integer = reinterpret_cast<std::uintptr_t>(heap_big);
  std::memcpy(&coefficient, &as_integer, sizeof(coefficient));
}

auto load_big_pointer(std::int64_t coefficient) -> BigCoefficient * {
  std::uintptr_t as_integer;
  std::memcpy(&as_integer, &coefficient, sizeof(as_integer));
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<BigCoefficient *>(as_integer);
}

auto coefficient_to_digit_string(std::int64_t coefficient,
                                 std::uint64_t coefficient_high,
                                 std::uint8_t flags) -> std::string {
  if (flags & FLAG_HEAP) {
    return load_big_pointer(coefficient)->to_string();
  }

  if (flags & FLAG_BIG) {
    auto high_string = std::to_string(coefficient_high);
    auto low_string = std::to_string(static_cast<std::uint64_t>(coefficient));
    std::string result = high_string;
    result.append(static_cast<std::size_t>(BASE_DIGITS) - low_string.size(),
                  '0');
    result += low_string;
    return result;
  }

  return std::to_string(coefficient);
}

auto modular_pow10(std::uint32_t exponent, std::uint64_t modulus)
    -> std::uint64_t {
  if (modulus == 1) {
    return 0;
  }

  if (exponent == 0) {
    return 1;
  }

  if (exponent <= 19) {
    return POWERS_OF_10[exponent] % modulus;
  }

  std::uint64_t result = 1;
  std::uint64_t base = 10 % modulus;
  auto remaining = exponent;
  while (remaining > 0) {
    if (remaining & 1) {
      result = static_cast<std::uint64_t>(
          static_cast<sourcemeta::core::uint128_t>(result) * base % modulus);
    }

    base = static_cast<std::uint64_t>(
        static_cast<sourcemeta::core::uint128_t>(base) * base % modulus);
    remaining >>= 1;
  }

  return result;
}

// Round-half-even (banker's rounding) to WORKING_PRECISION significant digits
constexpr std::int32_t WORKING_PRECISION = 16;

auto round_to_precision(std::int64_t &coefficient,
                        std::uint64_t &coefficient_high, std::int32_t &exponent,
                        std::uint8_t &flags) -> void {
  if (flags & (FLAG_NAN | FLAG_SNAN | FLAG_INFINITE)) {
    return;
  }

  if (flags & FLAG_BIG) {
    auto digit_string =
        coefficient_to_digit_string(coefficient, coefficient_high, flags);
    auto total_digits = static_cast<std::int32_t>(digit_string.size());
    if (total_digits <= WORKING_PRECISION) {
      return;
    }

    auto excess = total_digits - WORKING_PRECISION;

    auto kept =
        digit_string.substr(0, static_cast<std::size_t>(WORKING_PRECISION));
    auto dropped =
        digit_string.substr(static_cast<std::size_t>(WORKING_PRECISION));

    bool round_up = false;
    if (!dropped.empty()) {
      if (dropped[0] > '5') {
        round_up = true;
      } else if (dropped[0] == '5') {
        bool has_trailing = false;
        for (std::size_t index = 1; index < dropped.size(); index++) {
          if (dropped[index] != '0') {
            has_trailing = true;
            break;
          }
        }

        if (has_trailing) {
          round_up = true;
        } else {
          round_up = (kept.back() - '0') % 2 != 0;
        }
      }
    }

    if (flags & FLAG_HEAP) {
      delete load_big_pointer(coefficient);
    }

    std::int64_t new_coefficient = 0;
    for (auto character : kept) {
      new_coefficient = new_coefficient * 10 + (character - '0');
    }
    if (round_up) {
      new_coefficient++;
    }

    coefficient = new_coefficient;
    coefficient_high = 0;
    exponent += excess;
    flags = static_cast<std::uint8_t>(flags & ~(FLAG_BIG | FLAG_HEAP));
    return;
  }

  auto digits = digit_count(static_cast<std::uint64_t>(coefficient));
  if (static_cast<std::int32_t>(digits) <= WORKING_PRECISION) {
    return;
  }

  auto excess = static_cast<std::int32_t>(digits) - WORKING_PRECISION;
  std::int64_t divisor = 1;
  for (std::int32_t index = 0; index < excess; index++) {
    divisor *= 10;
  }

  auto quotient = coefficient / divisor;
  auto remainder = coefficient % divisor;
  auto half = divisor / 2;

  if (remainder > half || (remainder == half && quotient % 2 != 0)) {
    quotient++;
  }

  coefficient = quotient;
  coefficient_high = 0;
  exponent += excess;
}

void free_big_coefficient(std::int64_t coefficient, std::uint8_t flags) {
  if (flags & FLAG_HEAP) {
    delete load_big_pointer(coefficient);
  }
}

auto coefficient_as_big(std::int64_t coefficient,
                        std::uint64_t coefficient_high, std::uint8_t flags)
    -> BigCoefficient {
  if (flags & FLAG_HEAP) {
    return load_big_pointer(coefficient)->clone();
  }

  if (flags & FLAG_BIG) {
    BigCoefficient result{2};
    result.words[0] = static_cast<std::uint64_t>(coefficient);
    result.words[1] = coefficient_high;
    result.length = coefficient_high > 0 ? 2 : 1;
    return result;
  }

  return BigCoefficient::from_uint64(static_cast<std::uint64_t>(coefficient));
}

void store_big_result(std::int64_t &coefficient,
                      std::uint64_t &coefficient_high, std::uint8_t &flags,
                      BigCoefficient result_big, bool result_negative) {
  if (result_big.length <= 1 &&
      result_big.words[0] <= static_cast<std::uint64_t>(COMPACT_MAX)) {
    coefficient = static_cast<std::int64_t>(result_big.words[0]);
    coefficient_high = 0;
    flags = result_negative ? FLAG_SIGN : 0;
  } else if (result_big.length <= 2) {
    coefficient = static_cast<std::int64_t>(result_big.words[0]);
    coefficient_high = result_big.length > 1 ? result_big.words[1] : 0;
    flags =
        static_cast<std::uint8_t>(FLAG_BIG | (result_negative ? FLAG_SIGN : 0));
  } else {
    store_big_pointer(coefficient, std::move(result_big));
    coefficient_high = 0;
    flags = static_cast<std::uint8_t>(FLAG_BIG | FLAG_HEAP |
                                      (result_negative ? FLAG_SIGN : 0));
  }
}

} // namespace

#endif
