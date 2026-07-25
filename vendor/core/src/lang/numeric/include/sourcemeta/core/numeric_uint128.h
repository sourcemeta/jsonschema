#ifndef SOURCEMETA_CORE_NUMERIC_UINT128_H_
#define SOURCEMETA_CORE_NUMERIC_UINT128_H_

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <cassert> // assert
#include <climits> // UINT64_MAX
#include <cstdint> // std::uint64_t, std::int64_t

#if defined(_MSC_VER)
#include <intrin.h> // _umul128, _udiv128
#endif

namespace sourcemeta::core {

/// @ingroup numeric
/// Cross-platform 128-bit unsigned integer type.
#if defined(__SIZEOF_INT128__) || defined(__clang__)
using uint128_t = __uint128_t;
#else
// We keep the full implementation header-only to allow the compiler to better
// inline
struct uint128_t {
  /// The lower 64 bits of the value.
  std::uint64_t low;
  /// The upper 64 bits of the value.
  std::uint64_t high;

  constexpr uint128_t() noexcept : low{0}, high{0} {}
  /// Construct from a signed integer, sign-extending negative values.
  constexpr uint128_t(int value) noexcept
      : low{static_cast<std::uint64_t>(value)},
        high{value < 0 ? UINT64_MAX : 0} {}
  /// Construct from an unsigned integer.
  constexpr uint128_t(unsigned int value) noexcept
      : low{static_cast<std::uint64_t>(value)}, high{0} {}
  /// Construct from a 64-bit unsigned integer.
  constexpr uint128_t(std::uint64_t value) noexcept : low{value}, high{0} {}
  /// Construct from a 64-bit signed integer, sign-extending negative values.
  constexpr uint128_t(std::int64_t value) noexcept
      : low{static_cast<std::uint64_t>(value)},
        high{value < 0 ? UINT64_MAX : 0} {}
  /// Construct from separate upper and lower 64-bit halves.
  constexpr uint128_t(std::uint64_t high_part, std::uint64_t low_part) noexcept
      : low{low_part}, high{high_part} {}

  constexpr explicit operator std::uint64_t() const noexcept {
    return this->low;
  }
  constexpr explicit operator std::int64_t() const noexcept {
    return static_cast<std::int64_t>(this->low);
  }

  constexpr explicit operator bool() const noexcept {
    return this->low != 0 || this->high != 0;
  }

  constexpr auto operator+=(const uint128_t &other) noexcept -> uint128_t & {
    auto old_low = this->low;
    this->low += other.low;
    this->high += other.high + (this->low < old_low ? 1 : 0);
    return *this;
  }

  constexpr auto operator-=(const uint128_t &other) noexcept -> uint128_t & {
    const auto old_low = this->low;
    this->low -= other.low;
    this->high -= other.high + (old_low < other.low ? 1 : 0);
    return *this;
  }

  constexpr auto operator*=(const uint128_t &other) noexcept -> uint128_t & {
    *this = *this * other;
    return *this;
  }

  constexpr auto operator%=(const uint128_t &other) noexcept -> uint128_t & {
    *this = *this % other;
    return *this;
  }

  friend constexpr auto operator+(uint128_t left,
                                  const uint128_t &right) noexcept
      -> uint128_t {
    left += right;
    return left;
  }

  friend constexpr auto operator-(uint128_t left,
                                  const uint128_t &right) noexcept
      -> uint128_t {
    left -= right;
    return left;
  }

  friend constexpr auto operator*(const uint128_t &left,
                                  const uint128_t &right) noexcept
      -> uint128_t {
    std::uint64_t result_high;
    std::uint64_t result_low;
    // The intrinsic is not usable during constant evaluation, so fall through
    // to the portable computation, which yields the same value
#if defined(_MSC_VER)
    if !consteval {
      result_low = _umul128(left.low, right.low, &result_high);
    } else
#endif
    {
      const std::uint64_t left_low{left.low & 0xFFFFFFFF};
      const std::uint64_t left_high{left.low >> 32};
      const std::uint64_t right_low{right.low & 0xFFFFFFFF};
      const std::uint64_t right_high{right.low >> 32};
      const std::uint64_t cross_1{left_high * right_low};
      const std::uint64_t cross_2{left_low * right_high};
      const std::uint64_t mid{cross_1 + (left_low * right_low >> 32)};
      const std::uint64_t mid_2{(mid & 0xFFFFFFFF) + cross_2};
      result_high = left_high * right_high + (mid >> 32) + (mid_2 >> 32);
      result_low = (mid_2 << 32) | ((left_low * right_low) & 0xFFFFFFFF);
    }
    result_high += left.low * right.high + left.high * right.low;
    return {result_high, result_low};
  }

