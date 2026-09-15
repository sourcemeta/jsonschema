#ifndef SOURCEMETA_CORE_GZIP_DEFLATE_H_
#define SOURCEMETA_CORE_GZIP_DEFLATE_H_

#include "inflate.h"

#include <algorithm> // std::sort, std::fill, std::min
#include <array>     // std::array
#include <bit>       // std::endian, std::byteswap, std::countr_zero
#include <cstddef>   // std::size_t
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <cstring> // std::memcpy
#include <vector>  // std::vector

namespace sourcemeta::core {

inline constexpr std::size_t DEFLATE_MAXIMUM_MATCH_LENGTH{258};
inline constexpr std::size_t DEFLATE_WINDOW_SIZE{32768};
inline constexpr std::size_t DEFLATE_MAXIMUM_STORED_LENGTH{65535};
inline constexpr std::uint32_t DEFLATE_END_OF_BLOCK{256};

// The index of the length symbol, counting from symbol 257, for every match
// length as per RFC 1951 section 3.2.5
inline constexpr std::array<std::uint8_t, 259> DEFLATE_LENGTH_SLOTS{
    {0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9,  10, 10, 11, 11,
     12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16,
     16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18,
     18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20,
     20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
     21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
     22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24,
     24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
     24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25,
     25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
     25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
     26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27,
     27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
     27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28}};

// The distance symbol for every distance up to 256 in the first half, and for
// every larger distance indexed by its value minus one divided by 128 in the
// second half, as every symbol past 256 spans whole multiples of 128
inline constexpr std::array<std::uint8_t, 512> DEFLATE_DISTANCE_SLOTS{
    {0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,
     8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
     11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
     12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
     12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
     13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14,
     14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
     14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
     14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
     14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
     15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
     15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
     15, 15, 15, 15, 15, 15, 15, 15, 15, 0,  0,  16, 17, 18, 18, 19, 19, 20, 20,
     20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23,
     23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
     25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26,
     26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
     26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27,
     27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
     27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
     28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
     28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
     28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29,
     29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
     29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
     29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29}};

inline auto deflate_distance_slot(const std::size_t distance) -> std::size_t {
  return distance <= 256 ? DEFLATE_DISTANCE_SLOTS[distance - 1]
                         : DEFLATE_DISTANCE_SLOTS[256 + ((distance - 1) >> 7)];
}

// Packs bits starting from the least significant bit of each byte as per
// RFC 1951 section 3.1.1. The destination must always have room for a whole
// word past the bytes written so far
class DeflateBitWriter {
public:
  DeflateBitWriter(std::uint8_t *output) : next_{output} {}

  // The buffer holds 64 bits, and a flush leaves at most seven of them, so
  // callers flush before the bits added since the last flush exceed 57
  auto add(const std::uint32_t value, const unsigned int count) -> void {
    this->buffer_ |= static_cast<std::uint64_t>(value) << this->count_;
    this->count_ += count;
  }

  auto flush() -> void {
    std::uint64_t word{this->buffer_};
    if (std::endian::native == std::endian::big) {
      word = std::byteswap(word);
    }

    std::memcpy(this->next_, &word, sizeof(word));
    const auto bytes{this->count_ >> 3};
    this->next_ += bytes;
    this->buffer_ >>= bytes * 8;
    this->count_ -= bytes * 8;
  }

  auto align() -> void {
    this->flush();
    if (this->count_ > 0) {
      this->next_ += 1;
    }

    this->buffer_ = 0;
    this->count_ = 0;
  }

  auto bytes(const std::uint8_t *data, const std::size_t size) -> void {
    if (size > 0) {
      std::memcpy(this->next_, data, size);
    }

    this->next_ += size;
  }

  [[nodiscard]] auto pending_bits() const -> unsigned int {
    return this->count_;
  }

