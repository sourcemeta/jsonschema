#ifndef SOURCEMETA_CORE_GZIP_HUFFMAN_H_
#define SOURCEMETA_CORE_GZIP_HUFFMAN_H_

#include "bit_reader.h"

#include <sourcemeta/core/gzip_error.h>

#include <algorithm> // std::ranges::fill
#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t, std::uint16_t

namespace sourcemeta::core {

// Maximum Huffman code length per RFC 1951 section 3.2.7
inline constexpr unsigned int MAX_HUFFMAN_BITS{15};

// Largest alphabet is the literal/length alphabet (288 symbols) per RFC 1951
// section 3.2.5
inline constexpr std::size_t MAX_HUFFMAN_SYMBOLS{288};

class HuffmanDecoder {
public:
  // Primary lookup table covering all codes of length <= LUT_BITS. Misses
  // fall back to the canonical-puff traversal for codes of length 10..15
  static constexpr unsigned int LUT_BITS{9};
  static constexpr std::size_t LUT_SIZE{1u << LUT_BITS};

  HuffmanDecoder() = default;

  // The fixed distance tree of RFC 1951 section 3.2.6 is intentionally
  // incomplete (30 codes of length five over a 32-slot space), so the
  // completeness check is suppressed for it and enforced everywhere else
  auto build(const std::uint8_t *lengths, const std::size_t length_count,
             const bool allow_incomplete = false) -> void {
    std::ranges::fill(this->count_, std::uint16_t{0});
    std::ranges::fill(this->lut_, std::uint16_t{0});

    for (std::size_t symbol = 0; symbol < length_count; ++symbol) {
      if (lengths[symbol] > MAX_HUFFMAN_BITS) {
        throw GZIPError{"Huffman code length out of range"};
      }
      this->count_[lengths[symbol]]++;
    }

    if (this->count_[0] == length_count) {
      return;
    }

    // Verify the alphabet is complete or single-symbol per RFC 1951
    int left{1};
    for (unsigned int bits = 1; bits <= MAX_HUFFMAN_BITS; ++bits) {
      left <<= 1;
      left -= this->count_[bits];
      if (left < 0) {
        throw GZIPError{"Over-subscribed Huffman code"};
      }
    }

    // Reject incomplete codes, matching zlib and puff. RFC 1951 sanctions
    // incompleteness only for the single-code case (a tree built from one
    // used code of length one), where every length is either zero or one
    if (left > 0 && !allow_incomplete &&
        length_count != static_cast<std::size_t>(this->count_[0]) +
                            static_cast<std::size_t>(this->count_[1])) {
      throw GZIPError{"Incomplete Huffman code"};
    }

    std::array<std::uint16_t, MAX_HUFFMAN_BITS + 1> offsets{};
    offsets[1] = 0;
    for (unsigned int bits = 1; bits < MAX_HUFFMAN_BITS; ++bits) {
      offsets[bits + 1] =
          static_cast<std::uint16_t>(offsets[bits] + this->count_[bits]);
    }

    for (std::size_t symbol = 0; symbol < length_count; ++symbol) {
      if (lengths[symbol] != 0) {
        this->symbols_[offsets[lengths[symbol]]++] =
            static_cast<std::uint16_t>(symbol);
      }
    }

    // Build the fast-path LUT. Each entry packs (symbol << 4) | length;
    // a value of 0 means "no short code, fall back". Codes longer than
    // LUT_BITS leave their LUT entries at 0
    std::array<std::uint32_t, MAX_HUFFMAN_BITS + 1> next_code{};
    next_code[1] = 0;
    for (unsigned int bits = 2; bits <= MAX_HUFFMAN_BITS; ++bits) {
      next_code[bits] = (next_code[bits - 1] + this->count_[bits - 1]) << 1u;
    }

    std::size_t symbol_index{0};
    for (unsigned int code_length = 1; code_length <= MAX_HUFFMAN_BITS;
         ++code_length) {
      for (unsigned int k = 0; k < this->count_[code_length]; ++k) {
        const auto symbol{this->symbols_[symbol_index]};
        if (code_length <= LUT_BITS) {
          const auto code{next_code[code_length]};
          const auto lsb_key{reverse_bits(code, code_length)};
          const auto entry{static_cast<std::uint16_t>(
              static_cast<std::uint16_t>(code_length) |
              static_cast<std::uint16_t>(symbol << 4u))};
          const std::uint32_t stride{1u << code_length};
          for (std::uint32_t slot = lsb_key; slot < LUT_SIZE; slot += stride) {
            this->lut_[slot] = entry;
          }
        }
        ++next_code[code_length];
        ++symbol_index;
      }
    }
  }

  auto decode(BitReader &reader) const -> std::uint16_t {
    const auto key{reader.peek_bits(LUT_BITS)};
    const auto entry{this->lut_[key]};
    if (entry != 0) {
      const unsigned int length{entry & 0xfu};
      reader.consume_bits(length);
      return static_cast<std::uint16_t>(entry >> 4u);
    }
    return this->decode_long(reader);
  }

private:
  auto decode_long(BitReader &reader) const -> std::uint16_t {
    std::uint32_t bits{reader.peek_bits(MAX_HUFFMAN_BITS)};
    int code{0};
    int first{0};
    int index{0};
    for (unsigned int length = 1; length <= MAX_HUFFMAN_BITS; ++length) {
      code |= static_cast<int>(bits & 1u);
      bits >>= 1u;
      const auto entries{static_cast<int>(this->count_[length])};
      if (code - entries < first) {
        const auto position{static_cast<std::size_t>(index) +
                            static_cast<std::size_t>(code - first)};
        reader.consume_bits(length);
        return this->symbols_[position];
      }
      index += entries;
      first = (first + entries) << 1;
      code <<= 1;
    }
    throw GZIPError{"Invalid Huffman code"};
  }

  static auto reverse_bits(std::uint32_t value, const unsigned int length)
      -> std::uint32_t {
    std::uint32_t result{0};
    for (unsigned int index = 0; index < length; ++index) {
      if ((value & (1u << (length - 1 - index))) != 0u) {
        result |= (1u << index);
      }
    }
    return result;
  }

  std::array<std::uint16_t, MAX_HUFFMAN_BITS + 1> count_{};
  std::array<std::uint16_t, MAX_HUFFMAN_SYMBOLS> symbols_{};
  std::array<std::uint16_t, LUT_SIZE> lut_{};
};

} // namespace sourcemeta::core

#endif
