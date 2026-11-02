#include <sourcemeta/core/gzip.h>

#include "deflate.h"
#include "inflate.h"

#include <sourcemeta/core/crypto.h>

#include <algorithm>   // std::min
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <memory>      // std::make_unique, std::make_unique_for_overwrite
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

// RFC 1951 cannot expand data by more than this factor, which bounds how far
// the size recorded in a trailer can be trusted as a first capacity guess
constexpr std::size_t MAXIMUM_EXPANSION{1032};

constexpr std::size_t GZIP_HEADER_SIZE{10};
constexpr std::size_t GZIP_TRAILER_SIZE{8};

auto initial_capacity(const std::uint8_t *input, const std::size_t size,
                      const std::size_t output_hint,
                      const std::size_t maximum_size) -> std::size_t {
  if (output_hint > 0) {
    return std::min(output_hint, maximum_size);
  }

  // The last four bytes of a single member input record its size modulo 2^32
  if (size >= 18) {
    const auto recorded{static_cast<std::size_t>(input[size - 4]) |
                        (static_cast<std::size_t>(input[size - 3]) << 8) |
                        (static_cast<std::size_t>(input[size - 2]) << 16) |
                        (static_cast<std::size_t>(input[size - 1]) << 24)};
    const auto bound{size > maximum_size / MAXIMUM_EXPANSION
                         ? maximum_size
                         : size * MAXIMUM_EXPANSION};
    return std::min({recorded, bound, maximum_size});
  }

  return std::min(size, maximum_size);
}

auto store_little_endian(std::uint8_t *destination, const std::uint32_t value)
    -> void {
  destination[0] = static_cast<std::uint8_t>(value & 0xff);
  destination[1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
  destination[2] = static_cast<std::uint8_t>((value >> 16) & 0xff);
  destination[3] = static_cast<std::uint8_t>((value >> 24) & 0xff);
}

} // namespace

auto gzip(const std::uint8_t *input, const std::size_t size, const int level)
    -> std::string {
  if (level < 0 || level > 12) {
    throw GZIPError{"Invalid compression level"};
  }

  const auto encoder{std::make_unique<DeflateEncoder>(level, size)};
  std::string output;
  output.resize_and_overwrite(
      GZIP_HEADER_SIZE + DeflateEncoder::bound(size) + GZIP_TRAILER_SIZE,
      [&](char *const buffer, const std::size_t) -> std::size_t {
        auto *const data{reinterpret_cast<std::uint8_t *>(buffer)};
        // RFC 1952 section 2.3.1: no optional fields, no modification time,
        // and an unknown operating system
        data[0] = 0x1f;
        data[1] = 0x8b;
        data[2] = 0x08;
        data[3] = 0x00;
        store_little_endian(data + 4, 0);
        data[8] = level <= 1 ? 0x04 : (level >= 9 ? 0x02 : 0x00);
        data[9] = 0xff;
        const auto written{
            encoder->compress(input, size, data + GZIP_HEADER_SIZE)};
        auto *const trailer{data + GZIP_HEADER_SIZE + written};
        store_little_endian(trailer,
                            crc32(std::string_view{
                                reinterpret_cast<const char *>(input), size}));
        store_little_endian(trailer + 4, static_cast<std::uint32_t>(size));
        return GZIP_HEADER_SIZE + written + GZIP_TRAILER_SIZE;
      });

  return output;
}

auto gunzip(const std::uint8_t *input, const std::size_t size,
            const std::size_t output_hint, const std::size_t maximum_size)
    -> std::string {
  const auto decoder{std::make_unique_for_overwrite<InflateDecoder>()};
  InflateBuffers buffers{.input_next = input,
                         .input_end = input + size,
                         .input_final = true,
                         .output_begin = nullptr,
                         .output_next = nullptr,
                         .output_end = nullptr};
  std::string output;
  std::size_t capacity{
      initial_capacity(input, size, output_hint, maximum_size)};
  std::size_t position{0};
  while (true) {
    // Growing keeps the bytes decoded so far, and leaving the rest
    // uninitialised avoids zero-filling memory that is about to be written
    output.resize_and_overwrite(
        capacity,
        [](char *, const std::size_t count) -> std::size_t { return count; });
    auto *const begin{reinterpret_cast<std::uint8_t *>(output.data())};
    buffers.output_begin = begin;
    buffers.output_next = begin + position;
    buffers.output_end = begin + capacity;
    const auto status{decoder->decode(buffers)};
    position = static_cast<std::size_t>(buffers.output_next - begin);
    if (status == InflateStatus::Done) {
      break;
    }

    // The whole input is available upfront, so the decoder only ever stops
    // early because the output is full
    if (capacity >= maximum_size) {
      throw GZIPError{"Decompressed output exceeds the maximum allowed size"};
    }

    if (capacity == 0) {
      capacity = std::min(std::size_t{4096}, maximum_size);
    } else {
      capacity = capacity > maximum_size / 2 ? maximum_size : capacity * 2;
    }
  }

  output.resize(position);
  return output;
}

} // namespace sourcemeta::core