  [[nodiscard]] auto position() const -> std::uint8_t * { return this->next_; }

private:
  std::uint8_t *next_;
  std::uint64_t buffer_{0};
  unsigned int count_{0};
};

// Scratch space for computing length-limited codes with the package-merge
// algorithm, where every list holds at most twice as many items as symbols
class DeflateCodeBuilder {
public:
  // Assigns every symbol with a nonzero frequency a codeword length of at most
  // the given limit, minimizing the total encoded size. The resulting code is
  // always complete, adding placeholder symbols when fewer than two are used
  auto lengths(const std::uint32_t *frequencies, const std::size_t symbol_count,
               const unsigned int limit, std::uint8_t *lengths) -> void {
    std::fill(lengths, lengths + symbol_count, std::uint8_t{0});
    std::size_t used{0};
    for (std::size_t symbol = 0; symbol < symbol_count; ++symbol) {
      if (frequencies[symbol] != 0) {
        this->symbols_[used] = static_cast<std::uint16_t>(symbol);
        used += 1;
      }
    }

    if (used < 2) {
      const std::size_t first{used == 1
                                  ? static_cast<std::size_t>(this->symbols_[0])
                                  : std::size_t{0}};
      lengths[first] = 1;
      lengths[first == 0 ? 1 : 0] = 1;
      return;
    }

    std::sort(
        this->symbols_.begin(), this->symbols_.begin() + used,
        [frequencies](const std::uint16_t left, const std::uint16_t right) {
          return frequencies[left] != frequencies[right]
                     ? frequencies[left] < frequencies[right]
                     : left < right;
        });

    // A plain Huffman code is optimal whenever no codeword exceeds the limit,
    // and building it takes linear time over the sorted frequencies
    if (this->unlimited_lengths(frequencies, used, limit, lengths)) {
      return;
    }

    std::fill(lengths, lengths + symbol_count, std::uint8_t{0});

    // The deepest list holds only the symbols. Every shallower list merges the
    // symbols with the pairs of consecutive items of the list below it
    const std::size_t deepest{limit - 1};
    for (std::size_t index = 0; index < used; ++index) {
      this->weights_[deepest][index] = frequencies[this->symbols_[index]];
      this->leaves_[deepest][index] = 1;
    }

    this->sizes_[deepest] = used;
    for (std::size_t level = deepest; level > 0; --level) {
      const std::size_t below{level};
      const std::size_t current{level - 1};
      const std::size_t packages{this->sizes_[below] / 2};
      std::size_t leaf{0};
      std::size_t package{0};
      std::size_t size{0};
      while (leaf < used || package < packages) {
        const std::uint64_t package_weight{
            package < packages ? this->weights_[below][package * 2] +
                                     this->weights_[below][(package * 2) + 1]
                               : 0};
        if (package >= packages ||
            (leaf < used &&
             frequencies[this->symbols_[leaf]] <= package_weight)) {
          this->weights_[current][size] = frequencies[this->symbols_[leaf]];
          this->leaves_[current][size] = 1;
          leaf += 1;
        } else {
          this->weights_[current][size] = package_weight;
          this->leaves_[current][size] = 0;
          package += 1;
        }

        size += 1;
      }

      this->sizes_[current] = size;
    }

    // Selecting the cheapest items of the shallowest list determines how many
    // items of every deeper list take part, and a symbol gains one bit of
    // length for every list where it is selected
    std::size_t selected{(used * 2) - 2};
    for (std::size_t level = 0; level < limit; ++level) {
      std::size_t leaves{0};
      for (std::size_t index = 0; index < selected; ++index) {
        leaves += this->leaves_[level][index];
      }

      for (std::size_t index = 0; index < leaves; ++index) {
        lengths[this->symbols_[index]] += 1;
      }

      selected = (selected - leaves) * 2;
    }
  }

  // Builds a Huffman code by repeatedly joining the two lightest nodes, taking
  // them from either the sorted symbols or the internal nodes created so far,
  // which are created in order of weight. Returns whether every codeword fits
  // within the limit
  auto unlimited_lengths(const std::uint32_t *frequencies,
                         const std::size_t used, const unsigned int limit,
                         std::uint8_t *lengths) -> bool {
    std::size_t leaf{0};
    std::size_t internal{0};
    for (std::size_t node = 0; node + 1 < used; ++node) {
      std::uint64_t weight{0};
      for (std::size_t child = 0; child < 2; ++child) {
        if (leaf < used &&
            (internal >= node || frequencies[this->symbols_[leaf]] <=
                                     this->internal_weights_[internal])) {
          this->leaf_parents_[leaf] = static_cast<std::uint16_t>(node);
          weight += frequencies[this->symbols_[leaf]];
          leaf += 1;
        } else {
          this->internal_parents_[internal] = static_cast<std::uint16_t>(node);
          weight += this->internal_weights_[internal];
          internal += 1;
        }
      }

      this->internal_weights_[node] = weight;
    }

    // Every internal node has a later parent, so walking backwards from the
    // root assigns every depth after the depth of its parent
    const std::size_t root{used - 2};
    this->internal_depths_[root] = 0;
    for (std::size_t node = root; node > 0; --node) {
      this->internal_depths_[node - 1] = static_cast<std::uint16_t>(
          this->internal_depths_[this->internal_parents_[node - 1]] + 1);
    }

    for (std::size_t index = 0; index < used; ++index) {
      const unsigned int depth{
          static_cast<unsigned int>(
              this->internal_depths_[this->leaf_parents_[index]]) +
          1U};
      if (depth > limit) {
        return false;
      }

      lengths[this->symbols_[index]] = static_cast<std::uint8_t>(depth);
    }

    return true;
  }

private:
  static constexpr std::size_t MAXIMUM_ITEMS{INFLATE_LITERAL_LENGTH_SYMBOLS *
                                             2};

  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS> symbols_{};
  std::array<std::array<std::uint64_t, MAXIMUM_ITEMS>,
             INFLATE_MAXIMUM_CODEWORD_LENGTH>
      weights_{};
  std::array<std::array<std::uint8_t, MAXIMUM_ITEMS>,
             INFLATE_MAXIMUM_CODEWORD_LENGTH>
      leaves_{};
  std::array<std::size_t, INFLATE_MAXIMUM_CODEWORD_LENGTH> sizes_{};
  std::array<std::uint64_t, INFLATE_LITERAL_LENGTH_SYMBOLS> internal_weights_{};
  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS> internal_parents_{};
  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS> internal_depths_{};
  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS> leaf_parents_{};
};

// Assigns canonical codewords as per RFC 1951 section 3.2.2, bit-reversed so
// they can be written starting from the least significant bit
inline auto deflate_canonical_codes(const std::uint8_t *lengths,
                                    const std::size_t symbol_count,
                                    std::uint16_t *codes) -> void {
  std::array<std::uint16_t, INFLATE_MAXIMUM_CODEWORD_LENGTH + 1> counts{};
  for (std::size_t symbol = 0; symbol < symbol_count; ++symbol) {
    counts[lengths[symbol]] += 1;
  }

  counts[0] = 0;
  std::array<std::uint32_t, INFLATE_MAXIMUM_CODEWORD_LENGTH + 1> next{};
  std::uint32_t code{0};
  for (unsigned int length = 1; length <= INFLATE_MAXIMUM_CODEWORD_LENGTH;
       ++length) {
    code = (code + counts[length - 1]) << 1;
    next[length] = code;
  }

  for (std::size_t symbol = 0; symbol < symbol_count; ++symbol) {
    const auto length{lengths[symbol]};
    if (length != 0) {
      codes[symbol] = static_cast<std::uint16_t>(
          inflate_reverse_bits(next[length], length));
      next[length] += 1;
    } else {
      codes[symbol] = 0;
    }
  }
}

// The fixed codes of RFC 1951 section 3.2.6
struct DeflateFixedCodes {
  std::array<std::uint8_t, INFLATE_LITERAL_LENGTH_SYMBOLS>
      literal_length_lengths{};
  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS>
      literal_length_codes{};
  std::array<std::uint8_t, INFLATE_DISTANCE_SYMBOLS> distance_lengths{};
  std::array<std::uint16_t, INFLATE_DISTANCE_SYMBOLS> distance_codes{};

