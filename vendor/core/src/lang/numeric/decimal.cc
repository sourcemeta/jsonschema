#include <sourcemeta/core/numeric_decimal.h>
#include <sourcemeta/core/numeric_error.h>

#include "big_coefficient.h"

#include <array>     // std::array
#include <cassert>   // assert
#include <cmath>     // std::isfinite
#include <cstring>   // std::strlen
#include <iomanip>   // std::setprecision
#include <limits>    // std::numeric_limits
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::out_of_range
#include <string>    // std::string, std::stof, std::stod

namespace {

auto strip_trailing_zeros(std::int64_t &coefficient, std::int32_t &exponent)
    -> void {
  if (coefficient == 0) {
    return;
  }

  constexpr std::int64_t POWER_OF_10_16 = 10000000000000000LL;
  if (coefficient % POWER_OF_10_16 == 0) {
    coefficient /= POWER_OF_10_16;
    exponent += 16;
  }

  if (coefficient % 100000000 == 0) {
    coefficient /= 100000000;
    exponent += 8;
  }

  if (coefficient % 10000 == 0) {
    coefficient /= 10000;
    exponent += 4;
  }

  if (coefficient % 100 == 0) {
    coefficient /= 100;
    exponent += 2;
  }

  if (coefficient % 10 == 0) {
    coefficient /= 10;
    exponent += 1;
  }
}

template <typename FloatingPointType>
  requires std::is_floating_point_v<FloatingPointType>
auto floating_point_to_string(const FloatingPointType value) -> std::string {
  std::ostringstream stream;
  stream << std::setprecision(
                std::numeric_limits<FloatingPointType>::max_digits10)
         << value;
  return stream.str();
}

struct ParsedDecimal {
  std::int64_t coefficient;
  std::uint64_t coefficient_high;
  std::int32_t exponent;
  std::uint8_t flags;
};

auto parse_digit_payload(const char *cursor, std::size_t count)
    -> std::int64_t {
  std::int64_t payload = 0;
  for (std::size_t index = 0; index < count; index++) {
    payload = payload * 10 + (cursor[index] - '0');
  }

  return payload;
}

auto parse_special(const char *input, std::size_t length) -> ParsedDecimal * {
  static ParsedDecimal result;
  const char *cursor = input;
  std::uint8_t sign_flag = 0;

  if (length > 0 && (*cursor == '-' || *cursor == '+')) {
    if (*cursor == '-') {
      sign_flag = FLAG_SIGN;
    }

    cursor++;
  }

  auto remaining = length - static_cast<std::size_t>(cursor - input);

  if (remaining >= 3 && (cursor[0] == 'N' || cursor[0] == 'n') &&
      (cursor[1] == 'a' || cursor[1] == 'A') &&
      (cursor[2] == 'N' || cursor[2] == 'n')) {
    result.flags = FLAG_NAN | sign_flag;
    result.exponent = 0;
    result.coefficient_high = 0;
    result.coefficient = parse_digit_payload(cursor + 3, remaining - 3);
    return &result;
  }

  if (remaining >= 4 && (cursor[0] == 's' || cursor[0] == 'S') &&
      (cursor[1] == 'N' || cursor[1] == 'n') &&
      (cursor[2] == 'a' || cursor[2] == 'A') &&
      (cursor[3] == 'N' || cursor[3] == 'n')) {
    result.flags = FLAG_NAN | FLAG_SNAN | sign_flag;
    result.exponent = 0;
    result.coefficient_high = 0;
    result.coefficient = parse_digit_payload(cursor + 4, remaining - 4);
    return &result;
  }

  if (remaining >= 3 && (cursor[0] == 'I' || cursor[0] == 'i') &&
      (cursor[1] == 'n' || cursor[1] == 'N') &&
      (cursor[2] == 'f' || cursor[2] == 'F') &&
      (remaining == 3 ||
       (remaining == 8 && (cursor[3] == 'i' || cursor[3] == 'I') &&
        (cursor[4] == 'n' || cursor[4] == 'N') &&
        (cursor[5] == 'i' || cursor[5] == 'I') &&
        (cursor[6] == 't' || cursor[6] == 'T') &&
        (cursor[7] == 'y' || cursor[7] == 'Y')))) {
    result.flags = FLAG_INFINITE | sign_flag;
    result.coefficient = 0;
    result.exponent = 0;
    result.coefficient_high = 0;
    return &result;
  }

  return nullptr;
}

auto parse_decimal_string(const char *input, std::size_t length)
    -> ParsedDecimal {
  auto *special = parse_special(input, length);
  if (special) {
    return *special;
  }

  ParsedDecimal result{
      .coefficient = 0, .coefficient_high = 0, .exponent = 0, .flags = 0};
  const char *cursor = input;
  const char *end = input + length;

  if (cursor < end && *cursor == '-') {
    result.flags = FLAG_SIGN;
    cursor++;
  } else if (cursor < end && *cursor == '+') {
    cursor++;
  }

  if (cursor >= end) {
    throw sourcemeta::core::DecimalParseError{};
  }

  std::array<char, 1024> digit_buffer{};
  std::uint32_t digit_count_total = 0;
  std::int32_t decimal_offset = -1;
  bool has_digit = false;

  while (cursor < end) {
    if (*cursor >= '0' && *cursor <= '9') {
      if (digit_count_total < digit_buffer.size()) {
        digit_buffer[digit_count_total] = *cursor;
      }

      digit_count_total++;
      has_digit = true;
    } else if (*cursor == '.') {
      if (decimal_offset >= 0) {
        throw sourcemeta::core::DecimalParseError{};
      }

      decimal_offset = static_cast<std::int32_t>(digit_count_total);
    } else if (*cursor == 'e' || *cursor == 'E') {
      cursor++;
      break;
    } else {
      throw sourcemeta::core::DecimalParseError{};
    }

    cursor++;
  }

  if (!has_digit) {
    throw sourcemeta::core::DecimalParseError{};
  }

  std::int64_t exponent_suffix_64 = 0;
  bool has_exponent_marker =
      cursor > input && (*(cursor - 1) == 'e' || *(cursor - 1) == 'E');
  if (cursor < end) {
    bool is_exponent_negative = false;
    if (*cursor == '-') {
      is_exponent_negative = true;
      cursor++;
    } else if (*cursor == '+') {
      cursor++;
    }

    bool has_exponent_digit = false;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
      exponent_suffix_64 = exponent_suffix_64 * 10 + (*cursor - '0');
      has_exponent_digit = true;
      cursor++;
      if (exponent_suffix_64 > 4000000000LL) {
        while (cursor < end && *cursor >= '0' && *cursor <= '9') {
          cursor++;
        }

        break;
      }
    }

    if (!has_exponent_digit) {
      throw sourcemeta::core::DecimalParseError{};
    }

    if (cursor < end) {
      throw sourcemeta::core::DecimalParseError{};
    }

    if (is_exponent_negative) {
      exponent_suffix_64 = -exponent_suffix_64;
    }

  } else if (has_exponent_marker) {
    throw sourcemeta::core::DecimalParseError{};
  }

