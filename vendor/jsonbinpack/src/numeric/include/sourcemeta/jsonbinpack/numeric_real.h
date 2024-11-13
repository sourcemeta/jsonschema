#ifndef SOURCEMETA_JSONBINPACK_NUMERIC_REAL_H_
#define SOURCEMETA_JSONBINPACK_NUMERIC_REAL_H_

#include <cassert>  // assert
#include <cmath>    // std::modf, std::floor, std::isfinite
#include <concepts> // std::floating_point, std::integral

namespace sourcemeta::jsonbinpack {

// IEEE764 floating-point encoding is not precise. Some real numbers
// cannot be represented directly and thus approximations must be
// used. Here, we abuse those imprecision characteristics for
// space-efficiency by performing rounding on a very, very low
// threshold.
/// @ingroup numeric
template <std::floating_point Real>
constexpr auto correct_ieee764(const Real value) -> Real {
  assert(std::isfinite(value));
  const Real IEEE764_CORRECTION_THRESHOLD{0.000000001};
  const Real base{std::floor(value)};
  const Real next{base + 1};
  if (next - value <= IEEE764_CORRECTION_THRESHOLD) {
    return next;
  } else if (value - base <= IEEE764_CORRECTION_THRESHOLD) {
    return base;
  } else {
    return value;
  }
}

/// @ingroup numeric
template <std::integral Integer, std::floating_point Real>
constexpr auto real_digits(Real value, std::uint64_t *point_position)
    -> Integer {
  assert(std::isfinite(value));
  Real integral;
  std::uint64_t shifts{0};

  Real result = std::modf(value, &integral);
  while (result != 0.0) {
    value *= 10;
    shifts += 1;
    result = std::modf(correct_ieee764(value), &integral);
  }

  // This is the point position from right to left
  *point_position = shifts;

  return static_cast<Integer>(std::floor(integral));
}

} // namespace sourcemeta::jsonbinpack

#endif