  DeflateFixedCodes() {
    std::fill(this->literal_length_lengths.begin(),
              this->literal_length_lengths.begin() + 144, std::uint8_t{8});
    std::fill(this->literal_length_lengths.begin() + 144,
              this->literal_length_lengths.begin() + 256, std::uint8_t{9});
    std::fill(this->literal_length_lengths.begin() + 256,
              this->literal_length_lengths.begin() + 280, std::uint8_t{7});
    std::fill(this->literal_length_lengths.begin() + 280,
              this->literal_length_lengths.end(), std::uint8_t{8});
    deflate_canonical_codes(this->literal_length_lengths.data(),
                            this->literal_length_lengths.size(),
                            this->literal_length_codes.data());
    std::fill(this->distance_lengths.begin(), this->distance_lengths.end(),
              std::uint8_t{5});
    deflate_canonical_codes(this->distance_lengths.data(),
                            this->distance_lengths.size(),
                            this->distance_codes.data());
  }
};

// A code length sequence compressed with the repeat symbols of RFC 1951
// section 3.2.7
struct DeflateCodeLengthRun {
  std::array<std::uint8_t,
             INFLATE_LITERAL_LENGTH_SYMBOLS + INFLATE_DISTANCE_SYMBOLS>
      symbols{};
  std::array<std::uint8_t,
             INFLATE_LITERAL_LENGTH_SYMBOLS + INFLATE_DISTANCE_SYMBOLS>
      extra{};
  std::size_t size{0};

  auto push(const std::uint8_t symbol, const std::uint8_t value) -> void {
    this->symbols[this->size] = symbol;
    this->extra[this->size] = value;
    this->size += 1;
  }

  auto encode(const std::uint8_t *lengths, const std::size_t count) -> void {
    this->size = 0;
    std::size_t index{0};
    while (index < count) {
      const auto value{lengths[index]};
      std::size_t run{1};
      while (index + run < count && lengths[index + run] == value) {
        run += 1;
      }

      if (value == 0) {
        while (run >= 11) {
          const auto amount{std::min(run, std::size_t{138})};
          this->push(18, static_cast<std::uint8_t>(amount - 11));
          run -= amount;
          index += amount;
        }

        if (run >= 3) {
          this->push(17, static_cast<std::uint8_t>(run - 3));
          index += run;
          run = 0;
        }
      } else {
        this->push(value, 0);
        index += 1;
        run -= 1;
        while (run >= 3) {
          const auto amount{std::min(run, std::size_t{6})};
          this->push(16, static_cast<std::uint8_t>(amount - 3));
          run -= amount;
          index += amount;
        }
      }

      while (run > 0) {
        this->push(value, 0);
        index += 1;
        run -= 1;
      }
    }
  }
};

inline constexpr std::array<std::uint8_t, INFLATE_CODE_LENGTH_SYMBOLS>
    DEFLATE_CODE_LENGTH_EXTRA_BITS{
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7}};

// A run of literal bytes followed by a match, where a zero length marks a
// trailing run with no match
struct DeflateSequence {
  std::uint32_t literals;
  std::uint16_t length;
  std::uint16_t distance;
};

struct DeflateLevel {
  unsigned int search_depth;
  unsigned int nice_length;
  bool lazy;
};