  auto exponent_suffix = static_cast<std::int32_t>(std::min(
      std::max(
          exponent_suffix_64,
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())),
      static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));

  if (decimal_offset >= 0) {
    result.exponent =
        exponent_suffix -
        (static_cast<std::int32_t>(digit_count_total) - decimal_offset);
  } else {
    result.exponent = exponent_suffix;
  }

  std::uint32_t leading_zeros = 0;
  while (leading_zeros < digit_count_total - 1 &&
         digit_buffer[leading_zeros] == '0') {
    leading_zeros++;
  }

  auto significant_digits = digit_count_total - leading_zeros;

  if (significant_digits <= 18) {
    std::int64_t coefficient = 0;
    for (std::uint32_t index = leading_zeros; index < digit_count_total;
         index++) {
      coefficient = coefficient * 10 + (digit_buffer[index] - '0');
    }

    result.coefficient = coefficient;
  } else if (significant_digits <= 36) {
    auto low_start = leading_zeros + significant_digits -
                     static_cast<std::uint32_t>(BASE_DIGITS);
    std::uint64_t low_word = 0;
    for (std::uint32_t index = low_start;
         index < leading_zeros + significant_digits; index++) {
      low_word =
          low_word * 10 + static_cast<std::uint64_t>(digit_buffer[index] - '0');
    }

    std::uint64_t high_word = 0;
    for (std::uint32_t index = leading_zeros; index < low_start; index++) {
      high_word = high_word * 10 +
                  static_cast<std::uint64_t>(digit_buffer[index] - '0');
    }

    result.coefficient = static_cast<std::int64_t>(low_word);
    result.coefficient_high = high_word;
    result.flags |= FLAG_BIG;
  } else {
    auto big = BigCoefficient::from_digits(digit_buffer.data() + leading_zeros,
                                           significant_digits);
    store_big_pointer(result.coefficient, std::move(big));
    result.coefficient_high = 0;
    result.flags |= static_cast<std::uint8_t>(FLAG_BIG | FLAG_HEAP);
  }

  return result;
}

template <typename FloatingPointType>
auto is_representable_as_floating_point(
    const sourcemeta::core::Decimal &decimal) -> bool {
  if (decimal.is_nan() || decimal.is_infinite()) {
    return true;
  }

  if (!decimal.is_finite()) {
    return false;
  }

  const std::string decimal_string{decimal.to_scientific_string()};
  FloatingPointType converted_value;
  try {
    if constexpr (std::is_same_v<FloatingPointType, float>) {
      converted_value = std::stof(decimal_string);
    } else if constexpr (std::is_same_v<FloatingPointType, double>) {
      converted_value = std::stod(decimal_string);
    }

  } catch (const std::out_of_range &) {
    return false;
  }

  if (!std::isfinite(converted_value)) {
    return false;
  }

  std::ostringstream stream;
  stream << std::setprecision(
                std::numeric_limits<FloatingPointType>::max_digits10)
         << converted_value;
  const sourcemeta::core::Decimal roundtrip{stream.str()};
  return decimal == roundtrip;
}

void check_exponent_overflow(std::int32_t left_exponent,
                             std::int32_t right_exponent) {
  if (left_exponent == std::numeric_limits<std::int32_t>::max() ||
      left_exponent == std::numeric_limits<std::int32_t>::min() ||
      right_exponent == std::numeric_limits<std::int32_t>::max() ||
      right_exponent == std::numeric_limits<std::int32_t>::min()) {
    throw sourcemeta::core::NumericOverflowError{};
  }
}

auto format_special_value(std::string &result, std::uint8_t flags,
                          std::int64_t coefficient) -> bool {
  if (flags & FLAG_NAN) {
    if (flags & FLAG_SIGN) {
      result += '-';
    }

    if (flags & FLAG_SNAN) {
      result += "sNaN";
    } else {
      result += "NaN";
    }

    if (coefficient > 0) {
      result += std::to_string(coefficient);
    }

    return true;
  }

  if (flags & FLAG_INFINITE) {
    if (flags & FLAG_SIGN) {
      result += '-';
    }

    result += "Infinity";
    return true;
  }

  return false;
}

} // namespace

