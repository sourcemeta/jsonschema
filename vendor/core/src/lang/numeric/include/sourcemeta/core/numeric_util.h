#ifndef SOURCEMETA_CORE_NUMERIC_UTIL_H_
#define SOURCEMETA_CORE_NUMERIC_UTIL_H_

#include <sourcemeta/core/numeric_decimal.h>

#include <cassert>  // assert
#include <cmath>    // std::modf, std::floor, std::isfinite
#include <concepts> // std::floating_point, std::integral, std::same_as
#include <cstdint>  // std::uint8_t, std::int64_t, std::uint64_t
#include <limits>   // std::numeric_limits

namespace sourcemeta::core {

/// @ingroup numeric
/// Convert a value to a Decimal, returning a copy if it is already one
template <typename T> auto to_decimal(const T &value) -> Decimal {
  if constexpr (std::same_as<T, Decimal>) {
    return value;
  } else {
    return Decimal{value};
  }
}

/// @ingroup numeric
/// A type that is either a Decimal or a built-in integral type
template <typename T>
concept decimal_or_integral = std::same_as<T, Decimal> || std::integral<T>;

/// @ingroup numeric
/// True when at least one of the given types is a Decimal
template <typename... Ts>
concept any_decimal = (std::same_as<Ts, Decimal> || ...);

/// @ingroup numeric
/// Check whether a value fits in an unsigned 8-bit byte
template <typename T> constexpr auto is_byte(const T &value) -> bool {
  if constexpr (std::same_as<T, Decimal>) {
    return value.is_finite() && value.is_integral() && value >= Decimal{0} &&
           value <= Decimal{255};
  } else {
    return value >= 0 && value <= std::numeric_limits<std::uint8_t>::max();
  }
}

/// @ingroup numeric
/// Compute the floor of the division of two integral values. Supports
/// primitive integers, Decimal operands, and mixed combinations.
template <typename Dividend, typename Divisor>
  requires decimal_or_integral<Dividend> && decimal_or_integral<Divisor>
auto divide_floor(const Dividend &dividend, const Divisor &divisor) {
  if constexpr (any_decimal<Dividend, Divisor>) {
    Decimal decimal_dividend{to_decimal(dividend)};
    const Decimal decimal_divisor{to_decimal(divisor)};
    assert(decimal_dividend.is_integral());
    assert(decimal_divisor.is_integral());
    assert(decimal_divisor > Decimal{0});
    if (decimal_divisor == Decimal{1}) {
      return decimal_dividend;
    } else if (decimal_dividend >= Decimal{0}) {
      return decimal_dividend.divide_integer(decimal_divisor);
    } else {
      const Decimal absolute_dividend{
          decimal_dividend.is_signed() ? -decimal_dividend : decimal_dividend};
      const Decimal quotient{absolute_dividend.divide_integer(decimal_divisor)};
      if (absolute_dividend % decimal_divisor == Decimal{0}) {
        return -quotient;
      } else {
        return -(quotient + Decimal{1});
      }
    }
  } else {
    const auto signed_dividend{static_cast<std::int64_t>(dividend)};
    const auto unsigned_divisor{static_cast<std::uint64_t>(divisor)};
    assert(unsigned_divisor > 0);
    if (unsigned_divisor == 1) {
      return signed_dividend;
    } else if (signed_dividend >= 0) {
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(signed_dividend) / unsigned_divisor);
    } else {
      // Negate in unsigned to avoid UB for INT64_MIN
      const std::uint64_t absolute_dividend{
          static_cast<std::uint64_t>(0) -
          static_cast<std::uint64_t>(signed_dividend)};
      return -(static_cast<std::int64_t>(
          1 + ((absolute_dividend - 1) / unsigned_divisor)));
    }
  }
}

/// @ingroup numeric
/// Compute the ceiling of the division of two integral values. Supports
/// primitive integers, Decimal operands, and mixed combinations.
template <typename Dividend, typename Divisor>
  requires decimal_or_integral<Dividend> && decimal_or_integral<Divisor>
