#ifndef SOURCEMETA_CORE_GZIP_DEFLATE_H_
#define SOURCEMETA_CORE_GZIP_DEFLATE_H_

#include "bit_reader.h"
#include "huffman.h"

#include <sourcemeta/core/gzip_error.h>

#include <algorithm> // std::min
#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t, std::uint16_t
#include <cstring>   // std::memcpy

namespace sourcemeta::core {

inline constexpr std::size_t DEFLATE_WINDOW_SIZE{32768};
inline constexpr std::size_t DEFLATE_WINDOW_MASK{DEFLATE_WINDOW_SIZE - 1};
static_assert((DEFLATE_WINDOW_SIZE & DEFLATE_WINDOW_MASK) == 0,
              "DEFLATE_WINDOW_SIZE must be a power of two");

// RFC 1951 section 3.2.5 length codes 257-285
inline constexpr std::array<std::uint16_t, 29> DEFLATE_LENGTH_BASE{
    {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
     31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258}};

inline constexpr std::array<std::uint8_t, 29> DEFLATE_LENGTH_EXTRA{
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
     2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0}};

// RFC 1951 section 3.2.5 distance codes 0-29
inline constexpr std::array<std::uint16_t, 30> DEFLATE_DISTANCE_BASE{
    {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
     33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
     1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577}};

inline constexpr std::array<std::uint8_t, 30> DEFLATE_DISTANCE_EXTRA{
    {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
     6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13}};

// RFC 1951 section 3.2.7 code-length-of-codes order
inline constexpr std::array<std::uint8_t, 19> DEFLATE_CODE_LENGTH_ORDER{
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}};

class DeflateDecoder {
public:
  DeflateDecoder(BitReader &reader) : reader_{&reader} {}

  auto decompress(std::uint8_t *output, const std::size_t output_size)
      -> std::size_t {
    std::size_t produced{0};

    while (!this->stream_ended_) {
      switch (this->state_) {
        case State::BlockHeader:
          if (this->final_block_) {
            this->state_ = State::End;
          } else {
            this->start_block();
          }
          break;
        case State::StoredBlock:
          this->process_stored_block(output, output_size, produced);
          if (this->state_ == State::StoredBlock) {
            return produced;
          }
          break;
        case State::HuffmanBlock:
          this->process_huffman_block(output, output_size, produced);
          if (this->state_ == State::HuffmanBlock) {
            return produced;
          }
          break;
        case State::End:
          this->reader_->align_to_byte();
          this->stream_ended_ = true;
          return produced;
      }
    }
    return produced;
  }

  [[nodiscard]] auto stream_ended() const -> bool {
    return this->stream_ended_;
  }

  auto reset() -> void {
    this->state_ = State::BlockHeader;
    this->final_block_ = false;
    this->stream_ended_ = false;
    this->stored_remaining_ = 0;
    this->pending_copy_length_ = 0;
    this->pending_copy_distance_ = 0;
    this->window_position_ = 0;
    this->bytes_written_ = 0;
  }

private:
  enum class State : std::uint8_t {
    BlockHeader,
    StoredBlock,
    HuffmanBlock,
    End,
  };

  auto start_block() -> void {
    this->final_block_ = this->reader_->read_bits(1) != 0;
    const auto btype{this->reader_->read_bits(2)};
    switch (btype) {
      case 0:
        this->start_stored_block();
        break;
      case 1:
        this->build_fixed_trees();
        this->state_ = State::HuffmanBlock;
        break;
      case 2:
        this->read_dynamic_header();
        this->state_ = State::HuffmanBlock;
        break;
      default:
        throw GZIPError{"Reserved deflate block type"};
    }
  }