// Levels 0 and 1 use dedicated strategies, so their entries are unused
inline constexpr std::array<DeflateLevel, 13> DEFLATE_LEVELS{
    {{.search_depth = 0, .nice_length = 0, .lazy = false},
     {.search_depth = 0, .nice_length = 0, .lazy = false},
     {.search_depth = 4, .nice_length = 16, .lazy = false},
     {.search_depth = 8, .nice_length = 32, .lazy = false},
     {.search_depth = 16, .nice_length = 64, .lazy = false},
     {.search_depth = 16, .nice_length = 32, .lazy = true},
     {.search_depth = 32, .nice_length = 128, .lazy = true},
     {.search_depth = 64, .nice_length = 128, .lazy = true},
     {.search_depth = 128, .nice_length = 258, .lazy = true},
     {.search_depth = 256, .nice_length = 258, .lazy = true},
     {.search_depth = 512, .nice_length = 258, .lazy = true},
     {.search_depth = 1024, .nice_length = 258, .lazy = true},
     {.search_depth = 4096, .nice_length = 258, .lazy = true}}};

// Compresses a whole buffer into a raw deflate stream (RFC 1951)
class DeflateEncoder {
public:
  // Symbols are accumulated for this much input before a block is emitted
  // with codes tailored to it. The fastest level uses shorter blocks with a cap
  // on their sequences, so its codes follow changes in the input more closely,
  // and a shorter remainder is merged into the last block instead of forming a
  // tiny one
  static constexpr std::size_t BLOCK_INPUT_LENGTH{262144};
  static constexpr std::size_t FAST_BLOCK_INPUT_LENGTH{65535};
  static constexpr std::size_t FAST_BLOCK_SEQUENCES{8192};
  static constexpr std::size_t MINIMUM_BLOCK_LENGTH{5000};

  // Every match covers at least four bytes and a block overshoots its input
  // length by at most one match, which bounds the sequences per block
  DeflateEncoder(const int level, const std::size_t size)
      : level_{level},
        sequences_(((std::min(size, BLOCK_INPUT_LENGTH + MINIMUM_BLOCK_LENGTH) +
                     DEFLATE_MAXIMUM_MATCH_LENGTH) /
                    4) +
                   2),
        head_(level == 0 ? 0 : HASH_SIZE),
        previous_(level <= 1 ? 0 : DEFLATE_WINDOW_SIZE) {}

  // The largest possible stream for an input of the given size, including a
  // whole word of room for flushing bits
  static auto bound(const std::size_t size) -> std::size_t {
    const std::size_t stored_chunks{(size + DEFLATE_MAXIMUM_STORED_LENGTH - 1) /
                                    DEFLATE_MAXIMUM_STORED_LENGTH};
    // A block that is not the last covers at least as many bytes as the
    // sequences it can hold, each of which spans at least four bytes
    const std::size_t blocks{(size / (FAST_BLOCK_SEQUENCES * 4)) + 1};
    return size + (5 * (stored_chunks + blocks)) + 16;
  }

  auto compress(const std::uint8_t *input, const std::size_t size,
                std::uint8_t *output) -> std::size_t {
    DeflateBitWriter writer{output};
    if (this->level_ == 0) {
      this->emit_stored(writer, input, size, true);
      writer.align();
      return static_cast<std::size_t>(writer.position() - output);
    }

    std::size_t position{0};
    while (true) {
      const std::size_t block_start{position};
      const std::size_t block_length{this->level_ == 1 ? FAST_BLOCK_INPUT_LENGTH
                                                       : BLOCK_INPUT_LENGTH};
      const std::size_t block_end{size - position <
                                          block_length + MINIMUM_BLOCK_LENGTH
                                      ? size
                                      : position + block_length};
      this->literal_length_frequencies_.fill(0);
      this->distance_frequencies_.fill(0);
      this->sequence_count_ = 0;
      if (this->level_ == 1) {
        position = this->match_fast(input, size, position, block_end);
      } else {
        position = this->match_chains(input, size, position, block_end);
      }

      const bool final{position >= size};
      this->emit_block(writer, input + block_start, position - block_start,
                       final);
      if (final) {
        break;
      }
    }

    writer.align();
    return static_cast<std::size_t>(writer.position() - output);
  }

private:
  static constexpr unsigned int HASH_BITS{16};
  static constexpr std::size_t HASH_SIZE{std::size_t{1} << HASH_BITS};
  static constexpr std::size_t WINDOW_MASK{DEFLATE_WINDOW_SIZE - 1};

  // The fastest level spreads the same table over twice as few buckets, each
  // holding the two most recent positions
  static constexpr unsigned int FAST_HASH_BITS{HASH_BITS - 1};
  static constexpr std::size_t FAST_NICE_LENGTH{32};

  static auto load_32(const std::uint8_t *data) -> std::uint32_t {
    std::uint32_t word{0};
    std::memcpy(&word, data, sizeof(word));
    if (std::endian::native == std::endian::big) {
      word = std::byteswap(word);
    }

    return word;
  }

  static auto load_64(const std::uint8_t *data) -> std::uint64_t {
    std::uint64_t word{0};
    std::memcpy(&word, data, sizeof(word));
    if (std::endian::native == std::endian::big) {
      word = std::byteswap(word);
    }

    return word;
  }

  static auto hash(const std::uint32_t word) -> std::uint32_t {
    return (word * 0x1e35a7bdU) >> (32 - HASH_BITS);
  }

  static auto hash_fast(const std::uint32_t word) -> std::uint32_t {
    return (word * 0x1e35a7bdU) >> (32 - FAST_HASH_BITS);
  }