namespace sourcemeta::core {

Decimal::Decimal() noexcept = default;

Decimal::~Decimal() { free_big_coefficient(this->coefficient_, this->flags_); }

Decimal::Decimal(const Decimal &other)
    : coefficient_{other.coefficient_},
      coefficient_high_{other.coefficient_high_}, exponent_{other.exponent_},
      flags_{other.flags_} {
  if (other.flags_ & FLAG_HEAP) {
    store_big_pointer(this->coefficient_,
                      load_big_pointer(other.coefficient_)->clone());
  }
}

Decimal::Decimal(Decimal &&other) noexcept
    : coefficient_{other.coefficient_},
      coefficient_high_{other.coefficient_high_}, exponent_{other.exponent_},
      flags_{other.flags_} {
  other.coefficient_ = 0;
  other.coefficient_high_ = 0;
  other.exponent_ = 0;
  other.flags_ = 0;
}

auto Decimal::operator=(const Decimal &other) -> Decimal & {
  if (this != &other) {
    free_big_coefficient(this->coefficient_, this->flags_);
    this->coefficient_ = other.coefficient_;
    this->coefficient_high_ = other.coefficient_high_;
    this->exponent_ = other.exponent_;
    this->flags_ = other.flags_;
    if (other.flags_ & FLAG_HEAP) {
      store_big_pointer(this->coefficient_,
                        load_big_pointer(other.coefficient_)->clone());
    }
  }

  return *this;
}

auto Decimal::operator=(Decimal &&other) noexcept -> Decimal & {
  if (this != &other) {
    free_big_coefficient(this->coefficient_, this->flags_);
    this->coefficient_ = other.coefficient_;
    this->coefficient_high_ = other.coefficient_high_;
    this->exponent_ = other.exponent_;
    this->flags_ = other.flags_;
    other.coefficient_ = 0;
    other.coefficient_high_ = 0;
    other.exponent_ = 0;
    other.flags_ = 0;
  }

  return *this;
}

Decimal::Decimal(const std::int64_t value) {
  if (value == std::numeric_limits<std::int64_t>::min()) {
    auto absolute_value = static_cast<std::uint64_t>(-(value + 1)) + 1;
    this->coefficient_ = static_cast<std::int64_t>(absolute_value % BASE);
    this->coefficient_high_ = absolute_value / BASE;
    this->flags_ = FLAG_BIG | FLAG_SIGN;
  } else if (value < 0) {
    this->coefficient_ = -value;
    this->flags_ = FLAG_SIGN;
  } else {
    this->coefficient_ = value;
  }
}

Decimal::Decimal(const std::uint64_t value) {
  if (value > static_cast<std::uint64_t>(COMPACT_MAX)) {
    this->coefficient_ = static_cast<std::int64_t>(value % BASE);
    this->coefficient_high_ = value / BASE;
    this->flags_ = FLAG_BIG;
  } else {
    this->coefficient_ = static_cast<std::int64_t>(value);
  }
}

Decimal::Decimal(const float value)
    : Decimal{floating_point_to_string(value)} {}

Decimal::Decimal(const double value)
    : Decimal{floating_point_to_string(value)} {}

Decimal::Decimal(const char *const value) {
  auto parsed = parse_decimal_string(value, std::strlen(value));
  this->coefficient_ = parsed.coefficient;
  this->coefficient_high_ = parsed.coefficient_high;
  this->exponent_ = parsed.exponent;
  this->flags_ = parsed.flags;
}

Decimal::Decimal(const std::string &value) {
  auto parsed = parse_decimal_string(value.c_str(), value.size());
  this->coefficient_ = parsed.coefficient;
  this->coefficient_high_ = parsed.coefficient_high;
  this->exponent_ = parsed.exponent;
  this->flags_ = parsed.flags;
}

Decimal::Decimal(const std::string_view value) {
  auto parsed = parse_decimal_string(value.data(), value.size());
  this->coefficient_ = parsed.coefficient;
  this->coefficient_high_ = parsed.coefficient_high;
  this->exponent_ = parsed.exponent;
  this->flags_ = parsed.flags;
}

auto Decimal::nan(const std::uint64_t payload) -> Decimal {
  Decimal result;
  result.flags_ = FLAG_NAN;
  result.coefficient_ = static_cast<std::int64_t>(payload);
  return result;
}

auto Decimal::snan(const std::uint64_t payload) -> Decimal {
  Decimal result;
  result.flags_ = FLAG_NAN | FLAG_SNAN;
  result.coefficient_ = static_cast<std::int64_t>(payload);
  return result;
}

auto Decimal::infinity() -> Decimal {
  Decimal result;
  result.flags_ = FLAG_INFINITE;
  return result;
}

auto Decimal::negative_infinity() -> Decimal {
  Decimal result;
  result.flags_ = FLAG_INFINITE | FLAG_SIGN;
  return result;
}

auto Decimal::to_scientific_string() const -> std::string {
  std::string result;

  if (format_special_value(result, this->flags_, this->coefficient_)) {
    return result;
  }

  auto digit_string = coefficient_to_digit_string(
      this->coefficient_, this->coefficient_high_, this->flags_);
  auto number_of_digits = static_cast<std::int32_t>(digit_string.size());
  auto adjusted_exponent = this->exponent_ + number_of_digits - 1;

  if (this->flags_ & FLAG_SIGN) {
    result += '-';
  }

  if (number_of_digits == 1) {
    result += digit_string;
  } else {
    result += digit_string[0];
    result += '.';
    result += digit_string.substr(1);
  }

  result += 'e';
  if (adjusted_exponent >= 0) {
    result += '+';
  }

  result += std::to_string(adjusted_exponent);

  return result;
}

// General Decimal Arithmetic spec, to-engineering-string
auto Decimal::to_string() const -> std::string {
  std::string result;

  if (format_special_value(result, this->flags_, this->coefficient_)) {
    return result;
  }

  auto digit_string = coefficient_to_digit_string(
      this->coefficient_, this->coefficient_high_, this->flags_);
  auto number_of_digits = static_cast<std::int32_t>(digit_string.size());
  auto integer_digit_count = number_of_digits + this->exponent_;

  std::int32_t decimal_place;
  if (this->exponent_ <= 0 && integer_digit_count > -6) {
    decimal_place = integer_digit_count;
  } else if (this->coefficient_ == 0 && !(this->flags_ & FLAG_BIG)) {
    decimal_place = -1 + ((this->exponent_ + 2) % 3 + 3) % 3;
  } else {
    decimal_place = integer_digit_count +
                    ((integer_digit_count - 1) % 3 + 3) % 3 -
                    ((integer_digit_count - 1) % 3 + 3) % 3;
    auto adjusted = integer_digit_count - 1;
    auto remainder = ((adjusted % 3) + 3) % 3;
    decimal_place = 1 + remainder;
  }

  if (this->flags_ & FLAG_SIGN) {
    result += '-';
  }

  if (decimal_place <= 0) {
    result += "0.";
    for (std::int32_t index = 0; index < -decimal_place; index++) {
      result += '0';
    }

    result += digit_string;
  } else if (decimal_place >= number_of_digits) {
    result += digit_string;
    for (std::int32_t index = 0; index < decimal_place - number_of_digits;
         index++) {
      result += '0';
    }

  } else {
    result += digit_string.substr(0, static_cast<std::size_t>(decimal_place));
    result += '.';
    result += digit_string.substr(static_cast<std::size_t>(decimal_place));
  }

  if (integer_digit_count != decimal_place) {
    result += 'e';
    auto engineering_exponent = integer_digit_count - decimal_place;
    if (engineering_exponent >= 0) {
      result += '+';
    }

    result += std::to_string(engineering_exponent);
  }

  return result;
}

auto Decimal::to_int64() const -> std::int64_t {
  assert(this->is_int64());

  if (this->flags_ & FLAG_BIG) {
    auto big = coefficient_as_big(this->coefficient_, this->coefficient_high_,
                                  this->flags_);
    auto value = big.to_uint128(this->exponent_);
    if (this->flags_ & FLAG_SIGN) {
      return -static_cast<std::int64_t>(value);
    }

    return static_cast<std::int64_t>(value);
  }

  auto coefficient = this->coefficient_;
  auto exponent = this->exponent_;
  while (exponent > 0) {
    coefficient *= 10;
    exponent--;
  }

  return (this->flags_ & FLAG_SIGN) ? -coefficient : coefficient;
}

auto Decimal::to_int32() const -> std::int32_t {
  assert(this->is_int32());
  return static_cast<std::int32_t>(this->to_int64());
}

auto Decimal::to_uint64() const -> std::uint64_t {
  assert(this->is_uint64());

  if (this->flags_ & FLAG_BIG) {
    auto big = coefficient_as_big(this->coefficient_, this->coefficient_high_,
                                  this->flags_);
    auto value = static_cast<std::uint64_t>(big.to_uint128(this->exponent_));
    return value;
  }

  auto coefficient = this->coefficient_;
  auto exponent = this->exponent_;

  auto result = static_cast<std::uint64_t>(coefficient);
  while (exponent > 0) {
    result *= 10;
    exponent--;
  }

  return result;
}

auto Decimal::to_uint32() const -> std::uint32_t {
  assert(this->is_uint32());
  return static_cast<std::uint32_t>(this->to_uint64());
}

auto Decimal::to_float() const -> float {
  return std::stof(this->to_scientific_string());
}

auto Decimal::to_double() const -> double {
  return std::stod(this->to_scientific_string());
}

auto Decimal::is_zero() const -> bool {
  if (this->flags_ & SPECIAL_MASK) {
    if (this->flags_ & FLAG_HEAP) {
      return load_big_pointer(this->coefficient_)->is_zero();
    }

    if (this->flags_ & FLAG_BIG) {
      return this->coefficient_ == 0 && this->coefficient_high_ == 0;
    }

    return false;
  }

  return this->coefficient_ == 0;
}

auto Decimal::is_integral() const -> bool {
  if (this->flags_ & (FLAG_NAN | FLAG_SNAN | FLAG_INFINITE)) {
    return false;
  }

  if (this->exponent_ >= 0) {
    return true;
  }

  if (this->is_zero()) {
    return true;
  }

  if (this->flags_ & FLAG_BIG) {
    auto big = coefficient_as_big(this->coefficient_, this->coefficient_high_,
                                  this->flags_);
    auto stripped = big.strip_trailing_zeros();
    return stripped >= -this->exponent_;
  }

  auto coefficient = this->coefficient_;
  auto exponent = this->exponent_;
  while (exponent < 0 && coefficient != 0 && coefficient % 10 == 0) {
    coefficient /= 10;
    exponent++;
  }

  return exponent >= 0;
}

auto Decimal::is_integer() const -> bool {
  return this->is_integral() && this->exponent_ >= 0;
}

auto Decimal::is_finite() const -> bool {
  return !(this->flags_ & (FLAG_NAN | FLAG_SNAN | FLAG_INFINITE));
}

auto Decimal::is_real() const -> bool {
  return this->is_finite() && !this->is_integral();
}

auto Decimal::is_float() const -> bool {
  return is_representable_as_floating_point<float>(*this);
}

auto Decimal::is_double() const -> bool {
  return is_representable_as_floating_point<double>(*this);
}

auto Decimal::is_int32() const -> bool {
  assert(this->is_integer());
  return *this >= Decimal{std::numeric_limits<std::int32_t>::min()} &&
         *this <= Decimal{std::numeric_limits<std::int32_t>::max()};
}

auto Decimal::is_int64() const -> bool {
  assert(this->is_integer());
  return *this >= Decimal{std::numeric_limits<std::int64_t>::min()} &&
         *this <= Decimal{std::numeric_limits<std::int64_t>::max()};
}

auto Decimal::is_uint32() const -> bool {
  assert(this->is_integer());
  return *this >= Decimal{0} &&
         *this <= Decimal{std::numeric_limits<std::uint32_t>::max()};
}

auto Decimal::is_uint64() const -> bool {
  assert(this->is_integer());
  return *this >= Decimal{0} &&
         *this <= Decimal{std::numeric_limits<std::uint64_t>::max()};
}

auto Decimal::is_nan() const -> bool { return (this->flags_ & FLAG_NAN) != 0; }

auto Decimal::is_snan() const -> bool {
  return (this->flags_ & FLAG_SNAN) != 0;
}

auto Decimal::is_qnan() const -> bool {
  return (this->flags_ & FLAG_NAN) != 0 && !(this->flags_ & FLAG_SNAN);
}

auto Decimal::nan_payload() const -> std::uint64_t {
  assert(this->is_nan());
  return static_cast<std::uint64_t>(this->coefficient_);
}

auto Decimal::is_infinite() const -> bool {
  return (this->flags_ & FLAG_INFINITE) != 0;
}

auto Decimal::is_signed() const -> bool {
  return (this->flags_ & FLAG_SIGN) != 0;
}

auto Decimal::to_integral() const -> Decimal {
  if (!this->is_finite()) {
    return *this;
  }

  if (this->exponent_ >= 0) {
    return *this;
  }

  if (this->flags_ & FLAG_BIG) {
    auto digit_string = coefficient_to_digit_string(
        this->coefficient_, this->coefficient_high_, this->flags_);
    auto number_of_digits = static_cast<std::int32_t>(digit_string.size());
    auto digits_to_remove = -this->exponent_;

    if (digits_to_remove >= number_of_digits) {
      Decimal result;
      return result;
    }

    auto integer_string = digit_string.substr(
        0, static_cast<std::size_t>(number_of_digits - digits_to_remove));
    Decimal result{integer_string};
    if (this->flags_ & FLAG_SIGN) {
      result.flags_ |= FLAG_SIGN;
    }

    return result;
  }

  auto coefficient = this->coefficient_;
  auto digits_to_remove = -this->exponent_;

  if (static_cast<std::uint32_t>(digits_to_remove) >=
      digit_count(static_cast<std::uint64_t>(coefficient))) {
    return Decimal{};
  }

  std::int64_t divisor = 1;
  for (std::int32_t index = 0; index < digits_to_remove; index++) {
    divisor *= 10;
  }

  auto quotient = coefficient / divisor;
  auto remainder = coefficient % divisor;
  auto half = divisor / 2;

  if (remainder > half || (remainder == half && quotient % 2 != 0)) {
    quotient++;
  }

  Decimal result;
  result.coefficient_ = quotient;
  result.exponent_ = 0;
  if (this->flags_ & FLAG_SIGN) {
    result.flags_ = FLAG_SIGN;
  }

  return result;
}

auto Decimal::divisible_by(const Decimal &divisor) const -> bool {
  if (divisor.is_zero()) {
    return false;
  }

  if (this->is_zero()) {
    return true;
  }

  if (!this->is_finite() || !divisor.is_finite()) {
    return false;
  }

  if (!(divisor.flags_ & FLAG_BIG) && !(this->flags_ & FLAG_HEAP)) {
    auto divisor_value = static_cast<std::uint64_t>(divisor.coefficient_);

    std::uint64_t dividend_mod;
    if (this->flags_ & FLAG_BIG) {
      auto hi_mod = this->coefficient_high_ % divisor_value;
      auto base_mod = BASE % divisor_value;
      auto lo_mod =
          static_cast<std::uint64_t>(this->coefficient_) % divisor_value;
      dividend_mod = static_cast<std::uint64_t>(
          (static_cast<sourcemeta::core::uint128_t>(hi_mod) * base_mod +
           lo_mod) %
          divisor_value);
    } else {
      dividend_mod =
          static_cast<std::uint64_t>(this->coefficient_) % divisor_value;
    }

    if (this->exponent_ >= divisor.exponent_) {
      auto difference =
          static_cast<std::uint32_t>(this->exponent_ - divisor.exponent_);
      auto pow_mod = modular_pow10(difference, divisor_value);
      return static_cast<std::uint64_t>(
                 static_cast<sourcemeta::core::uint128_t>(dividend_mod) *
                 pow_mod % divisor_value) == 0;
    }

    auto difference =
        static_cast<std::uint32_t>(divisor.exponent_ - this->exponent_);
    if (difference > 36) {
      return false;
    }

    sourcemeta::core::uint128_t remaining;
    if (this->flags_ & FLAG_BIG) {
      remaining =
          static_cast<sourcemeta::core::uint128_t>(this->coefficient_high_) *
              BASE +
          static_cast<std::uint64_t>(this->coefficient_);
    } else {
      remaining = static_cast<sourcemeta::core::uint128_t>(
          static_cast<std::uint64_t>(this->coefficient_));
    }

    auto diff_left = difference;
    while (diff_left > 0) {
      auto chunk = std::min(diff_left, static_cast<std::uint32_t>(19));
      auto power = POWERS_OF_10[chunk];
      if (static_cast<std::uint64_t>(remaining % power) != 0) {
        return false;
      }

      remaining = remaining / power;
      diff_left -= chunk;
    }

    return static_cast<std::uint64_t>(remaining % divisor_value) == 0;
  }

  auto dividend_big = coefficient_as_big(this->coefficient_,
                                         this->coefficient_high_, this->flags_);
  auto divisor_big = coefficient_as_big(
      divisor.coefficient_, divisor.coefficient_high_, divisor.flags_);

  BigCoefficient::align_exponents(dividend_big, divisor_big, this->exponent_,
                                  divisor.exponent_);

  auto [quotient, remainder] = dividend_big.divide_modulo(divisor_big);

  return remainder.is_zero();
}

auto Decimal::same_quantum(const Decimal &other) const -> bool {
  if (this->is_nan() && other.is_nan()) {
    return true;
  }

  if (this->is_infinite() && other.is_infinite()) {
    return true;
  }

  if (this->is_nan() || other.is_nan() || this->is_infinite() ||
      other.is_infinite()) {
    return false;
  }

  return this->exponent_ == other.exponent_;
}

auto Decimal::reduce() const -> Decimal {
  if (!this->is_finite()) {
    return *this;
  }

  if (this->is_zero()) {
    Decimal result;
    if (this->flags_ & FLAG_SIGN) {
      result.flags_ = FLAG_SIGN;
    }

    return result;
  }

  if (this->flags_ & FLAG_BIG) {
    auto big = coefficient_as_big(this->coefficient_, this->coefficient_high_,
                                  this->flags_);
    auto stripped_count = big.strip_trailing_zeros();
    auto new_exponent =
        static_cast<std::int64_t>(this->exponent_) + stripped_count;
    if (new_exponent > std::numeric_limits<std::int32_t>::max() ||
        new_exponent < std::numeric_limits<std::int32_t>::min()) {
      throw NumericOverflowError{};
    }

    Decimal result;
    bool result_negative = (this->flags_ & FLAG_SIGN) != 0;
    store_big_result(result.coefficient_, result.coefficient_high_,
                     result.flags_, std::move(big), result_negative);
    result.exponent_ = static_cast<std::int32_t>(new_exponent);
    return result;
  }

  auto coefficient = this->coefficient_;
  auto exponent = this->exponent_;
  strip_trailing_zeros(coefficient, exponent);

  Decimal result;
  result.coefficient_ = coefficient;
  result.exponent_ = exponent;
  if (this->flags_ & FLAG_SIGN) {
    result.flags_ = FLAG_SIGN;
  }

  return result;
}

auto Decimal::logb() const -> Decimal {
  if (this->is_nan()) {
    return *this;
  }

  if (this->is_infinite()) {
    return Decimal::infinity();
  }

  if (this->is_zero()) {
    throw NumericDivisionByZeroError{};
  }

  std::int64_t digits;
  if (this->flags_ & FLAG_BIG) {
    auto big = coefficient_as_big(this->coefficient_, this->coefficient_high_,
                                  this->flags_);
    digits = static_cast<std::int64_t>(big.digit_count());
  } else {
    digits = static_cast<std::int64_t>(
        digit_count(static_cast<std::uint64_t>(this->coefficient_)));
  }

  auto adjusted = digits + this->exponent_ - 1;
  return Decimal{adjusted};
}

auto Decimal::scale_by(const Decimal &scale) const -> Decimal {
  if (this->is_snan()) {
    throw NumericInvalidOperationError{};
  }

  if (scale.is_snan()) {
    throw NumericInvalidOperationError{};
  }

  if (this->is_qnan()) {
    return *this;
  }

  if (scale.is_qnan()) {
    return scale;
  }

  if (!scale.is_finite() || scale.exponent_ != 0) {
    throw NumericInvalidOperationError{};
  }

  if (this->is_infinite()) {
    return *this;
  }

  if (!scale.is_int64()) {
    throw NumericOverflowError{};
  }

  auto scale_value = scale.to_int64();
  auto new_exponent = static_cast<std::int64_t>(this->exponent_) + scale_value;
  if (new_exponent > std::numeric_limits<std::int32_t>::max() ||
      new_exponent < std::numeric_limits<std::int32_t>::min()) {
    throw NumericOverflowError{};
  }

  Decimal result{*this};
  result.exponent_ = static_cast<std::int32_t>(new_exponent);
  return result;
}

auto Decimal::compare_total(const Decimal &other) const -> Decimal {
  auto classify = [](const Decimal &value) -> int {
    if (value.is_qnan()) {
      return value.is_signed() ? -6 : 6;
    }

    if (value.is_snan()) {
      return value.is_signed() ? -5 : 5;
    }

    if (value.is_infinite()) {
      return value.is_signed() ? -4 : 4;
    }

    if (value.is_zero()) {
      return value.is_signed() ? -2 : 2;
    }

    return value.is_signed() ? -3 : 3;
  };

  auto left_class = classify(*this);
  auto right_class = classify(other);

  if (left_class != right_class) {
    if (left_class < right_class) {
      return Decimal{-1};
    }

    return Decimal{1};
  }

  bool is_negative = left_class < 0;

  if (this->is_nan()) {
    auto left_payload = this->nan_payload();
    auto right_payload = other.nan_payload();
    if (left_payload != right_payload) {
      bool left_less = left_payload < right_payload;
      if (is_negative) {
        left_less = !left_less;
      }

      return left_less ? Decimal{-1} : Decimal{1};
    }

    return Decimal{0};
  }

  if (this->is_infinite()) {
    return Decimal{0};
  }

  if (this->is_zero()) {
    if (this->exponent_ != other.exponent_) {
      bool left_less = this->exponent_ < other.exponent_;
      if (is_negative) {
        left_less = !left_less;
      }

      return left_less ? Decimal{-1} : Decimal{1};
    }

    return Decimal{0};
  }

  int magnitude_compare;
  if ((this->flags_ & FLAG_BIG) || (other.flags_ & FLAG_BIG)) {
    auto left_big = coefficient_as_big(this->coefficient_,
                                       this->coefficient_high_, this->flags_);
    auto right_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);
    auto left_digit_count = static_cast<std::int64_t>(left_big.digit_count());
    auto right_digit_count = static_cast<std::int64_t>(right_big.digit_count());
    auto left_adjusted = this->exponent_ + left_digit_count - 1;
    auto right_adjusted = other.exponent_ + right_digit_count - 1;

    if (left_adjusted != right_adjusted) {
      magnitude_compare = left_adjusted < right_adjusted ? -1 : 1;
    } else {
      BigCoefficient::align_exponents(left_big, right_big, this->exponent_,
                                      other.exponent_);
      magnitude_compare = left_big.compare(right_big);
    }
  } else {
    if (this->exponent_ == other.exponent_) {
      if (this->coefficient_ < other.coefficient_) {
        magnitude_compare = -1;
      } else if (this->coefficient_ > other.coefficient_) {
        magnitude_compare = 1;
      } else {
        magnitude_compare = 0;
      }
    } else {
      auto left_digits =
          digit_count(static_cast<std::uint64_t>(this->coefficient_));
      auto right_digits =
          digit_count(static_cast<std::uint64_t>(other.coefficient_));
      auto left_adjusted =
          this->exponent_ + static_cast<std::int32_t>(left_digits) - 1;
      auto right_adjusted =
          other.exponent_ + static_cast<std::int32_t>(right_digits) - 1;

      if (left_adjusted != right_adjusted) {
        magnitude_compare = left_adjusted < right_adjusted ? -1 : 1;
      } else {
        auto exponent_difference = this->exponent_ - other.exponent_;
        if (exponent_difference > 0) {
          if (exponent_difference <= 18) {
            auto multiplier =
                POWERS_OF_10[static_cast<std::uint32_t>(exponent_difference)];
            auto product =
                static_cast<sourcemeta::core::uint128_t>(this->coefficient_) *
                multiplier;
            auto right_128 =
                static_cast<sourcemeta::core::uint128_t>(other.coefficient_);
            if (product < right_128) {
              magnitude_compare = -1;
            } else if (product > right_128) {
              magnitude_compare = 1;
            } else {
              magnitude_compare = 0;
            }
          } else {
            magnitude_compare = 1;
          }
        } else {
          auto difference = static_cast<std::uint32_t>(-exponent_difference);
          if (difference <= 18) {
            auto multiplier = POWERS_OF_10[difference];
            auto product =
                static_cast<sourcemeta::core::uint128_t>(other.coefficient_) *
                multiplier;
            auto left_128 =
                static_cast<sourcemeta::core::uint128_t>(this->coefficient_);
            if (left_128 < product) {
              magnitude_compare = -1;
            } else if (left_128 > product) {
              magnitude_compare = 1;
            } else {
              magnitude_compare = 0;
            }
          } else {
            magnitude_compare = -1;
          }
        }
      }
    }
  }