  auto start_stored_block() -> void {
    this->reader_->align_to_byte();
    this->stored_remaining_ =
        static_cast<std::uint16_t>(this->reader_->read_byte()) |
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(this->reader_->read_byte()) << 8);
    const auto nlen_lo{this->reader_->read_byte()};
    const auto nlen_hi{this->reader_->read_byte()};
    const std::uint16_t nlen{static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(nlen_lo) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(nlen_hi) << 8))};
    const std::uint16_t expected_nlen{
        static_cast<std::uint16_t>(~this->stored_remaining_)};
    if (expected_nlen != nlen) {
      throw GZIPError{"Stored block LEN/NLEN mismatch"};
    }
    this->state_ = State::StoredBlock;
  }

  auto build_fixed_trees() -> void {
    std::array<std::uint8_t, 288> literal_lengths{};
    for (std::size_t index = 0; index < 144; ++index) {
      literal_lengths[index] = 8;
    }
    for (std::size_t index = 144; index < 256; ++index) {
      literal_lengths[index] = 9;
    }
    for (std::size_t index = 256; index < 280; ++index) {
      literal_lengths[index] = 7;
    }
    for (std::size_t index = 280; index < 288; ++index) {
      literal_lengths[index] = 8;
    }
    this->literal_length_tree_.build(literal_lengths.data(),
                                     literal_lengths.size());

    std::array<std::uint8_t, 30> distance_lengths{};
    for (auto &length : distance_lengths) {
      length = 5;
    }
    this->distance_tree_.build(distance_lengths.data(), distance_lengths.size(),
                               true);
  }

  auto read_dynamic_header() -> void {
    const auto hlit{this->reader_->read_bits(5) + 257};
    const auto hdist{this->reader_->read_bits(5) + 1};
    const auto hclen{this->reader_->read_bits(4) + 4};

    // RFC 1951 section 3.2.7 caps the literal/length alphabet at 286 symbols
    if (hlit > 286) {
      throw GZIPError{"Too many literal/length codes"};
    }

    std::array<std::uint8_t, 19> code_length_lengths{};
    for (std::size_t index = 0; index < hclen; ++index) {
      code_length_lengths[DEFLATE_CODE_LENGTH_ORDER[index]] =
          static_cast<std::uint8_t>(this->reader_->read_bits(3));
    }

    HuffmanDecoder code_length_tree;
    code_length_tree.build(code_length_lengths.data(),
                           code_length_lengths.size());

    std::array<std::uint8_t, 288 + 32> all_lengths{};
    std::size_t index{0};
    while (index < hlit + hdist) {
      const auto symbol{code_length_tree.decode(*this->reader_)};
      if (symbol < 16) {
        all_lengths[index++] = static_cast<std::uint8_t>(symbol);
      } else if (symbol == 16) {
        if (index == 0) {
          throw GZIPError{"Repeat-previous code length with no previous"};
        }
        const auto previous{all_lengths[index - 1]};
        const auto repeats{this->reader_->read_bits(2) + 3};
        for (std::size_t step = 0; step < repeats; ++step) {
          if (index >= all_lengths.size()) {
            throw GZIPError{"Code length count overflow"};
          }
          all_lengths[index++] = previous;
        }
      } else if (symbol == 17) {
        const auto repeats{this->reader_->read_bits(3) + 3};
        for (std::size_t step = 0; step < repeats; ++step) {
          if (index >= all_lengths.size()) {
            throw GZIPError{"Code length count overflow"};
          }
          all_lengths[index++] = 0;
        }
      } else if (symbol == 18) {
        const auto repeats{this->reader_->read_bits(7) + 11};
        for (std::size_t step = 0; step < repeats; ++step) {
          if (index >= all_lengths.size()) {
            throw GZIPError{"Code length count overflow"};
          }
          all_lengths[index++] = 0;
        }
      } else {
        throw GZIPError{"Invalid code length symbol"};
      }
    }

    if (index != hlit + hdist) {
      throw GZIPError{"Code length count overflow"};
    }

    this->literal_length_tree_.build(all_lengths.data(), hlit);
    this->distance_tree_.build(all_lengths.data() + hlit, hdist);
  }

  auto process_stored_block(std::uint8_t *output, const std::size_t output_size,
                            std::size_t &produced) -> void {
    while (this->stored_remaining_ > 0 && produced < output_size) {
      const auto byte{this->reader_->read_byte()};
      this->emit(byte, output, output_size, produced);
      --this->stored_remaining_;
    }
    if (this->stored_remaining_ == 0) {
      this->state_ = State::BlockHeader;
    }
  }

  auto process_huffman_block(std::uint8_t *output,
                             const std::size_t output_size,
                             std::size_t &produced) -> void {
    if (this->pending_copy_length_ > 0) {
      this->copy_backref(output, output_size, produced);
      if (this->pending_copy_length_ > 0) {
        return;
      }
    }

    while (produced < output_size) {
      const auto symbol{this->literal_length_tree_.decode(*this->reader_)};
      if (symbol < 256) {
        this->window_[this->window_position_] =
            static_cast<std::uint8_t>(symbol);
        this->window_position_ =
            (this->window_position_ + 1) & DEFLATE_WINDOW_MASK;
        if (this->bytes_written_ < DEFLATE_WINDOW_SIZE) {
          ++this->bytes_written_;
        }
        output[produced++] = static_cast<std::uint8_t>(symbol);
      } else if (symbol == 256) {
        this->state_ = State::BlockHeader;
        return;
      } else if (symbol <= 285) {
        const auto length_index{static_cast<std::size_t>(symbol - 257)};
        const std::size_t length{
            static_cast<std::size_t>(DEFLATE_LENGTH_BASE[length_index]) +
            static_cast<std::size_t>(
                this->reader_->read_bits(DEFLATE_LENGTH_EXTRA[length_index]))};
        const auto distance_symbol{this->distance_tree_.decode(*this->reader_)};
        if (distance_symbol >= 30) {
          throw GZIPError{"Invalid distance code"};
        }
        const std::size_t distance{
            static_cast<std::size_t>(DEFLATE_DISTANCE_BASE[distance_symbol]) +
            static_cast<std::size_t>(this->reader_->read_bits(
                DEFLATE_DISTANCE_EXTRA[distance_symbol]))};
        if (distance > this->bytes_written_) {
          throw GZIPError{"Backref distance exceeds bytes available"};
        }
        this->pending_copy_length_ = length;
        this->pending_copy_distance_ = distance;
        this->copy_backref(output, output_size, produced);
        if (this->pending_copy_length_ > 0) {
          return;
        }
      } else {
        throw GZIPError{"Invalid literal/length code"};
      }
    }
  }

  auto copy_backref(std::uint8_t *output, const std::size_t output_size,
                    std::size_t &produced) -> void {
    const auto remaining{output_size - produced};
    const auto to_copy{std::min(this->pending_copy_length_, remaining)};

    if (this->pending_copy_distance_ >= this->pending_copy_length_) {
      // Source range does not overlap with the bytes about to be written
      this->copy_backref_non_overlapping(output, produced, to_copy);
    } else {
      // RLE-style overlap: must propagate byte by byte
      this->copy_backref_overlapping(output, output_size, produced);
    }
  }

  auto copy_backref_non_overlapping(std::uint8_t *output, std::size_t &produced,
                                    const std::size_t to_copy) -> void {
    const std::size_t source_position{(this->window_position_ +
                                       DEFLATE_WINDOW_SIZE -
                                       this->pending_copy_distance_) &
                                      DEFLATE_WINDOW_MASK};

    // Copy from circular window into linear output (one or two contiguous
    // chunks depending on whether the source range wraps the window)
    const std::size_t source_first{
        std::min(to_copy, DEFLATE_WINDOW_SIZE - source_position)};
    std::memcpy(output + produced, this->window_.data() + source_position,
                source_first);
    if (source_first < to_copy) {
      std::memcpy(output + produced + source_first, this->window_.data(),
                  to_copy - source_first);
    }

    // Mirror the freshly written bytes back into the circular window
    const std::size_t dest_first{
        std::min(to_copy, DEFLATE_WINDOW_SIZE - this->window_position_)};
    std::memcpy(this->window_.data() + this->window_position_,
                output + produced, dest_first);
    if (dest_first < to_copy) {
      std::memcpy(this->window_.data(), output + produced + dest_first,
                  to_copy - dest_first);
    }

    produced += to_copy;
    this->window_position_ =
        (this->window_position_ + to_copy) & DEFLATE_WINDOW_MASK;
    this->bytes_written_ =
        std::min(this->bytes_written_ + to_copy, DEFLATE_WINDOW_SIZE);
    this->pending_copy_length_ -= to_copy;
  }

  auto copy_backref_overlapping(std::uint8_t *output,
                                const std::size_t output_size,
                                std::size_t &produced) -> void {
    std::size_t source_position{(this->window_position_ + DEFLATE_WINDOW_SIZE -
                                 this->pending_copy_distance_) &
                                DEFLATE_WINDOW_MASK};
    while (this->pending_copy_length_ > 0 && produced < output_size) {
      const auto byte{this->window_[source_position]};
      this->window_[this->window_position_] = byte;
      this->window_position_ =
          (this->window_position_ + 1) & DEFLATE_WINDOW_MASK;
      source_position = (source_position + 1) & DEFLATE_WINDOW_MASK;
      if (this->bytes_written_ < DEFLATE_WINDOW_SIZE) {
        ++this->bytes_written_;
      }
      output[produced++] = byte;
      --this->pending_copy_length_;
    }
  }

  auto emit(const std::uint8_t byte, std::uint8_t *output,
            const std::size_t output_size, std::size_t &produced) -> bool {
    this->window_[this->window_position_] = byte;
    this->window_position_ = (this->window_position_ + 1) & DEFLATE_WINDOW_MASK;
    if (this->bytes_written_ < DEFLATE_WINDOW_SIZE) {
      ++this->bytes_written_;
    }
    if (produced < output_size) {
      output[produced++] = byte;
      return true;
    }
    return false;
  }

  BitReader *reader_;
  State state_{State::BlockHeader};
  bool final_block_{false};
  bool stream_ended_{false};

  std::uint16_t stored_remaining_{0};

  HuffmanDecoder literal_length_tree_{};
  HuffmanDecoder distance_tree_{};

  std::size_t pending_copy_length_{0};
  std::size_t pending_copy_distance_{0};

  std::array<std::uint8_t, DEFLATE_WINDOW_SIZE> window_{};
  std::size_t window_position_{0};
  // Bytes written into the sliding window since the last reset, capped at
  // DEFLATE_WINDOW_SIZE. Used to reject back-references whose distance
  // exceeds the data we have actually produced for the current member
  std::size_t bytes_written_{0};
};

} // namespace sourcemeta::core

#endif
