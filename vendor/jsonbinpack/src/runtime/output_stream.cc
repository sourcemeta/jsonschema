#include <sourcemeta/jsonbinpack/runtime_output_stream.h>

#include <sourcemeta/core/numeric.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint64_t, std::int64_t

namespace sourcemeta::jsonbinpack {

OutputStream::OutputStream(Stream &output)
    : sourcemeta::core::BinaryWriter{output} {}

auto OutputStream::put_varint(const std::uint64_t value) -> void {
  constexpr std::uint8_t LEAST_SIGNIFICANT_BITS{0b01111111};
  constexpr std::uint8_t MOST_SIGNIFICANT_BIT{0b10000000};
  constexpr std::uint8_t SHIFT{7};
  std::uint64_t accumulator = value;

  while (accumulator > LEAST_SIGNIFICANT_BITS) {
    this->put_byte(static_cast<std::uint8_t>(
        (accumulator & LEAST_SIGNIFICANT_BITS) | MOST_SIGNIFICANT_BIT));
    accumulator >>= SHIFT;
  }

  this->put_byte(static_cast<std::uint8_t>(accumulator));
}

auto OutputStream::put_varint_zigzag(const std::int64_t value) -> void {
  this->put_varint(sourcemeta::core::zigzag_encode(value));
}

auto OutputStream::put_string_utf8(const sourcemeta::core::JSON::String &string,
                                   const std::uint64_t length) -> void {
  assert(string.size() == length);
  // Do a manual for-loop based on the provided length instead of a range
  // loop based on the string value to avoid accidental overflows
  for (std::uint64_t index = 0; index < length; index++) {
    this->put_byte(static_cast<std::uint8_t>(string[index]));
  }
}

} // namespace sourcemeta::jsonbinpack