  if (magnitude_compare != 0) {
    if (is_negative) {
      magnitude_compare = -magnitude_compare;
    }

    return magnitude_compare < 0 ? Decimal{-1} : Decimal{1};
  }

  if (this->exponent_ != other.exponent_) {
    bool left_less = this->exponent_ < other.exponent_;
    if (is_negative) {
      left_less = !left_less;
    }

    return left_less ? Decimal{-1} : Decimal{1};
  }

  return Decimal{0};
}

auto Decimal::divide_integer(const Decimal &other) const -> Decimal {
  if (this->is_snan() || other.is_snan()) {
    throw NumericInvalidOperationError{};
  }

  if (this->is_nan()) {
    return Decimal::nan(this->nan_payload());
  }

  if (other.is_nan()) {
    return Decimal::nan(other.nan_payload());
  }

  if (this->is_infinite() && other.is_infinite()) {
    throw NumericInvalidOperationError{};
  }

  bool result_negative = ((this->flags_ ^ other.flags_) & FLAG_SIGN) != 0;

  if (this->is_infinite()) {
    Decimal result =
        result_negative ? Decimal::negative_infinity() : Decimal::infinity();
    return result;
  }

  if (other.is_zero()) {
    if (this->is_zero()) {
      throw NumericInvalidOperationError{};
    }

    throw NumericDivisionByZeroError{};
  }

  if (other.is_infinite()) {
    Decimal result;
    if (result_negative) {
      result.flags_ = FLAG_SIGN;
    }

    return result;
  }

  if (this->is_zero()) {
    Decimal result;
    if (result_negative) {
      result.flags_ = FLAG_SIGN;
    }

    return result;
  }

  auto dividend_big = coefficient_as_big(this->coefficient_,
                                         this->coefficient_high_, this->flags_);
  auto divisor_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);

  BigCoefficient::align_exponents(dividend_big, divisor_big, this->exponent_,
                                  other.exponent_);

  auto [quotient, remainder] = dividend_big.divide_modulo(divisor_big);

  Decimal result;
  store_big_result(result.coefficient_, result.coefficient_high_, result.flags_,
                   std::move(quotient), result_negative);
  if (result.is_zero()) {
    result.flags_ = result_negative ? FLAG_SIGN : 0;
  }

  result.exponent_ = 0;
  return result;
}

