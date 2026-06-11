#ifndef SOURCEMETA_CORE_GZIP_BIT_READER_H_
#define SOURCEMETA_CORE_GZIP_BIT_READER_H_

#include <sourcemeta/core/gzip_error.h>

#include <array>   // std::array
#include <cassert> // assert
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t
#include <istream> // std::istream

namespace sourcemeta::core {

class BitReader {
public:
  BitReader(std::istream &source) : source_{&source} {}

  auto read_bits(const unsigned int count) -> std::uint32_t {
    const auto value{this->peek_bits(count)};
    this->consume_bits(count);
    return value;
  }

  auto peek_bits(const unsigned int count) -> std::uint32_t {
    // Callers in this module always pass count in [0, 32]; larger shifts
    // would be undefined behaviour against the 64-bit accumulator. The
    // assert documents the contract without paying a release-build cost
    assert(count <= 32);
    if (this->bits_available_ < count) {
      this->refill_for(count);
    }
    const auto mask{(static_cast<std::uint64_t>(1) << count) - 1};
    return static_cast<std::uint32_t>(this->accumulator_ & mask);
  }

  auto consume_bits(const unsigned int count) -> void {
    // Consuming more bits than are buffered would underflow the unsigned
    // counter. Every call site is preceded by a peek or refill that
    // guarantees enough bits, so the assert documents the contract
    assert(count <= this->bits_available_);
    this->accumulator_ >>= count;
    this->bits_available_ -= count;
  }

  auto align_to_byte() -> void {
    const auto trailing_bits{this->bits_available_ % 8};
    this->accumulator_ >>= trailing_bits;
    this->bits_available_ -= trailing_bits;
  }

  auto read_byte() -> std::uint8_t {
    // Reading a byte while 1 to 7 bits are buffered would return a byte from
    // ahead of them. Every call site is byte-aligned, so the assert documents
    // the invariant without paying a release-build cost
    assert(this->bits_available_ % 8 == 0);
    if (this->bits_available_ >= 8) {
      const auto value{static_cast<std::uint8_t>(this->accumulator_ & 0xff)};
      this->accumulator_ >>= 8;
      this->bits_available_ -= 8;
      return value;
    }
    return this->pull_source_byte();
  }

  auto try_read_byte(std::uint8_t &byte) -> bool {
    assert(this->bits_available_ % 8 == 0);
    if (this->bits_available_ >= 8) {
      byte = static_cast<std::uint8_t>(this->accumulator_ & 0xff);
      this->accumulator_ >>= 8;
      this->bits_available_ -= 8;
      return true;
    }
    if (this->buffer_position_ >= this->buffer_size_ &&
        !this->try_refill_buffer()) {
      return false;
    }
    byte = this->buffer_[this->buffer_position_++];
    return true;
  }

  auto read_bytes(std::uint8_t *destination, const std::size_t count) -> void {
    for (std::size_t index = 0; index < count; ++index) {
      destination[index] = this->read_byte();
    }
  }

private:
  static constexpr std::size_t SOURCE_BUFFER_SIZE{4096};

  auto pull_source_byte() -> std::uint8_t {
    if (this->buffer_position_ >= this->buffer_size_) {
      this->refill_buffer();
    }
    return this->buffer_[this->buffer_position_++];
  }

  auto refill_for(const unsigned int count) -> void {
    // Fast path: if 4 bytes available in the input buffer and the
    // accumulator has room for 32 more bits, load 4 bytes at once.
    // RFC 1951 packs bits LSB-first within each byte, so the first byte
    // contributes the low 8 bits of the loaded word regardless of host
    // endianness. Construct the 32-bit value explicitly to keep this
    // portable on big-endian hosts
    if (this->bits_available_ <= 32 &&
        this->buffer_position_ + 4 <= this->buffer_size_) {
      const std::uint32_t four_bytes{
          static_cast<std::uint32_t>(this->buffer_[this->buffer_position_]) |
          (static_cast<std::uint32_t>(this->buffer_[this->buffer_position_ + 1])
           << 8) |
          (static_cast<std::uint32_t>(this->buffer_[this->buffer_position_ + 2])
           << 16) |
          (static_cast<std::uint32_t>(this->buffer_[this->buffer_position_ + 3])
           << 24)};
      this->accumulator_ |= static_cast<std::uint64_t>(four_bytes)
                            << this->bits_available_;
      this->bits_available_ += 32;
      this->buffer_position_ += 4;
    }
    while (this->bits_available_ < count) {
      const auto byte{this->pull_source_byte()};
      this->accumulator_ |= static_cast<std::uint64_t>(byte)
                            << this->bits_available_;
      this->bits_available_ += 8;
    }
  }

  auto refill_buffer() -> void {
    if (!this->try_refill_buffer()) {
      throw GZIPError{"Unexpected end of source stream"};
    }
  }

  auto try_refill_buffer() -> bool {
    this->source_->read(reinterpret_cast<char *>(this->buffer_.data()),
                        SOURCE_BUFFER_SIZE);
    const auto bytes_read{static_cast<std::size_t>(this->source_->gcount())};
    if (bytes_read == 0) {
      return false;
    }
    this->buffer_size_ = bytes_read;
    this->buffer_position_ = 0;
    return true;
  }

  std::istream *source_;
  std::uint64_t accumulator_{0};
  unsigned int bits_available_{0};
  std::array<std::uint8_t, SOURCE_BUFFER_SIZE> buffer_{};
  std::size_t buffer_position_{0};
  std::size_t buffer_size_{0};
};

} // namespace sourcemeta::core

#endif
