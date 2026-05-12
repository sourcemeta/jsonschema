#include <sourcemeta/jsonbinpack/runtime_input_stream.h>

#include <sourcemeta/core/numeric.h>

#include <cassert> // assert
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t, std::uint64_t, std::int64_t

namespace sourcemeta::jsonbinpack {

InputStream::InputStream(Stream &input)
    : sourcemeta::core::BinaryReader{input} {}

auto InputStream::rewind(const std::uint64_t relative_offset,
                         const std::uint64_t position) -> std::uint64_t {
  assert(position >= relative_offset);
  const std::uint64_t offset{position - relative_offset};
  assert(offset < position);
  const std::uint64_t current{this->position()};
  this->seek(offset);
  return current;
}

auto InputStream::get_varint() -> std::uint64_t {
  constexpr std::uint8_t LEAST_SIGNIFICANT_BITS{0b01111111};
  constexpr std::uint8_t MOST_SIGNIFICANT_BIT{0b10000000};
  constexpr std::uint8_t SHIFT{7};
  std::uint64_t result{0};
  std::size_t cursor{0};
  while (true) {
    const std::uint8_t byte{this->get_byte()};
    const std::uint64_t value{
        static_cast<std::uint64_t>(byte & LEAST_SIGNIFICANT_BITS)};
#ifndef NDEBUG
    const std::uint64_t current = result;
#endif
    result += static_cast<std::uint64_t>(value << SHIFT * cursor);
    // Try to catch potential overflows from the above addition
    assert(result >= current);
    cursor += 1;
    if ((byte & MOST_SIGNIFICANT_BIT) == 0) {
      break;
    }
  }

  return result;
}

auto InputStream::get_varint_zigzag() -> std::int64_t {
  return sourcemeta::core::zigzag_decode(this->get_varint());
}

auto InputStream::get_string_utf8(const std::uint64_t length)
    -> sourcemeta::core::JSON::String {
  sourcemeta::core::JSON::String result;
  result.reserve(length);
  std::uint64_t counter = 0;
  while (counter < length) {
    result += static_cast<sourcemeta::core::JSON::Char>(this->get_byte());
    counter += 1;
  }

  assert(counter == length);
  assert(result.size() == length);
  return result;
}

} // namespace sourcemeta::jsonbinpack