auto Decimal::operator==(const Decimal &other) const -> bool {
  if (this->is_nan() || other.is_nan()) {
    return false;
  }

  if (this->is_infinite() && other.is_infinite()) {
    return (this->flags_ & FLAG_SIGN) == (other.flags_ & FLAG_SIGN);
  }

  if (this->is_infinite() || other.is_infinite()) {
    return false;
  }

  if (this->is_zero() && other.is_zero()) {
    return true;
  }

  if ((this->flags_ & FLAG_SIGN) != (other.flags_ & FLAG_SIGN)) {
    return false;
  }

  auto left_coefficient = this->coefficient_;
  auto left_exponent = this->exponent_;
  auto right_coefficient = other.coefficient_;
  auto right_exponent = other.exponent_;

  if ((this->flags_ & FLAG_BIG) || (other.flags_ & FLAG_BIG)) {
    auto left_big = coefficient_as_big(this->coefficient_,
                                       this->coefficient_high_, this->flags_);
    auto right_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);
    BigCoefficient::align_exponents(left_big, right_big, left_exponent,
                                    right_exponent);
    return left_big.compare(right_big) == 0;
  }

  strip_trailing_zeros(left_coefficient, left_exponent);
  strip_trailing_zeros(right_coefficient, right_exponent);
  return left_coefficient == right_coefficient &&
         left_exponent == right_exponent;
}