auto divide_ceil(const Dividend &dividend, const Divisor &divisor) {
  if constexpr (any_decimal<Dividend, Divisor>) {
    Decimal decimal_dividend{to_decimal(dividend)};
    const Decimal decimal_divisor{to_decimal(divisor)};
    assert(decimal_dividend.is_integral());
    assert(decimal_divisor.is_integral());
    assert(decimal_divisor > Decimal{0});
    if (decimal_divisor == Decimal{1}) {
      return decimal_dividend;
    } else if (decimal_dividend >= Decimal{0}) {
      const Decimal quotient{decimal_dividend.divide_integer(decimal_divisor)};
      if (decimal_dividend % decimal_divisor == Decimal{0}) {
        return quotient;
      } else {
        return quotient + Decimal{1};
      }
    } else {
      const Decimal absolute_dividend{
          decimal_dividend.is_signed() ? -decimal_dividend : decimal_dividend};
      return -(absolute_dividend.divide_integer(decimal_divisor));
    }
  } else {
    const auto signed_dividend{static_cast<std::int64_t>(dividend)};
    const auto unsigned_divisor{static_cast<std::uint64_t>(divisor)};
    assert(unsigned_divisor > 0);
    if (unsigned_divisor == 1) {
      return signed_dividend;
    } else if (signed_dividend >= 0) {
      if (static_cast<std::uint64_t>(signed_dividend) + unsigned_divisor <
          unsigned_divisor) {
        return static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(signed_dividend) / unsigned_divisor) +
            1 - (1 / unsigned_divisor));
      } else {
        return static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(signed_dividend) + unsigned_divisor -
             1) /
            unsigned_divisor);
      }
    } else {
      // Negate in unsigned to avoid UB for INT64_MIN
      return -(static_cast<std::int64_t>(
          (static_cast<std::uint64_t>(0) -
           static_cast<std::uint64_t>(signed_dividend)) /
          unsigned_divisor));
    }
  }
}

/// @ingroup numeric
/// Count the number of multiples of a given multiplier that fall within
/// the closed range [minimum, maximum]. Supports primitive integers,
/// Decimal operands, and mixed combinations.
template <typename Minimum, typename Maximum, typename Multiplier>
  requires decimal_or_integral<Minimum> && decimal_or_integral<Maximum> &&
           decimal_or_integral<Multiplier>
auto count_multiples(const Minimum &minimum, const Maximum &maximum,
                     const Multiplier &multiplier) {
  if constexpr (any_decimal<Minimum, Maximum, Multiplier>) {
    const Decimal decimal_minimum{to_decimal(minimum)};
    const Decimal decimal_maximum{to_decimal(maximum)};
    const Decimal decimal_multiplier{to_decimal(multiplier)};
    assert(decimal_minimum.is_integral());
    assert(decimal_maximum.is_integral());
    assert(decimal_multiplier.is_integral());
    assert(decimal_minimum <= decimal_maximum);
    assert(decimal_multiplier > Decimal{0});
    return divide_floor(decimal_maximum, decimal_multiplier) -
           divide_floor(decimal_minimum - Decimal{1}, decimal_multiplier);
  } else {
    const auto signed_minimum{static_cast<std::int64_t>(minimum)};
    const auto signed_maximum{static_cast<std::int64_t>(maximum)};
    const auto signed_multiplier{static_cast<std::int64_t>(multiplier)};
    assert(signed_minimum <= signed_maximum);
    assert(signed_multiplier > 0);
    return static_cast<std::uint64_t>(
        divide_floor(signed_maximum,
                     static_cast<std::uint64_t>(signed_multiplier)) -
        divide_floor(signed_minimum - 1,
                     static_cast<std::uint64_t>(signed_multiplier)));
  }
}

