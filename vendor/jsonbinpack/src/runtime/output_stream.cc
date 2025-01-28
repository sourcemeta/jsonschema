#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_output_stream.h>

#include "varint.h"

#include <cassert> // assert
#include <ios>     // std::ios_base

namespace sourcemeta::jsonbinpack {

OutputStream::OutputStream(Stream &output) : stream{output} {
  this->stream.exceptions(std::ios_base::badbit | std::ios_base::failbit |
                          std::ios_base::eofbit);
}

auto OutputStream::position() const noexcept -> std::uint64_t {
  return static_cast<std::uint64_t>(this->stream.tellp());
}

auto OutputStream::put_byte(const std::uint8_t byte) -> void {
  this->stream.put(static_cast<sourcemeta::core::JSON::Char>(byte));
}

auto OutputStream::put_bytes(const std::uint16_t bytes) -> void {
  this->stream.write(
      reinterpret_cast<const sourcemeta::core::JSON::Char *>(&bytes),
      sizeof bytes);
}

auto OutputStream::put_varint(const std::uint64_t value) -> void {
  varint_encode(this->stream, value);
}

auto OutputStream::put_varint_zigzag(const std::int64_t value) -> void {
  varint_encode(this->stream, zigzag_encode(value));
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