  // Counts how many bytes match between two positions, up to a limit
  static auto extend(const std::uint8_t *left, const std::uint8_t *right,
                     const std::size_t limit) -> std::size_t {
    std::size_t length{0};
    while (length + 8 <= limit) {
      const auto difference{load_64(left + length) ^ load_64(right + length)};
      if (difference != 0) {
        return length +
               (static_cast<std::size_t>(std::countr_zero(difference)) / 8);
      }

      length += 8;
    }

    while (length < limit && left[length] == right[length]) {
      length += 1;
    }

    return length;
  }

  auto record_literal(const std::uint8_t byte) -> void {
    this->literal_length_frequencies_[byte] += 1;
    this->pending_literals_ += 1;
  }

  auto record_match(const std::size_t length, const std::size_t distance)
      -> void {
    this->literal_length_frequencies_[257 + DEFLATE_LENGTH_SLOTS[length]] += 1;
    this->distance_frequencies_[deflate_distance_slot(distance)] += 1;
    this->sequences_[this->sequence_count_] = {
        .literals = this->pending_literals_,
        .length = static_cast<std::uint16_t>(length),
        .distance = static_cast<std::uint16_t>(distance)};
    this->sequence_count_ += 1;
    this->pending_literals_ = 0;
  }

  auto finish_sequences() -> void {
    this->sequences_[this->sequence_count_] = {
        .literals = this->pending_literals_, .length = 0, .distance = 0};
    this->sequence_count_ += 1;
    this->pending_literals_ = 0;
  }

  // Greedy matching against the two most recent positions sharing a hash,
  // keeping the running state in locals as the tables could otherwise alias it
  auto match_fast(const std::uint8_t *input, const std::size_t size,
                  std::size_t position, const std::size_t block_end)
      -> std::size_t {
    auto *const table{this->head_.data()};
    auto *const literal_length_frequencies{
        this->literal_length_frequencies_.data()};
    auto *const distance_frequencies{this->distance_frequencies_.data()};
    auto *const sequences{this->sequences_.data()};
    std::size_t sequence_count{this->sequence_count_};
    std::uint32_t literals{this->pending_literals_};
    const std::size_t hash_end{size < 8 ? 0 : size - 7};
    const std::size_t match_end{std::min(block_end, hash_end)};
    while (position < match_end && sequence_count < FAST_BLOCK_SEQUENCES) {
      const auto word{load_32(input + position)};
      const std::size_t bucket{static_cast<std::size_t>(hash_fast(word)) * 2};
      const auto current{static_cast<std::uint32_t>(position)};
      const auto first{table[bucket]};
      const auto second{table[bucket + 1]};
      table[bucket + 1] = first;
      table[bucket] = current;
      const std::size_t limit{
          std::min(DEFLATE_MAXIMUM_MATCH_LENGTH, size - position)};
      std::size_t best_length{0};
      std::size_t best_distance{0};
      // Positions are stored modulo 2^32 and the second candidate is always
      // older than the first one, so it is only worth checking when the first
      // one is still within the window
      const std::uint32_t first_distance{current - first};
      if (first_distance - 1 < DEFLATE_WINDOW_SIZE) {
        const auto *const match{input + position - first_distance};
        if (load_32(match) == word) {
          best_length = 4 + extend(input + position + 4, match + 4, limit - 4);
          best_distance = first_distance;
        }

        const std::uint32_t second_distance{current - second};
        if (best_length < std::min(FAST_NICE_LENGTH, limit) &&
            second_distance - 1 < DEFLATE_WINDOW_SIZE) {
          const auto *const other{input + position - second_distance};
          if (load_32(other) == word &&
              other[best_length] == input[position + best_length]) {
            const std::size_t length{
                4 + extend(input + position + 4, other + 4, limit - 4)};
            if (length > best_length) {
              best_length = length;
              best_distance = second_distance;
            }
          }
        }
      }

      if (best_length == 0) {
        literal_length_frequencies[input[position]] += 1;
        literals += 1;
        position += 1;
        continue;
      }

      literal_length_frequencies[257 + DEFLATE_LENGTH_SLOTS[best_length]] += 1;
      distance_frequencies[deflate_distance_slot(best_distance)] += 1;
      sequences[sequence_count] = {
          .literals = literals,
          .length = static_cast<std::uint16_t>(best_length),
          .distance = static_cast<std::uint16_t>(best_distance)};
      sequence_count += 1;
      literals = 0;
      const std::size_t end{position + best_length};
      const std::size_t insert_end{std::min(end, hash_end)};
      for (position += 1; position < insert_end; ++position) {
        const std::size_t covered{
            static_cast<std::size_t>(hash_fast(load_32(input + position))) * 2};
        table[covered + 1] = table[covered];
        table[covered] = static_cast<std::uint32_t>(position);
      }

      position = end;
    }

    while (position < block_end && sequence_count < FAST_BLOCK_SEQUENCES) {
      literal_length_frequencies[input[position]] += 1;
      literals += 1;
      position += 1;
    }

    this->sequence_count_ = sequence_count;
    this->pending_literals_ = literals;
    this->finish_sequences();
    return position;
  }

  auto insert(const std::uint8_t *input, const std::size_t position) -> void {
    const auto slot{hash(load_32(input + position))};
    this->previous_[position & WINDOW_MASK] = this->head_[slot];
    this->head_[slot] = static_cast<std::uint32_t>(position);
  }