auto Decimal::operator!=(const Decimal &other) const -> bool {
  return !(*this == other);
}

auto Decimal::operator<(const Decimal &other) const -> bool {
  if (this->is_nan() || other.is_nan()) {
    return false;
  }

  if (this->is_infinite()) {
    if (this->flags_ & FLAG_SIGN) {
      return !other.is_infinite() || !(other.flags_ & FLAG_SIGN);
    }

    return false;
  }

  if (other.is_infinite()) {
    if (other.flags_ & FLAG_SIGN) {
      return false;
    }

    return true;
  }

  bool left_negative = (this->flags_ & FLAG_SIGN) != 0;
  bool right_negative = (other.flags_ & FLAG_SIGN) != 0;
  bool left_zero = this->is_zero();
  bool right_zero = other.is_zero();

  if (left_zero && right_zero) {
    return false;
  }

  if (left_zero) {
    return !right_negative;
  }

  if (right_zero) {
    return left_negative;
  }

  if (left_negative && !right_negative) {
    return true;
  }

  if (!left_negative && right_negative) {
    return false;
  }

  int magnitude_compare;

  if ((this->flags_ & FLAG_BIG) || (other.flags_ & FLAG_BIG)) {
    auto left_big = coefficient_as_big(this->coefficient_,
                                       this->coefficient_high_, this->flags_);
    auto right_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);

    auto left_digit_count = static_cast<std::int64_t>(left_big.digit_count());
    auto right_digit_count = static_cast<std::int64_t>(right_big.digit_count());
    auto left_adjusted = this->exponent_ + left_digit_count - 1;
    auto right_adjusted = other.exponent_ + right_digit_count - 1;

    if (left_adjusted != right_adjusted) {
      magnitude_compare = left_adjusted < right_adjusted ? -1 : 1;
    } else {
      BigCoefficient::align_exponents(left_big, right_big, this->exponent_,
                                      other.exponent_);
      magnitude_compare = left_big.compare(right_big);
    }

  } else {
    if (this->exponent_ == other.exponent_) {
      if (this->coefficient_ < other.coefficient_) {
        magnitude_compare = -1;
      } else if (this->coefficient_ > other.coefficient_) {
        magnitude_compare = 1;
      } else {
        magnitude_compare = 0;
      }

    } else {
      auto left_digits =
          digit_count(static_cast<std::uint64_t>(this->coefficient_));
      auto right_digits =
          digit_count(static_cast<std::uint64_t>(other.coefficient_));
      auto left_adjusted =
          this->exponent_ + static_cast<std::int32_t>(left_digits) - 1;
      auto right_adjusted =
          other.exponent_ + static_cast<std::int32_t>(right_digits) - 1;

      if (left_adjusted != right_adjusted) {
        magnitude_compare = left_adjusted < right_adjusted ? -1 : 1;
      } else {
        auto exponent_difference = this->exponent_ - other.exponent_;
        if (exponent_difference > 0) {
          if (exponent_difference <= 18) {
            auto multiplier =
                POWERS_OF_10[static_cast<std::uint32_t>(exponent_difference)];
            auto product =
                static_cast<sourcemeta::core::uint128_t>(this->coefficient_) *
                multiplier;
            auto right_128 =
                static_cast<sourcemeta::core::uint128_t>(other.coefficient_);
            if (product < right_128) {
              magnitude_compare = -1;
            } else if (product > right_128) {
              magnitude_compare = 1;
            } else {
              magnitude_compare = 0;
            }

          } else {
            magnitude_compare = 1;
          }

        } else {
          auto difference = static_cast<std::uint32_t>(-exponent_difference);
          if (difference <= 18) {
            auto multiplier = POWERS_OF_10[difference];
            auto product =
                static_cast<sourcemeta::core::uint128_t>(other.coefficient_) *
                multiplier;
            auto left_128 =
                static_cast<sourcemeta::core::uint128_t>(this->coefficient_);
            if (left_128 < product) {
              magnitude_compare = -1;
            } else if (left_128 > product) {
              magnitude_compare = 1;
            } else {
              magnitude_compare = 0;
            }

          } else {
            magnitude_compare = -1;
          }
        }
      }
    }
  }

  return left_negative ? magnitude_compare > 0 : magnitude_compare < 0;
}

