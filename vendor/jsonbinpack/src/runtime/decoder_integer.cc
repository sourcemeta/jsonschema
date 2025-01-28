#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint32_t, std::int64_t, std::uint64_t

namespace sourcemeta::jsonbinpack {

auto Decoder::BOUNDED_MULTIPLE_8BITS_ENUM_FIXED(
    const struct BOUNDED_MULTIPLE_8BITS_ENUM_FIXED &options)
    -> sourcemeta::core::JSON {
  assert(options.multiplier > 0);
  const std::uint8_t byte{this->get_byte()};
  const std::int64_t closest_minimum{
      divide_ceil(options.minimum, options.multiplier)};
  if (closest_minimum >= 0) {
    const std::uint64_t closest_minimum_multiple{
        static_cast<std::uint32_t>(closest_minimum) * options.multiplier};
    // We trust the encoder that the data we are seeing
    // corresponds to a valid 64-bit signed integer.
    return sourcemeta::core::JSON{static_cast<std::int64_t>(
        (byte * options.multiplier) + closest_minimum_multiple)};
  } else {
    const std::uint64_t closest_minimum_multiple{abs(closest_minimum) *
                                                 options.multiplier};
    // We trust the encoder that the data we are seeing
    // corresponds to a valid 64-bit signed integer.
    return sourcemeta::core::JSON{static_cast<std::int64_t>(
        (byte * options.multiplier) - closest_minimum_multiple)};
  }
}

auto Decoder::FLOOR_MULTIPLE_ENUM_VARINT(
    const struct FLOOR_MULTIPLE_ENUM_VARINT &options)
    -> sourcemeta::core::JSON {
  assert(options.multiplier > 0);
  const std::int64_t closest_minimum{
      divide_ceil(options.minimum, options.multiplier)};
  if (closest_minimum >= 0) {
    const std::uint64_t closest_minimum_multiple{
        static_cast<std::uint32_t>(closest_minimum) * options.multiplier};
    // We trust the encoder that the data we are seeing
    // corresponds to a valid 64-bit signed integer.
    return sourcemeta::core::JSON{static_cast<std::int64_t>(
        (this->get_varint() * options.multiplier) + closest_minimum_multiple)};
  } else {
    const std::uint64_t closest_minimum_multiple{abs(closest_minimum) *
                                                 options.multiplier};
    // We trust the encoder that the data we are seeing
    // corresponds to a valid 64-bit signed integer.
    return sourcemeta::core::JSON{static_cast<std::int64_t>(
        (this->get_varint() * options.multiplier) - closest_minimum_multiple)};
  }
}

auto Decoder::ROOF_MULTIPLE_MIRROR_ENUM_VARINT(
    const struct ROOF_MULTIPLE_MIRROR_ENUM_VARINT &options)
    -> sourcemeta::core::JSON {
  assert(options.multiplier > 0);
  const std::int64_t closest_maximum{
      divide_floor(options.maximum, options.multiplier)};
  if (closest_maximum >= 0) {
    const std::uint64_t closest_maximum_multiple{
        static_cast<std::uint32_t>(closest_maximum) * options.multiplier};
    // We trust the encoder that the data we are seeing
    // corresponds to a valid 64-bit signed integer.
    return sourcemeta::core::JSON{static_cast<std::int64_t>(
        -(static_cast<std::int64_t>(this->get_varint() * options.multiplier)) +
        static_cast<std::int64_t>(closest_maximum_multiple))};
  } else {
    const std::uint64_t closest_maximum_multiple{abs(closest_maximum) *
                                                 options.multiplier};
    // We trust the encoder that the data we are seeing
    // corresponds to a valid 64-bit signed integer.
    return sourcemeta::core::JSON{static_cast<std::int64_t>(
        -(static_cast<std::int64_t>(this->get_varint() * options.multiplier)) -
        static_cast<std::int64_t>(closest_maximum_multiple))};
  }
}

auto Decoder::ARBITRARY_MULTIPLE_ZIGZAG_VARINT(
    const struct ARBITRARY_MULTIPLE_ZIGZAG_VARINT &options)
    -> sourcemeta::core::JSON {
  assert(options.multiplier > 0);
  // We trust the encoder that the data we are seeing
  // corresponds to a valid 64-bit signed integer.
  return sourcemeta::core::JSON{
      static_cast<std::int64_t>(this->get_varint_zigzag() *
                                static_cast<std::int64_t>(options.multiplier))};
}

} // namespace sourcemeta::jsonbinpack