  // Walks the hash chain from the most recent position, stopping once the
  // chain no longer moves further back in the window
  auto find(const std::uint8_t *input, const std::size_t size,
            const std::size_t position, std::size_t &best_distance)
      -> std::size_t {
    const auto &settings{
        DEFLATE_LEVELS[static_cast<std::size_t>(this->level_)]};
    const std::size_t limit{
        std::min(DEFLATE_MAXIMUM_MATCH_LENGTH, size - position)};
    const auto word{load_32(input + position)};
    std::size_t best_length{0};
    std::size_t candidate{this->head_[hash(word)]};
    std::size_t previous_distance{0};
    for (unsigned int depth = 0; depth < settings.search_depth; ++depth) {
      const std::size_t distance{
          static_cast<std::uint32_t>(static_cast<std::uint32_t>(position) -
                                     static_cast<std::uint32_t>(candidate))};
      if (distance <= previous_distance || distance > DEFLATE_WINDOW_SIZE ||
          distance > position) {
        break;
      }

      const auto *match{input + position - distance};
      if ((best_length == 0 ||
           match[best_length] == input[position + best_length]) &&
          load_32(match) == word) {
        const std::size_t length{
            4 + extend(input + position + 4, match + 4, limit - 4)};
        if (length > best_length) {
          best_length = length;
          best_distance = distance;
          if (length >= settings.nice_length || length == limit) {
            break;
          }
        }
      }

      previous_distance = distance;
      candidate = this->previous_[candidate & WINDOW_MASK];
    }

    return best_length;
  }

  auto match_chains(const std::uint8_t *input, const std::size_t size,
                    std::size_t position, const std::size_t block_end)
      -> std::size_t {
    const auto &settings{
        DEFLATE_LEVELS[static_cast<std::size_t>(this->level_)]};
    std::size_t previous_length{0};
    std::size_t previous_distance{0};
    bool previous_available{false};
    while (position < block_end) {
      if (size - position < 8) {
        if (previous_available) {
          if (previous_length >= 4) {
            break;
          }

          this->record_literal(input[position - 1]);
          previous_available = false;
        }

        this->record_literal(input[position]);
        position += 1;
        continue;
      }

      std::size_t distance{0};
      std::size_t length{this->find(input, size, position, distance)};
      this->insert(input, position);
      if (length > 0 && length < 4) {
        length = 0;
      }

      if (previous_available && previous_length >= 4 &&
          length <= previous_length) {
        // The match found one position earlier is at least as good
        const std::size_t start{position - 1};
        this->record_match(previous_length, previous_distance);
        const std::size_t end{start + previous_length};
        position += 1;
        while (position < end && size - position >= 8) {
          this->insert(input, position);
          position += 1;
        }

        position = end;
        previous_available = false;
        previous_length = 0;
        continue;
      }

      if (!settings.lazy) {
        if (length < 4) {
          this->record_literal(input[position]);
          position += 1;
          continue;
        }

        this->record_match(length, distance);
        const std::size_t end{position + length};
        position += 1;
        while (position < end && size - position >= 8) {
          this->insert(input, position);
          position += 1;
        }

        position = end;
        continue;
      }

      if (previous_available) {
        this->record_literal(input[position - 1]);
      }

      if (length >= settings.nice_length) {
        this->record_match(length, distance);
        const std::size_t end{position + length};
        position += 1;
        while (position < end && size - position >= 8) {
          this->insert(input, position);
          position += 1;
        }

        position = end;
        previous_available = false;
        previous_length = 0;
        continue;
      }

      previous_available = true;
      previous_length = length;
      previous_distance = distance;
      position += 1;
    }

    if (previous_available) {
      if (previous_length >= 4 && position - 1 + previous_length <= size) {
        const std::size_t start{position - 1};
        this->record_match(previous_length, previous_distance);
        position = start + previous_length;
      } else {
        this->record_literal(input[position - 1]);
      }
    }

    this->finish_sequences();
    return position;
  }

  auto emit_stored(DeflateBitWriter &writer, const std::uint8_t *data,
                   std::size_t size, const bool final) -> void {
    while (true) {
      const std::size_t chunk{std::min(size, DEFLATE_MAXIMUM_STORED_LENGTH)};
      const bool last{final && chunk == size};
      writer.add(last ? 1U : 0U, 1);
      writer.add(0, 2);
      writer.align();
      const std::array<std::uint8_t, 4> header{
          {static_cast<std::uint8_t>(chunk & 0xff),
           static_cast<std::uint8_t>((chunk >> 8) & 0xff),
           static_cast<std::uint8_t>(~chunk & 0xff),
           static_cast<std::uint8_t>((~chunk >> 8) & 0xff)}};
      writer.bytes(header.data(), header.size());
      writer.bytes(data, chunk);
      data += chunk;
      size -= chunk;
      if (size == 0) {
        break;
      }
    }
  }

  static auto stored_cost(const unsigned int pending_bits,
                          const std::size_t size) -> std::uint64_t {
    const std::size_t chunks{size == 0
                                 ? 1
                                 : (size + DEFLATE_MAXIMUM_STORED_LENGTH - 1) /
                                       DEFLATE_MAXIMUM_STORED_LENGTH};
    const std::uint64_t first_padding{(8 - ((pending_bits + 3) % 8)) % 8};
    return 3 + first_padding + 32 + ((chunks - 1) * 40) +
           (static_cast<std::uint64_t>(size) * 8);
  }

