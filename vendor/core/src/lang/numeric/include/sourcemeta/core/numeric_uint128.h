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
  std::uint64_t low;
  std::uint64_t high;

  uint128_t() noexcept : low{0}, high{0} {}
  uint128_t(int value) noexcept
      : low{static_cast<std::uint64_t>(value)},
        high{value < 0 ? UINT64_MAX : 0} {}
  uint128_t(unsigned int value) noexcept
      : low{static_cast<std::uint64_t>(value)}, high{0} {}
  uint128_t(std::uint64_t value) noexcept : low{value}, high{0} {}
  uint128_t(std::int64_t value) noexcept
      : low{static_cast<std::uint64_t>(value)},
        high{value < 0 ? UINT64_MAX : 0} {}
  uint128_t(std::uint64_t high_part, std::uint64_t low_part) noexcept
      : low{low_part}, high{high_part} {}

  explicit operator std::uint64_t() const noexcept { return this->low; }
  explicit operator std::int64_t() const noexcept {
    return static_cast<std::int64_t>(this->low);
  }

  explicit operator bool() const noexcept {
    return this->low != 0 || this->high != 0;
  }

  auto operator+=(const uint128_t &other) noexcept -> uint128_t & {
    auto old_low = this->low;
    this->low += other.low;
    this->high += other.high + (this->low < old_low ? 1 : 0);
    return *this;
  }

  auto operator*=(const uint128_t &other) noexcept -> uint128_t & {
    *this = *this * other;
    return *this;
  }

  friend auto operator+(uint128_t left, const uint128_t &right) noexcept
      -> uint128_t {
    left += right;
    return left;
  }

  friend auto operator*(const uint128_t &left, const uint128_t &right) noexcept
      -> uint128_t {
    std::uint64_t result_high;
#if defined(_MSC_VER)
    auto result_low = _umul128(left.low, right.low, &result_high);
#else
    const std::uint64_t left_low{left.low & 0xFFFFFFFF};
    const std::uint64_t left_high{left.low >> 32};
    const std::uint64_t right_low{right.low & 0xFFFFFFFF};
    const std::uint64_t right_high{right.low >> 32};
    const std::uint64_t cross_1{left_high * right_low};
    const std::uint64_t cross_2{left_low * right_high};
    const std::uint64_t mid{cross_1 + (left_low * right_low >> 32)};
    const std::uint64_t mid_2{(mid & 0xFFFFFFFF) + cross_2};
    result_high = left_high * right_high + (mid >> 32) + (mid_2 >> 32);
    auto result_low = (mid_2 << 32) | ((left_low * right_low) & 0xFFFFFFFF);
#endif
    result_high += left.low * right.high + left.high * right.low;
    return {result_high, result_low};
  }

  friend auto operator/(const uint128_t &dividend,
                        std::uint64_t divisor) noexcept -> uint128_t {
    if (dividend.high == 0) {
      return {0, dividend.low / divisor};
    }

    auto quotient_high = dividend.high / divisor;
    auto remainder_high = dividend.high % divisor;
    std::uint64_t quotient_low;
    std::uint64_t remainder;
#if defined(_MSC_VER)
    quotient_low = _udiv128(remainder_high, dividend.low, divisor, &remainder);
#else
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
#endif
    return {quotient_high, quotient_low};
  }

  friend auto operator/(const uint128_t &dividend,
                        const uint128_t &divisor) noexcept -> uint128_t {
    assert(divisor.high == 0);
    return dividend / divisor.low;
  }

  friend auto operator%(const uint128_t &dividend,
                        std::uint64_t divisor) noexcept -> uint128_t {
    if (dividend.high == 0) {
      return {0, dividend.low % divisor};
    }

    auto remainder_high = dividend.high % divisor;
    std::uint64_t remainder;
#if defined(_MSC_VER)
    _udiv128(remainder_high, dividend.low, divisor, &remainder);
#else
    remainder = remainder_high;
    for (int bit = 63; bit >= 0; --bit) {
      const auto carry = remainder >> 63;
      remainder = (remainder << 1) | ((dividend.low >> bit) & 1);
      if (carry || remainder >= divisor) {
        remainder -= divisor;
      }
    }
#endif
    return {0, remainder};
  }

  friend auto operator%(const uint128_t &dividend,
                        const uint128_t &divisor) noexcept -> uint128_t {
    assert(divisor.high == 0);
    return dividend % divisor.low;
  }

  friend auto operator<(const uint128_t &left, const uint128_t &right) noexcept
      -> bool {
    return left.high < right.high ||
           (left.high == right.high && left.low < right.low);
  }

  friend auto operator>(const uint128_t &left, const uint128_t &right) noexcept
      -> bool {
    return right < left;
  }

  friend auto operator<=(const uint128_t &left, const uint128_t &right) noexcept
      -> bool {
    return !(right < left);
  }

  friend auto operator>=(const uint128_t &left, const uint128_t &right) noexcept
      -> bool {
    return !(left < right);
  }

  friend auto operator==(const uint128_t &left, const uint128_t &right) noexcept
      -> bool {
    return left.high == right.high && left.low == right.low;
  }

  friend auto operator!=(const uint128_t &left, const uint128_t &right) noexcept
      -> bool {
    return !(left == right);
  }

  friend auto operator<<(const uint128_t &value, int shift) noexcept
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

  friend auto operator>>(const uint128_t &value, int shift) noexcept
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

  friend auto operator|(const uint128_t &left, const uint128_t &right) noexcept
      -> uint128_t {
    return {left.high | right.high, left.low | right.low};
  }

  auto operator|=(const uint128_t &other) noexcept -> uint128_t & {
    this->high |= other.high;
    this->low |= other.low;
    return *this;
  }

  friend auto operator&(const uint128_t &left, const uint128_t &right) noexcept
      -> uint128_t {
    return {left.high & right.high, left.low & right.low};
  }
};
#endif

} // namespace sourcemeta::core

#endif