  friend constexpr auto operator/(const uint128_t &dividend,
                                  std::uint64_t divisor) noexcept -> uint128_t {
    if (dividend.high == 0) {
      return {0, dividend.low / divisor};
    }

    auto quotient_high = dividend.high / divisor;
    auto remainder_high = dividend.high % divisor;
    std::uint64_t quotient_low;
    // The high half of the partial dividend must stay below the divisor, or the
    // hardware 128-by-64 division would overflow its 64-bit quotient. The
    // remainder of the earlier division guarantees this
    assert(remainder_high < divisor);
    // The intrinsic is not usable during constant evaluation, so fall through
    // to the portable computation, which yields the same value
#if defined(_MSC_VER)
    if !consteval {
      std::uint64_t remainder;
      quotient_low =
          _udiv128(remainder_high, dividend.low, divisor, &remainder);
    } else
#endif
    {
      quotient_low = 0;
      auto current_remainder = remainder_high;
      for (int bit = 63; bit >= 0; --bit) {
        const auto carry = current_remainder >> 63;
        current_remainder =
            (current_remainder << 1) | ((dividend.low >> bit) & 1);
        if (carry || current_remainder >= divisor) {
          current_remainder -= divisor;
          quotient_low |= static_cast<std::uint64_t>(1) << bit;
        }
      }
    }
    return {quotient_high, quotient_low};
  }

  friend constexpr auto operator/(const uint128_t &dividend,
                                  const uint128_t &divisor) noexcept
      -> uint128_t {
    assert(divisor.high == 0);
    return dividend / divisor.low;
  }

  friend constexpr auto operator%(const uint128_t &dividend,
                                  std::uint64_t divisor) noexcept -> uint128_t {
    if (dividend.high == 0) {
      return {0, dividend.low % divisor};
    }

    auto remainder_high = dividend.high % divisor;
    std::uint64_t remainder;
    // The high half of the partial dividend must stay below the divisor, or the
    // hardware 128-by-64 division would overflow its 64-bit quotient. The
    // remainder of the earlier division guarantees this
    assert(remainder_high < divisor);
    // The intrinsic is not usable during constant evaluation, so fall through
    // to the portable computation, which yields the same value
#if defined(_MSC_VER)
    if !consteval {
      _udiv128(remainder_high, dividend.low, divisor, &remainder);
    } else
#endif
    {
      remainder = remainder_high;
      for (int bit = 63; bit >= 0; --bit) {
        const auto carry = remainder >> 63;
        remainder = (remainder << 1) | ((dividend.low >> bit) & 1);
        if (carry || remainder >= divisor) {
          remainder -= divisor;
        }
      }
    }
    return {0, remainder};
  }

  friend constexpr auto operator%(const uint128_t &dividend,
                                  const uint128_t &divisor) noexcept
      -> uint128_t {
    assert(divisor.high == 0);
    return dividend % divisor.low;
  }

  friend constexpr auto operator<(const uint128_t &left,
                                  const uint128_t &right) noexcept -> bool {
    return left.high < right.high ||
           (left.high == right.high && left.low < right.low);
  }

  friend constexpr auto operator>(const uint128_t &left,
                                  const uint128_t &right) noexcept -> bool {
    return right < left;
  }

  friend constexpr auto operator<=(const uint128_t &left,
                                   const uint128_t &right) noexcept -> bool {
    return !(right < left);
  }

  friend constexpr auto operator>=(const uint128_t &left,
                                   const uint128_t &right) noexcept -> bool {
    return !(left < right);
  }

  friend constexpr auto operator==(const uint128_t &left,
                                   const uint128_t &right) noexcept -> bool {
    return left.high == right.high && left.low == right.low;
  }

  friend constexpr auto operator!=(const uint128_t &left,
                                   const uint128_t &right) noexcept -> bool {
    return !(left == right);
  }

  friend constexpr auto operator<<(const uint128_t &value, int shift) noexcept
      -> uint128_t {
    if (shift == 0) {
      return value;
    } else if (shift >= 128) {
      return {0, 0};
    } else if (shift >= 64) {
      return {value.low << (shift - 64), 0};
    } else {
      return {(value.high << shift) | (value.low >> (64 - shift)),
              value.low << shift};
    }
  }

  friend constexpr auto operator>>(const uint128_t &value, int shift) noexcept
      -> uint128_t {
    if (shift == 0) {
      return value;
    } else if (shift >= 128) {
      return {0, 0};
    } else if (shift >= 64) {
      return {0, value.high >> (shift - 64)};
    } else {
      return {value.high >> shift,
              (value.low >> shift) | (value.high << (64 - shift))};
    }
  }

  friend constexpr auto operator|(const uint128_t &left,
                                  const uint128_t &right) noexcept
      -> uint128_t {
    return {left.high | right.high, left.low | right.low};
  }

  constexpr auto operator|=(const uint128_t &other) noexcept -> uint128_t & {
    this->high |= other.high;
    this->low |= other.low;
    return *this;
  }

  friend constexpr auto operator&(const uint128_t &left,
                                  const uint128_t &right) noexcept
      -> uint128_t {
    return {left.high & right.high, left.low & right.low};
  }
};
#endif

} // namespace sourcemeta::core

#endif
