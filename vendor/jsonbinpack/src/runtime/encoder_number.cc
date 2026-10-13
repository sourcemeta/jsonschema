#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <sourcemeta/core/numeric.h>

#include <cassert>      // assert
#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <cstdint>      // std::int64_t, std::uint64_t
#include <limits>       // std::numeric_limits
#include <string_view>  // std::string_view
#include <system_error> // std::errc

namespace {

#ifndef NDEBUG
// The widest coefficient that still fits in a signed 64-bit integer
constexpr std::size_t MAXIMUM_COEFFICIENT_DIGITS{19};
#endif

// The coefficient and the base-10 exponent of an arbitrary precision decimal.
// Core keeps both of them private and only spells them out through the
// scientific form, as `[-]D[.DDDD]e(+|-)ADJUSTED`, whose exponent is adjusted
// to count from the leading digit rather than from the last one
auto decimal_coefficient(const sourcemeta::core::Decimal &value,
                         std::int64_t &exponent) -> std::int64_t {
  const auto scientific{value.reduce().to_scientific_string()};
  const std::string_view view{scientific};
  const auto marker{view.find('e')};
  assert(marker != std::string_view::npos);

  auto mantissa{view.substr(0, marker)};
  const auto negative{mantissa.front() == '-'};
  if (negative) {
    mantissa.remove_prefix(1);
  }

  // Every digit past the leading one sits after a decimal mark that the
  // adjusted exponent already accounts for
  const auto point{mantissa.find('.')};
  const auto leading{mantissa.substr(
      0, point == std::string_view::npos ? mantissa.size() : point)};
  const auto trailing{point == std::string_view::npos
                          ? std::string_view{}
                          : mantissa.substr(point + 1)};

  const auto digits{leading.size() + trailing.size()};
  assert(digits >= 1);
  assert(digits <= MAXIMUM_COEFFICIENT_DIGITS);

  std::uint64_t magnitude{0};
  for (const auto digit : leading) {
    magnitude = (magnitude * 10) + static_cast<std::uint64_t>(digit - '0');
  }
  for (const auto digit : trailing) {
    magnitude = (magnitude * 10) + static_cast<std::uint64_t>(digit - '0');
  }

  assert(magnitude <=
         static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));

  auto adjusted_exponent{view.substr(marker + 1)};
  if (adjusted_exponent.front() == '+') {
    adjusted_exponent.remove_prefix(1);
  }

  std::int64_t adjusted{0};
  [[maybe_unused]] const auto conversion{std::from_chars(
      adjusted_exponent.data(),
      adjusted_exponent.data() + adjusted_exponent.size(), adjusted)};
  assert(conversion.ec == std::errc{});

  exponent = adjusted - static_cast<std::int64_t>(digits - 1);
  const auto coefficient{static_cast<std::int64_t>(magnitude)};
  return negative ? -coefficient : coefficient;
}

} // namespace

namespace sourcemeta::jsonbinpack {

auto Encoder::DOUBLE_VARINT_TUPLE(
    const sourcemeta::core::JSON &document,
    [[maybe_unused]] const struct DOUBLE_VARINT_TUPLE &options) -> void {
  assert(document.is_real());
  const auto value{document.to_real()};
  std::uint64_t point_position;
  const std::int64_t integral{
      sourcemeta::core::real_digits<std::int64_t>(value, point_position)};
  this->put_varint_zigzag(integral);
  this->put_varint(point_position);
}

auto Encoder::NUMBER_VARINT_TUPLE(
    const sourcemeta::core::JSON &document,
    [[maybe_unused]] const struct NUMBER_VARINT_TUPLE &options) -> void {
  assert(document.is_real() || document.is_decimal());
  const auto decimal{document.is_decimal()};
  std::int64_t coefficient{0};
  std::int64_t exponent{0};

  if (decimal) {
    coefficient = decimal_coefficient(document.to_decimal(), exponent);
  } else {
    std::uint64_t point_position;
    coefficient = sourcemeta::core::real_digits<std::int64_t>(
        document.to_real(), point_position);
    exponent = -static_cast<std::int64_t>(point_position);
  }

  this->put_varint_zigzag(coefficient);
  this->put_varint((sourcemeta::core::zigzag_encode(exponent) << 1) |
                   static_cast<std::uint64_t>(decimal));
}

} // namespace sourcemeta::jsonbinpack