auto Decimal::operator<=(const Decimal &other) const -> bool {
  if (this->is_nan() || other.is_nan()) {
    return false;
  }

  return !(other < *this);
}

auto Decimal::operator>(const Decimal &other) const -> bool {
  return other < *this;
}

auto Decimal::operator>=(const Decimal &other) const -> bool {
  if (this->is_nan() || other.is_nan()) {
    return false;
  }

  return !(*this < other);
}

auto Decimal::operator+=(const Decimal &other) -> Decimal & {
  if (!this->is_finite() || !other.is_finite()) {
    if (this->is_nan() || other.is_nan()) {
      *this = Decimal::nan();
      return *this;
    }

    if (this->is_infinite() && other.is_infinite()) {
      if ((this->flags_ & FLAG_SIGN) != (other.flags_ & FLAG_SIGN)) {
        *this = Decimal::nan();
      }

      return *this;
    }

    if (other.is_infinite()) {
      *this = other;
    }

    return *this;
  }

  check_exponent_overflow(this->exponent_, other.exponent_);

  if (other.is_zero()) {
    return *this;
  }

  if (this->is_zero()) {
    *this = other;
    return *this;
  }

  bool left_negative = (this->flags_ & FLAG_SIGN) != 0;
  bool right_negative = (other.flags_ & FLAG_SIGN) != 0;

  bool needs_big = (this->flags_ & FLAG_BIG) || (other.flags_ & FLAG_BIG);
  auto left_coefficient = this->coefficient_;
  auto right_coefficient = other.coefficient_;
  auto result_exponent = std::min(this->exponent_, other.exponent_);

  if (!needs_big) {
    if (this->exponent_ < other.exponent_) {
      auto difference =
          static_cast<std::uint32_t>(other.exponent_ - this->exponent_);
      if (difference <= 18) {
        auto scaled =
            static_cast<sourcemeta::core::uint128_t>(right_coefficient) *
            POWERS_OF_10[difference];
        if (scaled <= static_cast<sourcemeta::core::uint128_t>(COMPACT_MAX)) {
          right_coefficient = static_cast<std::int64_t>(scaled);
        } else {
          needs_big = true;
        }

      } else {
        needs_big = true;
      }

    } else if (other.exponent_ < this->exponent_) {
      auto difference =
          static_cast<std::uint32_t>(this->exponent_ - other.exponent_);
      if (difference <= 18) {
        auto scaled =
            static_cast<sourcemeta::core::uint128_t>(left_coefficient) *
            POWERS_OF_10[difference];
        if (scaled <= static_cast<sourcemeta::core::uint128_t>(COMPACT_MAX)) {
          left_coefficient = static_cast<std::int64_t>(scaled);
        } else {
          needs_big = true;
        }

      } else {
        needs_big = true;
      }
    }
  }

  if (needs_big) {
    auto left_big = coefficient_as_big(this->coefficient_,
                                       this->coefficient_high_, this->flags_);
    auto right_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);
    BigCoefficient::align_exponents(left_big, right_big, this->exponent_,
                                    other.exponent_);
    auto [result_big, result_negative] = BigCoefficient::add_signed(
        left_big, right_big, left_negative, right_negative);
    if (result_big.is_zero()) {
      result_negative = false;
    }

    free_big_coefficient(this->coefficient_, this->flags_);
    store_big_result(this->coefficient_, this->coefficient_high_, this->flags_,
                     std::move(result_big), result_negative);
    this->exponent_ = result_exponent;
    round_to_precision(this->coefficient_, this->coefficient_high_,
                       this->exponent_, this->flags_);
    return *this;
  }

  auto left_signed = left_negative ? -left_coefficient : left_coefficient;
  auto right_signed = right_negative ? -right_coefficient : right_coefficient;
  auto sum = left_signed + right_signed;

  std::int64_t result_coefficient;
  bool result_negative;
  if (sum < 0) {
    result_coefficient = -sum;
    result_negative = true;
  } else {
    result_coefficient = sum;
    result_negative = false;
  }

  if (result_coefficient == 0) {
    result_negative = false;
  }

  this->coefficient_ = result_coefficient;
  this->exponent_ = result_exponent;
  this->flags_ = result_negative ? FLAG_SIGN : 0;

  return *this;
}

auto Decimal::operator-=(const Decimal &other) -> Decimal & {
  return *this += (-other);
}

