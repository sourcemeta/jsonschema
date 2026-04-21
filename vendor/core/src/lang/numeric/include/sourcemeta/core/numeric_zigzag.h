#ifndef SOURCEMETA_CORE_NUMERIC_ZIGZAG_H_
#define SOURCEMETA_CORE_NUMERIC_ZIGZAG_H_

#include <sourcemeta/core/numeric_decimal.h>

#include <cassert>  // assert
#include <concepts> // std::same_as
#include <cstdint>  // std::uint64_t, std::int64_t

namespace sourcemeta::core {

/// @ingroup numeric
/// Encode a signed integer into an unsigned integer using zigzag encoding,
/// where the sign information is stored in the least significant bit
template <typename T> auto zigzag_encode(const T &value) {
  if constexpr (std::same_as<T, Decimal>) {
    assert(value.is_integral());
    if (value >= Decimal{0}) {
      return value * Decimal{2};
    }
    const Decimal absolute{value.is_signed() ? -value : value};
    return (absolute * Decimal{2}) - Decimal{1};
  } else {
    const auto signed_value{static_cast<std::int64_t>(value)};
    if (signed_value >= 0) {
      return static_cast<std::uint64_t>(signed_value) * 2;
    }
    // Negate in unsigned to avoid UB for INT64_MIN
    return (static_cast<std::uint64_t>(0) -
            static_cast<std::uint64_t>(signed_value)) *
               2 -
           1;
  }
}

/// @ingroup numeric
/// Decode a zigzag-encoded unsigned integer back into a signed integer
template <typename T> auto zigzag_decode(const T &value) {
  if constexpr (std::same_as<T, Decimal>) {
    assert(value.is_integral());
    assert(value >= Decimal{0});
    if (value % Decimal{2} == Decimal{0}) {
      return value.divide_integer(Decimal{2});
    }
    return -((value + Decimal{1}).divide_integer(Decimal{2}));
  } else {
    const auto unsigned_value{static_cast<std::uint64_t>(value)};
    if (unsigned_value % 2 == 0) {
      return static_cast<std::int64_t>(unsigned_value / 2);
    }
    // Use bitwise complement to avoid overflow for UINT64_MAX
    // `~(x / 2)` == `-(x / 2 + 1)` in two's complement
    return static_cast<std::int64_t>(~(unsigned_value / 2));
  }
}

} // namespace sourcemeta::core

#endif
