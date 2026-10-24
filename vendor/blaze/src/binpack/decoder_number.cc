#include <sourcemeta/blaze/binpack_decoder.h>

#include <sourcemeta/core/numeric.h>

#include <cmath>   // std::isfinite
#include <cstdint> // std::int64_t, std::uint64_t

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("no-reciprocal-math")
#endif

namespace sourcemeta::blaze {

auto BinPackDecoder::DOUBLE_VARINT_TUPLE(
    [[maybe_unused]] const struct DOUBLE_VARINT_TUPLE &options)
    -> sourcemeta::core::JSON {
#ifdef __clang__
#pragma clang fp reciprocal(off)
#endif
  const std::int64_t digits{this->get_varint_zigzag()};
  const std::uint64_t point{this->get_varint()};
  double divisor{1.0};
  for (std::uint64_t i = 0; i < point; ++i) {
    divisor *= 10.0;
  }
  return sourcemeta::core::JSON{static_cast<double>(digits) / divisor};
}

auto BinPackDecoder::NUMBER_VARINT_TUPLE(
    [[maybe_unused]] const struct NUMBER_VARINT_TUPLE &options)
    -> sourcemeta::core::JSON {
#ifdef __clang__
#pragma clang fp reciprocal(off)
#endif
  const std::int64_t coefficient{this->get_varint_zigzag()};
  const std::uint64_t tag{this->get_varint()};
  const std::int64_t exponent{
      sourcemeta::core::zigzag_decode<std::uint64_t>(tag >> 1)};

  if ((tag & 1) != 0) {
    return sourcemeta::core::JSON{
        sourcemeta::core::Decimal{coefficient}.scale_by(
            sourcemeta::core::Decimal{exponent})};
  }

  // A scale that has grown past the range of the type can only keep growing,
  // so the loop stops there rather than counting out an exponent that a
  // corrupt stream may have made arbitrarily large
  const std::uint64_t shifts{exponent < 0
                                 ? static_cast<std::uint64_t>(-exponent)
                                 : static_cast<std::uint64_t>(exponent)};
  double scale{1.0};
  for (std::uint64_t index = 0; index < shifts && std::isfinite(scale);
       ++index) {
    scale *= 10.0;
  }

  return sourcemeta::core::JSON{exponent < 0
                                    ? static_cast<double>(coefficient) / scale
                                    : static_cast<double>(coefficient) * scale};
}

} // namespace sourcemeta::blaze