auto Decimal::operator*=(const Decimal &other) -> Decimal & {
  if (!this->is_finite() || !other.is_finite()) {
    if (this->is_nan() || other.is_nan()) {
      *this = Decimal::nan();
      return *this;
    }

    if (this->is_infinite()) {
      if (other.is_zero()) {
        *this = Decimal::nan();
        return *this;
      }

      if ((other.flags_ & FLAG_SIGN) != 0) {
        this->flags_ ^= FLAG_SIGN;
      }

      return *this;
    }

    if (other.is_infinite()) {
      if (this->is_zero()) {
        *this = Decimal::nan();
        return *this;
      }

      auto sign = ((this->flags_ ^ other.flags_) & FLAG_SIGN);
      *this = other;
      this->flags_ =
          static_cast<std::uint8_t>((this->flags_ & ~FLAG_SIGN) | sign);
      return *this;
    }

    return *this;
  }

  check_exponent_overflow(this->exponent_, other.exponent_);

  bool result_negative = ((this->flags_ ^ other.flags_) & FLAG_SIGN) != 0;
  auto result_exponent_64 =
      static_cast<std::int64_t>(this->exponent_) + other.exponent_;
  if (result_exponent_64 > std::numeric_limits<std::int32_t>::max() ||
      result_exponent_64 < std::numeric_limits<std::int32_t>::min()) {
    throw NumericOverflowError{};
  }

  auto result_exponent = static_cast<std::int32_t>(result_exponent_64);

  if ((this->flags_ & FLAG_BIG) || (other.flags_ & FLAG_BIG)) {
    auto left_big = coefficient_as_big(this->coefficient_,
                                       this->coefficient_high_, this->flags_);
    auto right_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);
    auto product = left_big.multiply(right_big);
    free_big_coefficient(this->coefficient_, this->flags_);
    store_big_result(this->coefficient_, this->coefficient_high_, this->flags_,
                     std::move(product), result_negative);
    this->exponent_ = result_exponent;
    round_to_precision(this->coefficient_, this->coefficient_high_,
                       this->exponent_, this->flags_);
    return *this;
  }

  auto product = static_cast<sourcemeta::core::uint128_t>(this->coefficient_) *
                 static_cast<sourcemeta::core::uint128_t>(other.coefficient_);

  if (product <= static_cast<sourcemeta::core::uint128_t>(COMPACT_MAX)) {
    this->coefficient_ = static_cast<std::int64_t>(product);
    this->exponent_ = result_exponent;
    this->flags_ = result_negative ? FLAG_SIGN : 0;
    if (this->coefficient_ == 0) {
      this->flags_ = 0;
    }

  } else {
    auto left_big = coefficient_as_big(this->coefficient_,
                                       this->coefficient_high_, this->flags_);
    auto right_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);
    auto result = left_big.multiply(right_big);
    store_big_result(this->coefficient_, this->coefficient_high_, this->flags_,
                     std::move(result), result_negative);
    this->exponent_ = result_exponent;
  }

  round_to_precision(this->coefficient_, this->coefficient_high_,
                     this->exponent_, this->flags_);
  return *this;
}

auto Decimal::operator/=(const Decimal &other) -> Decimal & {
  if (this->is_nan() || other.is_nan()) {
    *this = Decimal::nan();
    return *this;
  }

  if (this->is_infinite() && other.is_infinite()) {
    throw NumericInvalidOperationError{};
  }

  if (this->is_infinite()) {
    if ((other.flags_ & FLAG_SIGN) != 0) {
      this->flags_ ^= FLAG_SIGN;
    }

    return *this;
  }

  if (other.is_zero()) {
    if (this->is_zero()) {
      throw NumericInvalidOperationError{};
    }

    throw NumericDivisionByZeroError{};
  }

  if (other.is_infinite()) {
    bool result_negative = ((this->flags_ ^ other.flags_) & FLAG_SIGN) != 0;
    free_big_coefficient(this->coefficient_, this->flags_);
    this->coefficient_ = 0;
    this->exponent_ = 0;
    this->flags_ = result_negative ? FLAG_SIGN : 0;
    return *this;
  }

  bool result_negative = ((this->flags_ ^ other.flags_) & FLAG_SIGN) != 0;

  auto dividend_big = coefficient_as_big(this->coefficient_,
                                         this->coefficient_high_, this->flags_);
  auto divisor_big = coefficient_as_big(other.coefficient_,
                                        other.coefficient_high_, other.flags_);

  auto scaled = dividend_big.multiply_pow10(WORKING_PRECISION);
  auto [quotient, remainder] = scaled.divide_modulo(divisor_big);

  free_big_coefficient(this->coefficient_, this->flags_);
  store_big_result(this->coefficient_, this->coefficient_high_, this->flags_,
                   std::move(quotient), result_negative);
  if (this->coefficient_ == 0 && !(this->flags_ & FLAG_BIG)) {
    this->flags_ = 0;
  }

  this->exponent_ = this->exponent_ - other.exponent_ - WORKING_PRECISION;

  round_to_precision(this->coefficient_, this->coefficient_high_,
                     this->exponent_, this->flags_);
  return *this;
}

auto Decimal::operator%=(const Decimal &other) -> Decimal & {
  if (this->is_nan() || other.is_nan()) {
    *this = Decimal::nan();
    return *this;
  }

  if (other.is_zero()) {
    throw NumericInvalidOperationError{};
  }

  if (this->is_infinite()) {
    *this = Decimal::nan();
    return *this;
  }

  if (other.is_infinite()) {
    return *this;
  }

  Decimal quotient{*this};
  quotient /= other;

  if (quotient.is_finite() && !quotient.is_zero()) {
    if (quotient.exponent_ < 0) {
      if (quotient.flags_ & FLAG_BIG) {
        auto digit_string = coefficient_to_digit_string(
            quotient.coefficient_, quotient.coefficient_high_, quotient.flags_);
        auto number_of_digits = static_cast<std::int32_t>(digit_string.size());
        auto digits_to_remove = -quotient.exponent_;
        if (digits_to_remove >= number_of_digits) {
          quotient = Decimal{};
        } else {
          auto integer_string = digit_string.substr(
              0, static_cast<std::size_t>(number_of_digits - digits_to_remove));
          auto old_sign =
              static_cast<std::uint8_t>(quotient.flags_ & FLAG_SIGN);
          free_big_coefficient(quotient.coefficient_, quotient.flags_);
          quotient = Decimal{integer_string};
          quotient.flags_ =
              static_cast<std::uint8_t>(quotient.flags_ | old_sign);
        }

      } else {
        auto coefficient = quotient.coefficient_;
        auto exponent = quotient.exponent_;
        while (exponent < 0 && coefficient > 0) {
          coefficient /= 10;
          exponent++;
        }

        if (exponent < 0) {
          quotient = Decimal{};
        } else {
          quotient.coefficient_ = coefficient;
          quotient.exponent_ = exponent;
        }
      }
    }
  }

  Decimal product{quotient};
  product *= other;
  *this -= product;

  return *this;
}

auto Decimal::operator+(const Decimal &other) const -> Decimal {
  Decimal result{*this};
  result += other;
  return result;
}

auto Decimal::operator-(const Decimal &other) const -> Decimal {
  Decimal result{*this};
  result -= other;
  return result;
}

auto Decimal::operator*(const Decimal &other) const -> Decimal {
  Decimal result{*this};
  result *= other;
  return result;
}

auto Decimal::operator/(const Decimal &other) const -> Decimal {
  Decimal result{*this};
  result /= other;
  return result;
}

auto Decimal::operator%(const Decimal &other) const -> Decimal {
  Decimal result{*this};
  result %= other;
  return result;
}

auto Decimal::operator-() const -> Decimal {
  Decimal result{*this};
  result.flags_ ^= FLAG_SIGN;
  return result;
}

auto Decimal::operator+() const -> Decimal { return *this; }

auto Decimal::operator++() -> Decimal & {
  *this += Decimal{1};
  return *this;
}

auto Decimal::operator++(int) -> Decimal {
  Decimal result{*this};
  ++(*this);
  return result;
}

auto Decimal::operator--() -> Decimal & {
  *this -= Decimal{1};
  return *this;
}

auto Decimal::operator--(int) -> Decimal {
  Decimal result{*this};
  --(*this);
  return result;
}

} // namespace sourcemeta::core