/// @ingroup numeric
/// The maximum value representable by an unsigned integer of T bits
template <unsigned int T>
constexpr auto uint_max = [] {
  static_assert(T > 0 && T < 64, "uint_max<T> requires 0 < T < 64");
  return (std::uint64_t{1} << T) - 1;
}();

/// @ingroup numeric
/// Check whether a value falls within the closed range [lower, higher]
/// using signed 64-bit bounds
template <typename T>
constexpr auto is_within(const T &value, const std::int64_t lower,
                         const std::int64_t higher) noexcept -> bool {
  return value >= lower && value <= higher;
}

/// @ingroup numeric
/// Check whether a value falls within the closed range [lower, higher]
/// using unsigned 64-bit bounds
template <typename T>
constexpr auto is_within(const T &value, const std::uint64_t lower,
                         const std::uint64_t higher) noexcept -> bool {
  if (value >= 0) {
    return static_cast<std::uint64_t>(value) >= lower &&
           static_cast<std::uint64_t>(value) <= higher;
  } else {
    return false;
  }
}

/// @ingroup numeric
/// Check whether a Decimal value falls within the closed range
/// [lower, higher]
inline auto is_within(const Decimal &value, const Decimal &lower,
                      const Decimal &higher) -> bool {
  return value >= lower && value <= higher;
}

/// @ingroup numeric
/// Compute the absolute value of an integer or Decimal
template <typename T> auto abs(const T &value) {
  if constexpr (std::same_as<T, Decimal>) {
    return value.is_signed() ? -value : value;
  } else {
    if (value < 0) {
      // Negate in unsigned to avoid UB for INT64_MIN
      return static_cast<std::uint64_t>(0) - static_cast<std::uint64_t>(value);
    } else {
      return static_cast<std::uint64_t>(value);
    }
  }
}

/// @ingroup numeric
/// Find the smallest exponent in [exponent_start, exponent_end] such that
/// base raised to the next power exceeds the given value, i.e.
/// `(base ^ exponent) <= value < (base ^ (exponent + 1))`
constexpr auto closest_smallest_exponent(const std::uint64_t value,
                                         const std::uint8_t base,
                                         const std::uint8_t exponent_start,
                                         const std::uint8_t exponent_end)
    -> std::uint8_t {
  assert(exponent_start <= exponent_end);
  std::uint64_t result{base};
  for (std::uint8_t exponent{1}; exponent < exponent_end; exponent++) {
    const std::uint64_t next{result * base};
    if (next > value && exponent >= exponent_start) {
      return exponent;
    } else {
      result = next;
    }
  }

  assert(result <= value);
  return exponent_end;
}

/// @ingroup numeric
/// Correct IEEE 754 floating-point imprecision by rounding values that
/// are extremely close to the nearest integer
template <std::floating_point Real>
constexpr auto correct_ieee754(const Real value) -> Real {
  assert(std::isfinite(value));
  const Real threshold{static_cast<Real>(0.000000001)};
  const Real base{std::floor(value)};
  const Real next{base + 1};
  if (next - value <= threshold) {
    return next;
  } else if (value - base <= threshold) {
    return base;
  } else {
    return value;
  }
}

/// @ingroup numeric
/// Extract the integer digits and decimal point position from a
/// floating-point value. The point_position output indicates how many
/// digits from the right the decimal point sits.
template <std::integral Integer, std::floating_point Real>
constexpr auto real_digits(Real value, std::uint64_t &point_position)
    -> Integer {
  assert(std::isfinite(value));
  Real integral_part;
  std::uint64_t shifts{0};

  Real fractional_part{std::modf(value, &integral_part)};
  while (fractional_part != 0.0) {
    value *= 10;
    shifts += 1;
    fractional_part = std::modf(correct_ieee754(value), &integral_part);
  }

  point_position = shifts;
  return static_cast<Integer>(std::floor(integral_part));
}

} // namespace sourcemeta::core

#endif