  // Works on a local copy of the writer so that its state can live in
  // registers instead of memory
  auto emit_symbols(DeflateBitWriter &destination, const std::uint8_t *data,
                    const std::uint8_t *literal_length_lengths,
                    const std::uint16_t *literal_length_codes,
                    const std::uint8_t *distance_lengths,
                    const std::uint16_t *distance_codes) -> void {
    DeflateBitWriter writer{destination};
    const auto *literal{data};
    for (std::size_t index = 0; index < this->sequence_count_; ++index) {
      const auto &sequence{this->sequences_[index]};
      // Three codewords of at most 15 bits always fit next to the at most seven
      // bits that a flush leaves in the buffer
      std::uint32_t count{0};
      while (count + 3 <= sequence.literals) {
        writer.add(literal_length_codes[literal[0]],
                   literal_length_lengths[literal[0]]);
        writer.add(literal_length_codes[literal[1]],
                   literal_length_lengths[literal[1]]);
        writer.add(literal_length_codes[literal[2]],
                   literal_length_lengths[literal[2]]);
        writer.flush();
        literal += 3;
        count += 3;
      }

      while (count < sequence.literals) {
        writer.add(literal_length_codes[*literal],
                   literal_length_lengths[*literal]);
        writer.flush();
        literal += 1;
        count += 1;
      }

      if (sequence.length == 0) {
        continue;
      }

      const std::size_t length_slot{DEFLATE_LENGTH_SLOTS[sequence.length]};
      const std::size_t length_symbol{257 + length_slot};
      writer.add(literal_length_codes[length_symbol],
                 literal_length_lengths[length_symbol]);
      writer.add(static_cast<std::uint32_t>(sequence.length -
                                            INFLATE_LENGTH_BASE[length_slot]),
                 INFLATE_LENGTH_EXTRA_BITS[length_slot]);
      writer.flush();
      const auto distance_slot{deflate_distance_slot(sequence.distance)};
      writer.add(distance_codes[distance_slot],
                 distance_lengths[distance_slot]);
      writer.add(static_cast<std::uint32_t>(
                     sequence.distance - INFLATE_DISTANCE_BASE[distance_slot]),
                 INFLATE_DISTANCE_EXTRA_BITS[distance_slot]);
      writer.flush();
      literal += sequence.length;
    }

    writer.add(literal_length_codes[DEFLATE_END_OF_BLOCK],
               literal_length_lengths[DEFLATE_END_OF_BLOCK]);
    writer.flush();
    destination = writer;
  }

