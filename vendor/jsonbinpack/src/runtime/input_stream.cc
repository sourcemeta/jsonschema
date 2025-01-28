#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_input_stream.h>

#include "varint.h"

#include <cassert> // assert
#include <ios>     // std::ios_base

namespace sourcemeta::jsonbinpack {

InputStream::InputStream(Stream &input) : stream{input} {
  this->stream.exceptions(std::ios_base::badbit | std::ios_base::failbit |
                          std::ios_base::eofbit);
}

auto InputStream::position() const noexcept -> std::uint64_t {
  return static_cast<std::uint64_t>(this->stream.tellg());
}

auto InputStream::seek(const std::uint64_t offset) -> void {
  this->stream.seekg(static_cast<std::streamoff>(offset));
}

auto InputStream::rewind(const std::uint64_t relative_offset,
                         const std::uint64_t position) -> std::uint64_t {
  assert(position >= relative_offset);
  const std::uint64_t offset{position - relative_offset};
  assert(offset < position);
  const std::uint64_t current{this->position()};
  this->seek(offset);
  return current;
}

auto InputStream::get_byte() -> std::uint8_t {
  return static_cast<std::uint8_t>(this->stream.get());
}

auto InputStream::get_word() -> std::uint16_t {
  std::uint16_t word;
  this->stream.read(reinterpret_cast<char *>(&word), sizeof word);
  return word;
}

auto InputStream::get_varint() -> std::uint64_t {
  return varint_decode(this->stream);
}

auto InputStream::get_varint_zigzag() -> std::int64_t {
  const std::uint64_t value = varint_decode(this->stream);
  return zigzag_decode(value);
}

auto InputStream::has_more_data() const noexcept -> bool {
  // A way to check if the stream is empty without using `.peek()`,
  // which throws given we set exceptions on the EOF bit.
  // However, `in_avail()` works on characters and will return zero
  // if all that's remaining is 0x00 (null), so we need to handle
  // that case separately.
  return this->stream.rdbuf()->in_avail() > 0 ||
         this->stream.rdbuf()->sgetc() == '\0';
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
