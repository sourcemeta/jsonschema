#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::int64_t, std::uint64_t

namespace sourcemeta::jsonbinpack {

auto Encoder::BOUNDED_MULTIPLE_8BITS_ENUM_FIXED(
    const sourcemeta::core::JSON &document,
    const struct BOUNDED_MULTIPLE_8BITS_ENUM_FIXED &options) -> void {
  assert(document.is_integer());
  const std::int64_t value{document.to_integer()};
  assert(is_within(value, options.minimum, options.maximum));
  assert(options.multiplier > 0);
  assert(abs(value) % options.multiplier == 0);
  const std::int64_t enum_minimum{
      divide_ceil(options.minimum, options.multiplier)};
#ifndef NDEBUG
  const std::int64_t enum_maximum{
      divide_floor(options.maximum, options.multiplier)};
#endif
  assert(is_byte(enum_maximum - enum_minimum));
  this->put_byte(static_cast<std::uint8_t>(
      (value / static_cast<std::int64_t>(options.multiplier)) - enum_minimum));
}

auto Encoder::FLOOR_MULTIPLE_ENUM_VARINT(
    const sourcemeta::core::JSON &document,
    const struct FLOOR_MULTIPLE_ENUM_VARINT &options) -> void {
  assert(document.is_integer());
  const std::int64_t value{document.to_integer()};
  assert(options.minimum <= value);
  assert(options.multiplier > 0);
  assert(abs(value) % options.multiplier == 0);
  if (options.multiplier == 1) {
    return this->put_varint(
        static_cast<std::uint64_t>(value - options.minimum));
  }

  return this->put_varint(
      (static_cast<std::uint64_t>(value) / options.multiplier) -
      static_cast<std::uint64_t>(divide_ceil(
          options.minimum, static_cast<std::uint64_t>(options.multiplier))));
}

auto Encoder::ROOF_MULTIPLE_MIRROR_ENUM_VARINT(
    const sourcemeta::core::JSON &document,
    const struct ROOF_MULTIPLE_MIRROR_ENUM_VARINT &options) -> void {
  assert(document.is_integer());
  const std::int64_t value{document.to_integer()};
  assert(value <= options.maximum);
  assert(options.multiplier > 0);
  assert(abs(value) % options.multiplier == 0);
  if (options.multiplier == 1) {
    return this->put_varint(
        static_cast<std::uint64_t>(options.maximum - value));
  }

  return this->put_varint(
      static_cast<std::uint64_t>(
          divide_floor(options.maximum, options.multiplier)) -
      (static_cast<std::uint64_t>(value) / options.multiplier));
}

auto Encoder::ARBITRARY_MULTIPLE_ZIGZAG_VARINT(
    const sourcemeta::core::JSON &document,
    const struct ARBITRARY_MULTIPLE_ZIGZAG_VARINT &options) -> void {
  assert(document.is_integer());
  const std::int64_t value{document.to_integer()};
  assert(options.multiplier > 0);
  assert(abs(value) % options.multiplier == 0);
  this->put_varint_zigzag(value /
                          static_cast<std::int64_t>(options.multiplier));
}

} // namespace sourcemeta::jsonbinpack