  // Emits the block with whichever of the stored, fixed, and dynamic
  // encodings of RFC 1951 section 3.2.3 is the smallest
  auto emit_block(DeflateBitWriter &writer, const std::uint8_t *data,
                  const std::size_t size, const bool final) -> void {
    auto &frequencies{this->literal_length_frequencies_};
    frequencies[DEFLATE_END_OF_BLOCK] = 1;

    std::uint64_t extra_bits{0};
    for (std::size_t slot = 0; slot < INFLATE_LENGTH_BASE.size(); ++slot) {
      extra_bits += static_cast<std::uint64_t>(frequencies[257 + slot]) *
                    INFLATE_LENGTH_EXTRA_BITS[slot];
    }

    for (std::size_t slot = 0; slot < INFLATE_DISTANCE_BASE.size(); ++slot) {
      extra_bits +=
          static_cast<std::uint64_t>(this->distance_frequencies_[slot]) *
          INFLATE_DISTANCE_EXTRA_BITS[slot];
    }

    this->code_builder_.lengths(frequencies.data(), 286,
                                INFLATE_MAXIMUM_CODEWORD_LENGTH,
                                this->literal_length_lengths_.data());
    this->code_builder_.lengths(this->distance_frequencies_.data(), 30,
                                INFLATE_MAXIMUM_CODEWORD_LENGTH,
                                this->distance_lengths_.data());

    std::size_t literal_length_count{286};
    while (literal_length_count > 257 &&
           this->literal_length_lengths_[literal_length_count - 1] == 0) {
      literal_length_count -= 1;
    }

    std::size_t distance_count{30};
    while (distance_count > 1 &&
           this->distance_lengths_[distance_count - 1] == 0) {
      distance_count -= 1;
    }

    std::array<std::uint8_t,
               INFLATE_LITERAL_LENGTH_SYMBOLS + INFLATE_DISTANCE_SYMBOLS>
        all_lengths{};
    std::memcpy(all_lengths.data(), this->literal_length_lengths_.data(),
                literal_length_count);
    std::memcpy(all_lengths.data() + literal_length_count,
                this->distance_lengths_.data(), distance_count);
    this->run_.encode(all_lengths.data(),
                      literal_length_count + distance_count);

    std::array<std::uint32_t, INFLATE_CODE_LENGTH_SYMBOLS>
        code_length_frequencies{};
    for (std::size_t index = 0; index < this->run_.size; ++index) {
      code_length_frequencies[this->run_.symbols[index]] += 1;
    }

    std::array<std::uint8_t, INFLATE_CODE_LENGTH_SYMBOLS> code_length_lengths{};
    this->code_builder_.lengths(code_length_frequencies.data(),
                                INFLATE_CODE_LENGTH_SYMBOLS, 7,
                                code_length_lengths.data());
    std::size_t code_length_count{INFLATE_CODE_LENGTH_SYMBOLS};
    while (
        code_length_count > 4 &&
        code_length_lengths[INFLATE_CODE_LENGTH_ORDER[code_length_count - 1]] ==
            0) {
      code_length_count -= 1;
    }

    std::uint64_t dynamic_bits{17 + (3 * code_length_count) + extra_bits};
    for (std::size_t symbol = 0; symbol < INFLATE_CODE_LENGTH_SYMBOLS;
         ++symbol) {
      dynamic_bits +=
          static_cast<std::uint64_t>(code_length_frequencies[symbol]) *
          (code_length_lengths[symbol] +
           DEFLATE_CODE_LENGTH_EXTRA_BITS[symbol]);
    }

    std::uint64_t fixed_bits{3 + extra_bits};
    for (std::size_t symbol = 0; symbol < 286; ++symbol) {
      dynamic_bits += static_cast<std::uint64_t>(frequencies[symbol]) *
                      this->literal_length_lengths_[symbol];
      fixed_bits += static_cast<std::uint64_t>(frequencies[symbol]) *
                    this->fixed_.literal_length_lengths[symbol];
    }

    for (std::size_t symbol = 0; symbol < 30; ++symbol) {
      dynamic_bits +=
          static_cast<std::uint64_t>(this->distance_frequencies_[symbol]) *
          this->distance_lengths_[symbol];
      fixed_bits +=
          static_cast<std::uint64_t>(this->distance_frequencies_[symbol]) * 5;
    }

    const auto stored_bits{stored_cost(writer.pending_bits(), size)};
    if (stored_bits <= fixed_bits && stored_bits <= dynamic_bits) {
      this->emit_stored(writer, data, size, final);
      return;
    }

    if (fixed_bits <= dynamic_bits) {
      writer.add(final ? 1U : 0U, 1);
      writer.add(1, 2);
      writer.flush();
      this->emit_symbols(writer, data,
                         this->fixed_.literal_length_lengths.data(),
                         this->fixed_.literal_length_codes.data(),
                         this->fixed_.distance_lengths.data(),
                         this->fixed_.distance_codes.data());
      return;
    }

    writer.add(final ? 1U : 0U, 1);
    writer.add(2, 2);
    writer.add(static_cast<std::uint32_t>(literal_length_count - 257), 5);
    writer.add(static_cast<std::uint32_t>(distance_count - 1), 5);
    writer.add(static_cast<std::uint32_t>(code_length_count - 4), 4);
    writer.flush();
    for (std::size_t index = 0; index < code_length_count; ++index) {
      writer.add(code_length_lengths[INFLATE_CODE_LENGTH_ORDER[index]], 3);
      writer.flush();
    }

    std::array<std::uint16_t, INFLATE_CODE_LENGTH_SYMBOLS> code_length_codes{};
    deflate_canonical_codes(code_length_lengths.data(),
                            INFLATE_CODE_LENGTH_SYMBOLS,
                            code_length_codes.data());
    for (std::size_t index = 0; index < this->run_.size; ++index) {
      const auto symbol{this->run_.symbols[index]};
      writer.add(code_length_codes[symbol], code_length_lengths[symbol]);
      writer.add(this->run_.extra[index],
                 DEFLATE_CODE_LENGTH_EXTRA_BITS[symbol]);
      writer.flush();
    }

    deflate_canonical_codes(this->literal_length_lengths_.data(),
                            INFLATE_LITERAL_LENGTH_SYMBOLS,
                            this->literal_length_codes_.data());
    deflate_canonical_codes(this->distance_lengths_.data(),
                            INFLATE_DISTANCE_SYMBOLS,
                            this->distance_codes_.data());
    this->emit_symbols(writer, data, this->literal_length_lengths_.data(),
                       this->literal_length_codes_.data(),
                       this->distance_lengths_.data(),
                       this->distance_codes_.data());
  }

  int level_;
  std::vector<DeflateSequence> sequences_;
  std::size_t sequence_count_{0};
  std::uint32_t pending_literals_{0};
  std::vector<std::uint32_t> head_;
  std::vector<std::uint32_t> previous_;
  std::array<std::uint32_t, INFLATE_LITERAL_LENGTH_SYMBOLS>
      literal_length_frequencies_{};
  std::array<std::uint32_t, INFLATE_DISTANCE_SYMBOLS> distance_frequencies_{};
  std::array<std::uint8_t, INFLATE_LITERAL_LENGTH_SYMBOLS>
      literal_length_lengths_{};
  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS>
      literal_length_codes_{};
  std::array<std::uint8_t, INFLATE_DISTANCE_SYMBOLS> distance_lengths_{};
  std::array<std::uint16_t, INFLATE_DISTANCE_SYMBOLS> distance_codes_{};
  DeflateCodeBuilder code_builder_;
  DeflateCodeLengthRun run_;
  DeflateFixedCodes fixed_;
};

} // namespace sourcemeta::core

#endif
