#ifndef SOURCEMETA_CORE_GZIP_INFLATE_H_
#define SOURCEMETA_CORE_GZIP_INFLATE_H_

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/gzip_error.h>

#include <algorithm> // std::min
#include <array>     // std::array
#include <bit>       // std::endian, std::byteswap, std::countl_zero
#include <cstddef>   // std::size_t
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <cstring> // std::memcpy, std::memset, std::memchr
#include <string_view> // std::string_view
#include <utility>     // std::cmp_greater

namespace sourcemeta::core {

// A decode table entry packs the symbol value in the upper 16 bits, flags in
// bits 13 to 15 and 5 to 7, the number of codeword bits in bits 8 to 12, and
// the number of codeword and extra bits to consume in the lowest 5 bits
inline constexpr std::uint32_t INFLATE_ENTRY_END_OF_BLOCK{0x20};
inline constexpr std::uint32_t INFLATE_ENTRY_EXCEPTIONAL{0x40};
inline constexpr std::uint32_t INFLATE_ENTRY_LITERAL{0x80};
inline constexpr std::uint32_t INFLATE_ENTRY_SUBTABLE{0x2000};
inline constexpr std::uint32_t INFLATE_ENTRY_INVALID_CODE{0x4000};
inline constexpr std::uint32_t INFLATE_ENTRY_INVALID_SYMBOL{0x8000};

// RFC 1951 section 3.2.7 limits every codeword to 15 bits
inline constexpr unsigned int INFLATE_MAXIMUM_CODEWORD_LENGTH{15};

// Codes longer than the primary index continue into fixed size subtables. The
// number of subtables is bounded by the number of symbols, as every subtable
// holds at least one codeword
inline constexpr unsigned int INFLATE_LITERAL_LENGTH_TABLE_BITS{11};
inline constexpr unsigned int INFLATE_DISTANCE_TABLE_BITS{8};
inline constexpr unsigned int INFLATE_CODE_LENGTH_TABLE_BITS{7};
inline constexpr std::size_t INFLATE_LITERAL_LENGTH_SYMBOLS{288};
inline constexpr std::size_t INFLATE_DISTANCE_SYMBOLS{32};
inline constexpr std::size_t INFLATE_CODE_LENGTH_SYMBOLS{19};
inline constexpr std::size_t INFLATE_LITERAL_LENGTH_TABLE_SIZE{
    (std::size_t{1} << INFLATE_LITERAL_LENGTH_TABLE_BITS) +
    (INFLATE_LITERAL_LENGTH_SYMBOLS *
     (std::size_t{1} << (INFLATE_MAXIMUM_CODEWORD_LENGTH -
                         INFLATE_LITERAL_LENGTH_TABLE_BITS)))};
inline constexpr std::size_t INFLATE_DISTANCE_TABLE_SIZE{
    (std::size_t{1} << INFLATE_DISTANCE_TABLE_BITS) +
    (INFLATE_DISTANCE_SYMBOLS *
     (std::size_t{1} << (INFLATE_MAXIMUM_CODEWORD_LENGTH -
                         INFLATE_DISTANCE_TABLE_BITS)))};
inline constexpr std::size_t INFLATE_CODE_LENGTH_TABLE_SIZE{
    std::size_t{1} << INFLATE_CODE_LENGTH_TABLE_BITS};

// RFC 1951 section 3.2.5
inline constexpr std::array<std::uint16_t, 29> INFLATE_LENGTH_BASE{
    {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
     31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258}};
inline constexpr std::array<std::uint8_t, 29> INFLATE_LENGTH_EXTRA_BITS{
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
     2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0}};
inline constexpr std::array<std::uint16_t, 30> INFLATE_DISTANCE_BASE{
    {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
     33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
     1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577}};
inline constexpr std::array<std::uint8_t, 30> INFLATE_DISTANCE_EXTRA_BITS{
    {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
     6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13}};

// RFC 1951 section 3.2.7
inline constexpr std::array<std::uint8_t, 19> INFLATE_CODE_LENGTH_ORDER{
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}};

// The part of the decode table entry of every symbol that does not depend on
// its codeword, packing the literal byte, or the base and the number of extra
// bits of a length or a distance as per RFC 1951 section 3.2.5. Symbols 286
// and 287 of the literal/length alphabet and symbols 30 and 31 of the distance
// alphabet never occur in compressed data as per RFC 1951 section 3.2.6
inline constexpr std::array<std::uint32_t, INFLATE_LITERAL_LENGTH_SYMBOLS>
    INFLATE_LITERAL_LENGTH_RESULTS{
        {0x00000080U,
         0x00010080U,
         0x00020080U,
         0x00030080U,
         0x00040080U,
         0x00050080U,
         0x00060080U,
         0x00070080U,
         0x00080080U,
         0x00090080U,
         0x000A0080U,
         0x000B0080U,
         0x000C0080U,
         0x000D0080U,
         0x000E0080U,
         0x000F0080U,
         0x00100080U,
         0x00110080U,
         0x00120080U,
         0x00130080U,
         0x00140080U,
         0x00150080U,
         0x00160080U,
         0x00170080U,
         0x00180080U,
         0x00190080U,
         0x001A0080U,
         0x001B0080U,
         0x001C0080U,
         0x001D0080U,
         0x001E0080U,
         0x001F0080U,
         0x00200080U,
         0x00210080U,
         0x00220080U,
         0x00230080U,
         0x00240080U,
         0x00250080U,
         0x00260080U,
         0x00270080U,
         0x00280080U,
         0x00290080U,
         0x002A0080U,
         0x002B0080U,
         0x002C0080U,
         0x002D0080U,
         0x002E0080U,
         0x002F0080U,
         0x00300080U,
         0x00310080U,
         0x00320080U,
         0x00330080U,
         0x00340080U,
         0x00350080U,
         0x00360080U,
         0x00370080U,
         0x00380080U,
         0x00390080U,
         0x003A0080U,
         0x003B0080U,
         0x003C0080U,
         0x003D0080U,
         0x003E0080U,
         0x003F0080U,
         0x00400080U,
         0x00410080U,
         0x00420080U,
         0x00430080U,
         0x00440080U,
         0x00450080U,
         0x00460080U,
         0x00470080U,
         0x00480080U,
         0x00490080U,
         0x004A0080U,
         0x004B0080U,
         0x004C0080U,
         0x004D0080U,
         0x004E0080U,
         0x004F0080U,
         0x00500080U,
         0x00510080U,
         0x00520080U,
         0x00530080U,
         0x00540080U,
         0x00550080U,
         0x00560080U,
         0x00570080U,
         0x00580080U,
         0x00590080U,
         0x005A0080U,
         0x005B0080U,
         0x005C0080U,
         0x005D0080U,
         0x005E0080U,
         0x005F0080U,
         0x00600080U,
         0x00610080U,
         0x00620080U,
         0x00630080U,
         0x00640080U,
         0x00650080U,
         0x00660080U,
         0x00670080U,
         0x00680080U,
         0x00690080U,
         0x006A0080U,
         0x006B0080U,
         0x006C0080U,
         0x006D0080U,
         0x006E0080U,
         0x006F0080U,
         0x00700080U,
         0x00710080U,
         0x00720080U,
         0x00730080U,
         0x00740080U,
         0x00750080U,
         0x00760080U,
         0x00770080U,
         0x00780080U,
         0x00790080U,
         0x007A0080U,
         0x007B0080U,
         0x007C0080U,
         0x007D0080U,
         0x007E0080U,
         0x007F0080U,
         0x00800080U,
         0x00810080U,
         0x00820080U,
         0x00830080U,
         0x00840080U,
         0x00850080U,
         0x00860080U,
         0x00870080U,
         0x00880080U,
         0x00890080U,
         0x008A0080U,
         0x008B0080U,
         0x008C0080U,
         0x008D0080U,
         0x008E0080U,
         0x008F0080U,
         0x00900080U,
         0x00910080U,
         0x00920080U,
         0x00930080U,
         0x00940080U,
         0x00950080U,
         0x00960080U,
         0x00970080U,
         0x00980080U,
         0x00990080U,
         0x009A0080U,
         0x009B0080U,
         0x009C0080U,
         0x009D0080U,
         0x009E0080U,
         0x009F0080U,
         0x00A00080U,
         0x00A10080U,
         0x00A20080U,
         0x00A30080U,
         0x00A40080U,
         0x00A50080U,
         0x00A60080U,
         0x00A70080U,
         0x00A80080U,
         0x00A90080U,
         0x00AA0080U,
         0x00AB0080U,
         0x00AC0080U,
         0x00AD0080U,
         0x00AE0080U,
         0x00AF0080U,
         0x00B00080U,
         0x00B10080U,
         0x00B20080U,
         0x00B30080U,
         0x00B40080U,
         0x00B50080U,
         0x00B60080U,
         0x00B70080U,
         0x00B80080U,
         0x00B90080U,
         0x00BA0080U,
         0x00BB0080U,
         0x00BC0080U,
         0x00BD0080U,
         0x00BE0080U,
         0x00BF0080U,
         0x00C00080U,
         0x00C10080U,
         0x00C20080U,
         0x00C30080U,
         0x00C40080U,
         0x00C50080U,
         0x00C60080U,
         0x00C70080U,
         0x00C80080U,
         0x00C90080U,
         0x00CA0080U,
         0x00CB0080U,
         0x00CC0080U,
         0x00CD0080U,
         0x00CE0080U,
         0x00CF0080U,
         0x00D00080U,
         0x00D10080U,
         0x00D20080U,
         0x00D30080U,
         0x00D40080U,
         0x00D50080U,
         0x00D60080U,
         0x00D70080U,
         0x00D80080U,
         0x00D90080U,
         0x00DA0080U,
         0x00DB0080U,
         0x00DC0080U,
         0x00DD0080U,
         0x00DE0080U,
         0x00DF0080U,
         0x00E00080U,
         0x00E10080U,
         0x00E20080U,
         0x00E30080U,
         0x00E40080U,
         0x00E50080U,
         0x00E60080U,
         0x00E70080U,
         0x00E80080U,
         0x00E90080U,
         0x00EA0080U,
         0x00EB0080U,
         0x00EC0080U,
         0x00ED0080U,
         0x00EE0080U,
         0x00EF0080U,
         0x00F00080U,
         0x00F10080U,
         0x00F20080U,
         0x00F30080U,
         0x00F40080U,
         0x00F50080U,
         0x00F60080U,
         0x00F70080U,
         0x00F80080U,
         0x00F90080U,
         0x00FA0080U,
         0x00FB0080U,
         0x00FC0080U,
         0x00FD0080U,
         0x00FE0080U,
         0x00FF0080U,
         INFLATE_ENTRY_EXCEPTIONAL | INFLATE_ENTRY_END_OF_BLOCK,
         0x00030000U,
         0x00040000U,
         0x00050000U,
         0x00060000U,
         0x00070000U,
         0x00080000U,
         0x00090000U,
         0x000A0000U,
         0x000B0100U,
         0x000D0100U,
         0x000F0100U,
         0x00110100U,
         0x00130200U,
         0x00170200U,
         0x001B0200U,
         0x001F0200U,
         0x00230300U,
         0x002B0300U,
         0x00330300U,
         0x003B0300U,
         0x00430400U,
         0x00530400U,
         0x00630400U,
         0x00730400U,
         0x00830500U,
         0x00A30500U,
         0x00C30500U,
         0x00E30500U,
         0x01020000U,
         INFLATE_ENTRY_EXCEPTIONAL | INFLATE_ENTRY_INVALID_SYMBOL,
         INFLATE_ENTRY_EXCEPTIONAL | INFLATE_ENTRY_INVALID_SYMBOL}};
inline constexpr std::array<std::uint32_t, INFLATE_DISTANCE_SYMBOLS>
    INFLATE_DISTANCE_RESULTS{
        {0x00010000U,
         0x00020000U,
         0x00030000U,
         0x00040000U,
         0x00050100U,
         0x00070100U,
         0x00090200U,
         0x000D0200U,
         0x00110300U,
         0x00190300U,
         0x00210400U,
         0x00310400U,
         0x00410500U,
         0x00610500U,
         0x00810600U,
         0x00C10600U,
         0x01010700U,
         0x01810700U,
         0x02010800U,
         0x03010800U,
         0x04010900U,
         0x06010900U,
         0x08010A00U,
         0x0C010A00U,
         0x10010B00U,
         0x18010B00U,
         0x20010C00U,
         0x30010C00U,
         0x40010D00U,
         0x60010D00U,
         INFLATE_ENTRY_EXCEPTIONAL | INFLATE_ENTRY_INVALID_SYMBOL,
         INFLATE_ENTRY_EXCEPTIONAL | INFLATE_ENTRY_INVALID_SYMBOL}};
inline constexpr std::array<std::uint32_t, INFLATE_CODE_LENGTH_SYMBOLS>
    INFLATE_CODE_LENGTH_RESULTS{
        {0x00000000U, 0x00010000U, 0x00020000U, 0x00030000U, 0x00040000U,
         0x00050000U, 0x00060000U, 0x00070000U, 0x00080000U, 0x00090000U,
         0x000A0000U, 0x000B0000U, 0x000C0000U, 0x000D0000U, 0x000E0000U,
         0x000F0000U, 0x00100000U, 0x00110000U, 0x00120000U}};

inline auto inflate_reverse_bits(const std::uint32_t value,
                                 const unsigned int length) -> std::uint32_t {
  std::uint32_t result{0};
  for (unsigned int index = 0; index < length; ++index) {
    result |= ((value >> index) & 1U) << (length - 1 - index);
  }

  return result;
}

// The symbol results hold the number of extra bits in bits 8 to 12, which the
// table entry replaces with the codeword length, adding both to the bits to
// consume so that decoding takes the codeword and its extra bits in one shift
inline auto inflate_make_entry(const std::uint32_t result,
                               const unsigned int length) -> std::uint32_t {
  const std::uint32_t extra{(result >> 8) & 0x1fU};
  return (result & ~std::uint32_t{0x1f00U}) | (length << 8) | (length + extra);
}

// Builds a decode table from a sequence of code lengths as per RFC 1951
// section 3.2.2, returning a null pointer on success or the reason the code
// is invalid. Codewords are read starting from their most significant bit
// while the input is packed starting from the least significant bit, so every
// index is the bit reversal of the codeword
inline auto inflate_build_table(const std::uint8_t *lengths,
                                const std::size_t symbol_count,
                                const std::uint32_t *results,
                                const unsigned int table_bits,
                                std::uint32_t *table) -> const char * {
  std::array<std::uint16_t, INFLATE_MAXIMUM_CODEWORD_LENGTH + 1> counts{};
  for (std::size_t symbol = 0; symbol < symbol_count; ++symbol) {
    counts[lengths[symbol]] += 1;
  }

  const std::size_t used_symbols{symbol_count - counts[0]};
  int remaining_codespace{1};
  for (unsigned int length = 1; length <= INFLATE_MAXIMUM_CODEWORD_LENGTH;
       ++length) {
    remaining_codespace <<= 1;
    remaining_codespace -= counts[length];
    if (remaining_codespace < 0) {
      return "Over-subscribed Huffman code";
    }
  }

  const std::size_t primary_size{std::size_t{1} << table_bits};
  if (remaining_codespace > 0) {
    // RFC 1951 section 3.2.7 only sanctions an incomplete code that is empty
    // or that holds a single codeword of one bit
    if (used_symbols > 1 || (used_symbols == 1 && counts[1] != 1)) {
      return "Incomplete Huffman code";
    }

    const std::uint32_t invalid{INFLATE_ENTRY_EXCEPTIONAL |
                                INFLATE_ENTRY_INVALID_CODE |
                                INFLATE_MAXIMUM_CODEWORD_LENGTH};
    for (std::size_t index = 0; index < primary_size; ++index) {
      table[index] = invalid;
    }
  }

  std::array<std::uint16_t, INFLATE_MAXIMUM_CODEWORD_LENGTH + 2> offsets{};
  for (unsigned int length = 1; length <= INFLATE_MAXIMUM_CODEWORD_LENGTH;
       ++length) {
    offsets[length + 1] =
        static_cast<std::uint16_t>(offsets[length] + counts[length]);
  }

  std::array<std::uint16_t, INFLATE_LITERAL_LENGTH_SYMBOLS> sorted{};
  for (std::size_t symbol = 0; symbol < symbol_count; ++symbol) {
    if (lengths[symbol] != 0) {
      sorted[offsets[lengths[symbol]]] = static_cast<std::uint16_t>(symbol);
      offsets[lengths[symbol]] += 1;
    }
  }

  // An allowed incomplete code holds at most one codeword, of one bit, whose
  // reversal is zero
  if (remaining_codespace > 0) {
    if (used_symbols == 1) {
      const std::uint32_t entry{inflate_make_entry(results[sorted[0]], 1)};
      for (std::size_t slot = 0; slot < primary_size; slot += 2) {
        table[slot] = entry;
      }
    }

    return nullptr;
  }

  // Walks the codewords in canonical order while keeping them bit reversed,
  // where incrementing a reversed codeword flips its highest zero bit and
  // clears every bit above it. The primary table starts as wide as the
  // shortest codeword and doubles in size with every length, so that every
  // shorter codeword is replicated with a single copy
  unsigned int length{1};
  while (counts[length] == 0) {
    length += 1;
  }

  std::uint32_t codeword{0};
  std::size_t position{0};
  std::size_t table_end{std::size_t{1} << length};
  unsigned int count{counts[length]};
  while (length <= table_bits) {
    while (count > 0) {
      table[codeword] = inflate_make_entry(results[sorted[position]], length);
      position += 1;
      count -= 1;
      if (codeword == table_end - 1) {
        while (table_end < primary_size) {
          std::memcpy(table + table_end, table,
                      table_end * sizeof(std::uint32_t));
          table_end <<= 1;
        }

        return nullptr;
      }

      const auto flipped{codeword ^ static_cast<std::uint32_t>(table_end - 1)};
      const std::uint32_t bit{std::uint32_t{1}
                              << (31 - std::countl_zero(flipped))};
      codeword = (codeword & (bit - 1)) | bit;
    }

    while (count == 0) {
      length += 1;
      if (length <= table_bits) {
        std::memcpy(table + table_end, table,
                    table_end * sizeof(std::uint32_t));
        table_end <<= 1;
      }

      count = counts[length];
    }
  }

  // Longer codewords sharing the same first bits share a subtable, and
  // canonical codewords sharing those bits are contiguous
  const unsigned int subtable_bits{INFLATE_MAXIMUM_CODEWORD_LENGTH -
                                   table_bits};
  const std::size_t subtable_size{std::size_t{1} << subtable_bits};
  std::size_t next_subtable{primary_size};
  std::uint32_t current_prefix{0xffffffffU};
  std::size_t current_subtable{0};
  while (true) {
    const std::uint32_t prefix{codeword &
                               static_cast<std::uint32_t>(primary_size - 1)};
    if (prefix != current_prefix) {
      current_prefix = prefix;
      current_subtable = next_subtable;
      next_subtable += subtable_size;
      table[prefix] = INFLATE_ENTRY_EXCEPTIONAL | INFLATE_ENTRY_SUBTABLE |
                      (static_cast<std::uint32_t>(current_subtable) << 16) |
                      table_bits;
    }

    const unsigned int remaining_length{length - table_bits};
    const std::uint32_t entry{
        inflate_make_entry(results[sorted[position]], remaining_length)};
    position += 1;
    for (std::size_t slot = codeword >> table_bits; slot < subtable_size;
         slot += std::size_t{1} << remaining_length) {
      table[current_subtable + slot] = entry;
    }

    const std::uint32_t last{(std::uint32_t{1} << length) - 1};
    if (codeword == last) {
      return nullptr;
    }

    const auto flipped{codeword ^ last};
    const std::uint32_t bit{std::uint32_t{1}
                            << (31 - std::countl_zero(flipped))};
    codeword = (codeword & (bit - 1)) | bit;
    count -= 1;
    while (count == 0) {
      length += 1;
      count = counts[length];
    }
  }
}

enum class InflateStatus : std::uint8_t { Done, NeedInput, OutputFull };

// The output buffer holds the bytes that back-references may point to before
// the next output position, and the input buffer holds the compressed bytes
// not yet read. Final input means no more compressed bytes will ever follow
struct InflateBuffers {
  const std::uint8_t *input_next;
  const std::uint8_t *input_end;
  bool input_final;
  std::uint8_t *output_begin;
  std::uint8_t *output_next;
  std::uint8_t *output_end;
};

// Decodes a sequence of gzip members (RFC 1952) holding deflate data
// (RFC 1951). Every call makes as much progress as the buffers allow and
// returns when the input is exhausted, the output is full, or the data ended
class InflateDecoder {
public:
  // A dynamic block header never exceeds this many bytes, so holding at least
  // this much unread input before a block header avoids suspending inside it
  static constexpr std::size_t INPUT_MARGIN{1024};

  auto decode(InflateBuffers &buffers) -> InflateStatus {
    this->checksum_position_ = buffers.output_next;
    this->history_start_ =
        buffers.output_next -
        std::min(this->member_history_,
                 static_cast<std::size_t>(buffers.output_next -
                                          buffers.output_begin));

    while (true) {
      switch (this->state_) {
        case State::MemberStart:
          if (!this->start_member(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::HeaderFixed:
          if (!this->read_header_fixed(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::HeaderExtraLength:
          if (!this->read_header_extra_length(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::HeaderExtra:
          if (!this->skip_header_extra(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::HeaderName:
        case State::HeaderComment:
          if (!this->skip_header_string(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::HeaderChecksum:
          if (!this->read_header_checksum(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::BlockHeader:
          if (!this->read_block_header(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::StoredLength:
          if (!this->read_stored_length(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::StoredData: {
          const auto progress{this->copy_stored_data(buffers)};
          if (progress != InflateStatus::Done) {
            return this->leave(buffers, progress);
          }

          break;
        }
        case State::Huffman: {
          const auto progress{this->decode_huffman(buffers)};
          if (progress != InflateStatus::Done) {
            return this->leave(buffers, progress);
          }

          break;
        }
        case State::Trailer:
          if (!this->read_trailer(buffers)) {
            return this->leave(buffers, InflateStatus::NeedInput);
          }

          break;
        case State::Done:
          return this->leave(buffers, InflateStatus::Done);
      }
    }
  }

  // Whole bytes already moved from the input into the bit buffer but not yet
  // consumed. A caller that compacts its input buffer must keep them in place
  // before the next input position
  [[nodiscard]] auto buffered_input() const -> std::size_t {
    return this->bits_available_ / 8;
  }

private:
  enum class State : std::uint8_t {
    MemberStart,
    HeaderFixed,
    HeaderExtraLength,
    HeaderExtra,
    HeaderName,
    HeaderComment,
    HeaderChecksum,
    BlockHeader,
    StoredLength,
    StoredData,
    Huffman,
    Trailer,
    Done
  };

  static constexpr std::uint8_t FLAG_HEADER_CHECKSUM{0x02};
  static constexpr std::uint8_t FLAG_EXTRA{0x04};
  static constexpr std::uint8_t FLAG_NAME{0x08};
  static constexpr std::uint8_t FLAG_COMMENT{0x10};
  static constexpr std::uint8_t FLAG_RESERVED{0xe0};

  // Enough input for two word refills between the checks, and enough output
  // for the longest match plus the bytes that chunked copies write past it
  static constexpr std::size_t FAST_INPUT_MARGIN{32};
  static constexpr std::size_t FAST_OUTPUT_MARGIN{258 + 32};

  // The smallest multiple of every distance below eight that is at least
  // eight, so overlapping matches can continue with whole word copies
  static constexpr std::array<std::size_t, 8> SHORT_DISTANCE_STEP{
      {0, 8, 8, 9, 8, 10, 12, 14}};

  static auto load_word(const std::uint8_t *data) -> std::uint64_t {
    std::uint64_t word{0};
    std::memcpy(&word, data, sizeof(word));
    if constexpr (std::endian::native == std::endian::big) {
      word = std::byteswap(word);
    }

    return word;
  }

  static auto low_bits(const std::uint64_t value, const unsigned int count)
      -> std::uint32_t {
    return static_cast<std::uint32_t>(value &
                                      ((std::uint64_t{1} << count) - 1));
  }

  static auto available_input(const InflateBuffers &buffers) -> std::size_t {
    return static_cast<std::size_t>(buffers.input_end - buffers.input_next);
  }

  static auto available_output(const InflateBuffers &buffers) -> std::size_t {
    return static_cast<std::size_t>(buffers.output_end - buffers.output_next);
  }

  auto leave(InflateBuffers &buffers, const InflateStatus status)
      -> InflateStatus {
    this->flush_member_progress(buffers);
    return status;
  }

  auto flush_member_progress(const InflateBuffers &buffers) -> void {
    const auto produced{static_cast<std::size_t>(buffers.output_next -
                                                 this->checksum_position_)};
    if (produced > 0) {
      this->member_checksum_ =
          crc32_update(this->member_checksum_,
                       std::string_view{reinterpret_cast<const char *>(
                                            this->checksum_position_),
                                        produced});
      this->member_size_ += static_cast<std::uint32_t>(produced);
      this->member_history_ += produced;
      this->checksum_position_ = buffers.output_next;
    }
  }

  auto update_header_checksum(const std::uint8_t *data, const std::size_t size)
      -> void {
    if ((this->flags_ & FLAG_HEADER_CHECKSUM) != 0) {
      this->header_checksum_ = crc32_update(
          this->header_checksum_,
          std::string_view{reinterpret_cast<const char *>(data), size});
    }
  }

  [[noreturn]] static auto unexpected_end() -> void {
    throw GZIPError{"Unexpected end of source stream"};
  }

  [[nodiscard]] auto real_bits() const -> long long {
    return static_cast<long long>(this->bits_available_) -
           (static_cast<long long>(this->overread_) * 8);
  }

  // Reports the input as truncated when the bits that led to the error were
  // not all real input, as the error may only be an artifact of the padding
  [[noreturn]] auto fail(const char *message, const long long bits_needed) const
      -> void {
    if (this->real_bits() < bits_needed) {
      unexpected_end();
    }

    throw GZIPError{message};
  }

  auto check_not_overread() const -> void {
    if (this->real_bits() < 0) {
      unexpected_end();
    }
  }

  auto consume(const unsigned int count) -> void {
    this->bit_buffer_ >>= count;
    this->bits_available_ -= count;
  }

  // Past the end of the final input the bit buffer is padded with zero bytes,
  // which are only an error if they end up consumed. Stopping short of 56
  // bits keeps at most 63 bits buffered, which the word refill relies on
  auto refill_careful(InflateBuffers &buffers) -> void {
    while (this->bits_available_ < 56) {
      if (buffers.input_next < buffers.input_end) {
        this->bit_buffer_ |= static_cast<std::uint64_t>(*buffers.input_next)
                             << this->bits_available_;
        buffers.input_next += 1;
      } else if (buffers.input_final) {
        this->overread_ += 1;
      } else {
        return;
      }

      this->bits_available_ += 8;
    }
  }

  auto read_bits(InflateBuffers &buffers, const unsigned int count)
      -> std::uint32_t {
    if (this->bits_available_ < count) {
      this->refill_careful(buffers);
    }

    const auto value{low_bits(this->bit_buffer_, count)};
    this->consume(count);
    return value;
  }

  // Discards the bits up to the next byte boundary and hands the whole bytes
  // still in the bit buffer back to the input
  auto align_to_byte(InflateBuffers &buffers) -> void {
    const auto whole_bytes{this->bits_available_ / 8};
    if (whole_bytes < this->overread_) {
      unexpected_end();
    }

    buffers.input_next -= whole_bytes - this->overread_;
    this->bit_buffer_ = 0;
    this->bits_available_ = 0;
    this->overread_ = 0;
  }

  [[nodiscard]] auto header_state_after(const State completed) const -> State {
    if (completed == State::HeaderFixed && (this->flags_ & FLAG_EXTRA) != 0) {
      return State::HeaderExtraLength;
    }

    if ((completed == State::HeaderFixed || completed == State::HeaderExtra) &&
        (this->flags_ & FLAG_NAME) != 0) {
      return State::HeaderName;
    }

    if (completed != State::HeaderComment &&
        (this->flags_ & FLAG_COMMENT) != 0) {
      return State::HeaderComment;
    }

    if ((this->flags_ & FLAG_HEADER_CHECKSUM) != 0) {
      return State::HeaderChecksum;
    }

    return State::BlockHeader;
  }

  auto start_member(InflateBuffers &buffers) -> bool {
    const auto available{available_input(buffers)};
    if (this->first_member_) {
      if (available == 0) {
        if (buffers.input_final) {
          throw GZIPError{"Empty source stream"};
        }

        return false;
      }

      if (buffers.input_next[0] != 0x1f) {
        throw GZIPError{"Invalid gzip magic bytes"};
      }

      if (available < 2) {
        if (buffers.input_final) {
          unexpected_end();
        }

        return false;
      }

      if (buffers.input_next[1] != 0x8b) {
        throw GZIPError{"Invalid gzip magic bytes"};
      }
    } else {
      // Like gzip(1), data after a member that does not start with the
      // identification bytes is ignored as trailing garbage
      if (available < 2) {
        if (buffers.input_final) {
          this->state_ = State::Done;
          return true;
        }

        return false;
      }

      if (buffers.input_next[0] != 0x1f || buffers.input_next[1] != 0x8b) {
        this->state_ = State::Done;
        return true;
      }
    }

    this->first_member_ = false;
    this->flags_ = FLAG_HEADER_CHECKSUM;
    this->header_checksum_ = 0;
    this->update_header_checksum(buffers.input_next, 2);
    buffers.input_next += 2;
    this->member_checksum_ = 0;
    this->member_size_ = 0;
    this->member_history_ = 0;
    this->checksum_position_ = buffers.output_next;
    this->history_start_ = buffers.output_next;
    this->state_ = State::HeaderFixed;
    return true;
  }

  auto read_header_fixed(InflateBuffers &buffers) -> bool {
    const auto available{available_input(buffers)};
    if (available >= 1 && buffers.input_next[0] != 8) {
      throw GZIPError{"Unsupported gzip compression method"};
    }

    if (available >= 2 && (buffers.input_next[1] & FLAG_RESERVED) != 0) {
      throw GZIPError{"Reserved gzip FLG bits must be zero"};
    }

    // The compression method, flags, modification time, extra flags, and
    // operating system
    if (available < 8) {
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    const std::uint8_t checksum_flag{FLAG_HEADER_CHECKSUM};
    const bool track{(buffers.input_next[1] & checksum_flag) != 0};
    this->flags_ = track ? checksum_flag : std::uint8_t{0};
    this->update_header_checksum(buffers.input_next, 8);
    if (!track) {
      this->header_checksum_ = 0;
    }

    this->flags_ = buffers.input_next[1];
    buffers.input_next += 8;
    this->state_ = this->header_state_after(State::HeaderFixed);
    return true;
  }

  auto read_header_extra_length(InflateBuffers &buffers) -> bool {
    if (available_input(buffers) < 2) {
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    this->skip_remaining_ =
        static_cast<std::size_t>(buffers.input_next[0]) |
        (static_cast<std::size_t>(buffers.input_next[1]) << 8);
    this->update_header_checksum(buffers.input_next, 2);
    buffers.input_next += 2;
    this->state_ = State::HeaderExtra;
    return true;
  }

  auto skip_header_extra(InflateBuffers &buffers) -> bool {
    const auto amount{
        std::min(this->skip_remaining_, available_input(buffers))};
    this->update_header_checksum(buffers.input_next, amount);
    buffers.input_next += amount;
    this->skip_remaining_ -= amount;
    if (this->skip_remaining_ > 0) {
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    this->state_ = this->header_state_after(State::HeaderExtra);
    return true;
  }

  // The file name and the comment are zero-terminated
  auto skip_header_string(InflateBuffers &buffers) -> bool {
    const auto available{available_input(buffers)};
    const auto *terminator{static_cast<const std::uint8_t *>(
        std::memchr(buffers.input_next, 0, available))};
    if (terminator == nullptr) {
      this->update_header_checksum(buffers.input_next, available);
      buffers.input_next += available;
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    const auto amount{
        static_cast<std::size_t>(terminator - buffers.input_next) + 1};
    this->update_header_checksum(buffers.input_next, amount);
    buffers.input_next += amount;
    this->state_ = this->header_state_after(this->state_);
    return true;
  }

  auto read_header_checksum(InflateBuffers &buffers) -> bool {
    if (available_input(buffers) < 2) {
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    // RFC 1952 section 2.3.1: the two least significant bytes of the CRC-32
    // of every header byte before this field
    const auto stored{static_cast<std::uint32_t>(buffers.input_next[0]) |
                      (static_cast<std::uint32_t>(buffers.input_next[1]) << 8)};
    if (stored != (this->header_checksum_ & 0xffffU)) {
      throw GZIPError{"FHCRC mismatch"};
    }

    buffers.input_next += 2;
    this->state_ = State::BlockHeader;
    return true;
  }

  auto end_block(InflateBuffers &buffers) -> void {
    // Checksumming the output of every block as soon as it ends reads it
    // while it is still in cache
    this->flush_member_progress(buffers);
    if (this->final_block_) {
      this->align_to_byte(buffers);
      this->final_block_ = false;
      this->state_ = State::Trailer;
    } else {
      this->state_ = State::BlockHeader;
    }
  }

  auto read_block_header(InflateBuffers &buffers) -> bool {
    if (!buffers.input_final && available_input(buffers) < INPUT_MARGIN) {
      return false;
    }

    this->final_block_ = this->read_bits(buffers, 1) != 0;
    const auto type{this->read_bits(buffers, 2)};
    this->check_not_overread();
    switch (type) {
      case 0:
        this->align_to_byte(buffers);
        this->state_ = State::StoredLength;
        return true;
      case 1:
        this->load_fixed_tables();
        this->state_ = State::Huffman;
        return true;
      case 2:
        this->read_dynamic_header(buffers);
        this->state_ = State::Huffman;
        return true;
      default:
        throw GZIPError{"Reserved deflate block type"};
    }
  }

  auto read_stored_length(InflateBuffers &buffers) -> bool {
    if (available_input(buffers) < 4) {
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    const auto length{static_cast<std::uint32_t>(buffers.input_next[0]) |
                      (static_cast<std::uint32_t>(buffers.input_next[1]) << 8)};
    const auto complement{
        static_cast<std::uint32_t>(buffers.input_next[2]) |
        (static_cast<std::uint32_t>(buffers.input_next[3]) << 8)};
    if (length != (~complement & 0xffffU)) {
      throw GZIPError{"Stored block LEN/NLEN mismatch"};
    }

    buffers.input_next += 4;
    this->stored_remaining_ = length;
    this->state_ = State::StoredData;
    return true;
  }

  auto copy_stored_data(InflateBuffers &buffers) -> InflateStatus {
    while (this->stored_remaining_ > 0) {
      if (available_output(buffers) == 0) {
        return InflateStatus::OutputFull;
      }

      const auto input{available_input(buffers)};
      if (input == 0) {
        if (buffers.input_final) {
          unexpected_end();
        }

        return InflateStatus::NeedInput;
      }

      const auto amount{std::min(
          {this->stored_remaining_, input, available_output(buffers)})};
      std::memcpy(buffers.output_next, buffers.input_next, amount);
      buffers.input_next += amount;
      buffers.output_next += amount;
      this->stored_remaining_ -= amount;
    }

    this->end_block(buffers);
    return InflateStatus::Done;
  }

  auto read_trailer(InflateBuffers &buffers) -> bool {
    if (available_input(buffers) < 8) {
      if (buffers.input_final) {
        unexpected_end();
      }

      return false;
    }

    this->flush_member_progress(buffers);
    const auto *data{buffers.input_next};
    const auto checksum{static_cast<std::uint32_t>(data[0]) |
                        (static_cast<std::uint32_t>(data[1]) << 8) |
                        (static_cast<std::uint32_t>(data[2]) << 16) |
                        (static_cast<std::uint32_t>(data[3]) << 24)};
    const auto size{static_cast<std::uint32_t>(data[4]) |
                    (static_cast<std::uint32_t>(data[5]) << 8) |
                    (static_cast<std::uint32_t>(data[6]) << 16) |
                    (static_cast<std::uint32_t>(data[7]) << 24)};
    if (checksum != this->member_checksum_) {
      throw GZIPError{"Gzip member CRC32 mismatch"};
    }

    if (size != this->member_size_) {
      throw GZIPError{"Gzip member ISIZE mismatch"};
    }

    buffers.input_next += 8;
    this->state_ = State::MemberStart;
    return true;
  }

  auto load_fixed_tables() -> void {
    if (this->fixed_tables_) {
      return;
    }

    // RFC 1951 section 3.2.6
    std::array<std::uint8_t, INFLATE_LITERAL_LENGTH_SYMBOLS> literal_lengths{};
    std::memset(literal_lengths.data(), 8, 144);
    std::memset(literal_lengths.data() + 144, 9, 112);
    std::memset(literal_lengths.data() + 256, 7, 24);
    std::memset(literal_lengths.data() + 280, 8, 8);
    inflate_build_table(literal_lengths.data(), literal_lengths.size(),
                        INFLATE_LITERAL_LENGTH_RESULTS.data(),
                        INFLATE_LITERAL_LENGTH_TABLE_BITS,
                        this->literal_length_table_.data());
    std::array<std::uint8_t, INFLATE_DISTANCE_SYMBOLS> distance_lengths{};
    std::memset(distance_lengths.data(), 5, distance_lengths.size());
    inflate_build_table(distance_lengths.data(), distance_lengths.size(),
                        INFLATE_DISTANCE_RESULTS.data(),
                        INFLATE_DISTANCE_TABLE_BITS,
                        this->distance_table_.data());
    this->fixed_tables_ = true;
  }

  auto read_dynamic_header(InflateBuffers &buffers) -> void {
    const auto literal_length_count{this->read_bits(buffers, 5) + 257};
    const auto distance_count{this->read_bits(buffers, 5) + 1};
    const auto code_length_count{this->read_bits(buffers, 4) + 4};
    if (literal_length_count > 286) {
      this->fail("Too many literal/length codes", 0);
    }

    std::array<std::uint8_t, INFLATE_CODE_LENGTH_SYMBOLS> code_length_lengths{};
    for (std::size_t index = 0; index < code_length_count; ++index) {
      code_length_lengths[INFLATE_CODE_LENGTH_ORDER[index]] =
          static_cast<std::uint8_t>(this->read_bits(buffers, 3));
    }

    const auto *code_length_error{inflate_build_table(
        code_length_lengths.data(), code_length_lengths.size(),
        INFLATE_CODE_LENGTH_RESULTS.data(), INFLATE_CODE_LENGTH_TABLE_BITS,
        this->code_length_table_.data())};
    if (code_length_error != nullptr) {
      this->fail(code_length_error, 0);
    }

    std::array<std::uint8_t,
               INFLATE_LITERAL_LENGTH_SYMBOLS + INFLATE_DISTANCE_SYMBOLS>
        lengths{};
    const std::size_t total{literal_length_count + distance_count};
    std::size_t index{0};
    while (index < total) {
      if (this->bits_available_ < 14) {
        this->refill_careful(buffers);
      }

      const auto entry{this->code_length_table_[low_bits(
          this->bit_buffer_, INFLATE_CODE_LENGTH_TABLE_BITS)]};
      if ((entry & INFLATE_ENTRY_EXCEPTIONAL) != 0) {
        this->fail("Invalid Huffman code", INFLATE_CODE_LENGTH_TABLE_BITS);
      }

      this->consume(entry & 0x1fU);
      const auto symbol{entry >> 16};
      if (symbol < 16) {
        lengths[index] = static_cast<std::uint8_t>(symbol);
        index += 1;
        continue;
      }

      std::size_t repeats{0};
      std::uint8_t value{0};
      if (symbol == 16) {
        if (index == 0) {
          this->fail("Repeat-previous code length with no previous", 0);
        }

        value = lengths[index - 1];
        repeats = this->read_bits(buffers, 2) + 3;
      } else if (symbol == 17) {
        repeats = this->read_bits(buffers, 3) + 3;
      } else {
        repeats = this->read_bits(buffers, 7) + 11;
      }

      if (index + repeats > total) {
        this->fail("Code length count overflow", 0);
      }

      std::memset(lengths.data() + index, value, repeats);
      index += repeats;
    }

    this->check_not_overread();
    const auto *literal_length_error{inflate_build_table(
        lengths.data(), literal_length_count,
        INFLATE_LITERAL_LENGTH_RESULTS.data(),
        INFLATE_LITERAL_LENGTH_TABLE_BITS, this->literal_length_table_.data())};
    if (literal_length_error != nullptr) {
      throw GZIPError{literal_length_error};
    }

    const auto *distance_error{inflate_build_table(
        lengths.data() + literal_length_count, distance_count,
        INFLATE_DISTANCE_RESULTS.data(), INFLATE_DISTANCE_TABLE_BITS,
        this->distance_table_.data())};
    if (distance_error != nullptr) {
      throw GZIPError{distance_error};
    }

    this->fixed_tables_ = false;
  }

  // Copies the match byte by byte, which is always correct for overlapping
  // ranges, stopping when the output fills up
  auto copy_match_careful(InflateBuffers &buffers) -> InflateStatus {
    const auto amount{
        std::min(this->pending_length_, available_output(buffers))};
    const std::uint8_t *source{buffers.output_next - this->pending_distance_};
    for (std::size_t index = 0; index < amount; ++index) {
      buffers.output_next[index] = source[index];
    }

    buffers.output_next += amount;
    this->pending_length_ -= amount;
    return this->pending_length_ > 0 ? InflateStatus::OutputFull
                                     : InflateStatus::Done;
  }

  auto decode_huffman(InflateBuffers &buffers) -> InflateStatus {
    if (this->pending_length_ > 0 &&
        this->copy_match_careful(buffers) != InflateStatus::Done) {
      return InflateStatus::OutputFull;
    }

    if (this->decode_huffman_fast(buffers)) {
      return InflateStatus::Done;
    }

    return this->decode_huffman_careful(buffers);
  }

  // Returns whether the block ended
  auto decode_huffman_fast(InflateBuffers &buffers) -> bool {
    const auto *literal_length_table{this->literal_length_table_.data()};
    const auto *distance_table{this->distance_table_.data()};
    const auto *input_next{buffers.input_next};
    const auto *const input_limit{available_input(buffers) >= FAST_INPUT_MARGIN
                                      ? buffers.input_end - FAST_INPUT_MARGIN
                                      : buffers.input_next};
    auto *output_next{buffers.output_next};
    auto *const output_limit{available_output(buffers) >= FAST_OUTPUT_MARGIN
                                 ? buffers.output_end - FAST_OUTPUT_MARGIN
                                 : buffers.output_next};
    const auto *const history_start{this->history_start_};
    auto bit_buffer{this->bit_buffer_};
    auto bits_available{this->bits_available_};
    // Only the lowest six bits of the bit count are meaningful, which lets
    // literals subtract their whole table entry instead of masking it first
    bool block_ended{false};

    // Every iteration starts with at least 56 buffered bits, enough for
    // three literals from the primary table or for a whole match
    if (input_next < input_limit && output_next < output_limit) {
      bit_buffer |= load_word(input_next) << (bits_available & 63U);
      input_next += (63U - (bits_available & 63U)) >> 3;
      bits_available |= 56;
      auto entry{literal_length_table[low_bits(
          bit_buffer, INFLATE_LITERAL_LENGTH_TABLE_BITS)]};
      while (true) {
        if ((entry & INFLATE_ENTRY_LITERAL) != 0) {
          // Every literal leaves the entry of the next symbol loaded, and a
          // refill only appends bits past the ones that entry was read from
          bit_buffer >>= entry & 0x3fU;
          bits_available -= entry;
          *output_next++ = static_cast<std::uint8_t>(entry >> 16);
          entry = literal_length_table[low_bits(
              bit_buffer, INFLATE_LITERAL_LENGTH_TABLE_BITS)];
          if ((entry & INFLATE_ENTRY_LITERAL) != 0) {
            bit_buffer >>= entry & 0x3fU;
            bits_available -= entry;
            *output_next++ = static_cast<std::uint8_t>(entry >> 16);
            entry = literal_length_table[low_bits(
                bit_buffer, INFLATE_LITERAL_LENGTH_TABLE_BITS)];
            if ((entry & INFLATE_ENTRY_LITERAL) != 0) {
              bit_buffer >>= entry & 0x3fU;
              bits_available -= entry;
              *output_next++ = static_cast<std::uint8_t>(entry >> 16);
              entry = literal_length_table[low_bits(
                  bit_buffer, INFLATE_LITERAL_LENGTH_TABLE_BITS)];
            }
          }

          if (input_next >= input_limit || output_next >= output_limit) {
            break;
          }

          bit_buffer |= load_word(input_next) << (bits_available & 63U);
          input_next += (63U - (bits_available & 63U)) >> 3;
          bits_available |= 56;
          continue;
        }

        if ((entry & INFLATE_ENTRY_SUBTABLE) != 0) {
          bit_buffer >>= INFLATE_LITERAL_LENGTH_TABLE_BITS;
          bits_available -= INFLATE_LITERAL_LENGTH_TABLE_BITS;
          entry = literal_length_table
              [(entry >> 16) +
               low_bits(bit_buffer, INFLATE_MAXIMUM_CODEWORD_LENGTH -
                                        INFLATE_LITERAL_LENGTH_TABLE_BITS)];
        }

        if ((entry & INFLATE_ENTRY_LITERAL) != 0) {
          bit_buffer >>= entry & 0x3fU;
          bits_available -= entry;
          *output_next++ = static_cast<std::uint8_t>(entry >> 16);
        } else if ((entry & INFLATE_ENTRY_EXCEPTIONAL) != 0) {
          if ((entry & INFLATE_ENTRY_END_OF_BLOCK) != 0) {
            bit_buffer >>= entry & 0x1fU;
            bits_available -= entry & 0x1fU;
            block_ended = true;
            break;
          }

          if ((entry & INFLATE_ENTRY_INVALID_CODE) != 0) {
            throw GZIPError{"Invalid Huffman code"};
          }

          throw GZIPError{"Invalid literal/length code"};
        } else {
          const auto buffer_before_length{bit_buffer};
          bit_buffer >>= entry & 0x3fU;
          bits_available -= entry;
          const std::size_t length{
              (entry >> 16) + (low_bits(buffer_before_length, entry & 0x1fU) >>
                               ((entry >> 8) & 0x1fU))};

          auto distance_entry{distance_table[low_bits(
              bit_buffer, INFLATE_DISTANCE_TABLE_BITS)]};
          if ((distance_entry & INFLATE_ENTRY_EXCEPTIONAL) != 0) {
            if ((distance_entry & INFLATE_ENTRY_SUBTABLE) != 0) {
              bit_buffer >>= INFLATE_DISTANCE_TABLE_BITS;
              bits_available -= INFLATE_DISTANCE_TABLE_BITS;
              distance_entry =
                  distance_table[(distance_entry >> 16) +
                                 low_bits(bit_buffer,
                                          INFLATE_MAXIMUM_CODEWORD_LENGTH -
                                              INFLATE_DISTANCE_TABLE_BITS)];
            }

            if ((distance_entry & INFLATE_ENTRY_INVALID_CODE) != 0) {
              throw GZIPError{"Invalid Huffman code"};
            }

            if ((distance_entry & INFLATE_ENTRY_INVALID_SYMBOL) != 0) {
              throw GZIPError{"Invalid distance code"};
            }
          }

          const auto buffer_before_distance{bit_buffer};
          bit_buffer >>= distance_entry & 0x3fU;
          bits_available -= distance_entry;
          const std::size_t distance{
              (distance_entry >> 16) +
              (low_bits(buffer_before_distance, distance_entry & 0x1fU) >>
               ((distance_entry >> 8) & 0x1fU))};

          if (std::cmp_greater(distance, output_next - history_start)) {
            throw GZIPError{"Backref distance exceeds bytes available"};
          }

          // A match leaves too few bits for the next lookup, so refill and
          // load the next entry before copying, which keeps a mispredicted
          // copy loop from holding back the decoding that follows
          bit_buffer |= load_word(input_next) << (bits_available & 63U);
          input_next += (63U - (bits_available & 63U)) >> 3;
          bits_available |= 56;
          entry = literal_length_table[low_bits(
              bit_buffer, INFLATE_LITERAL_LENGTH_TABLE_BITS)];

          // Whole chunks may run past the end of the match, which the
          // output margin leaves room for. Most matches fit in the chunks
          // copied before the loop, so its branch is rarely taken
          auto *destination{output_next};
          const auto *source{output_next - distance};
          output_next += length;
          if (distance >= 16) {
            std::memcpy(destination, source, 16);
            std::memcpy(destination + 16, source + 16, 16);
            std::memcpy(destination + 32, source + 32, 16);
            destination += 48;
            source += 48;
            while (destination < output_next) {
              std::memcpy(destination, source, 16);
              destination += 16;
              source += 16;
            }
          } else if (distance >= 8) {
            std::memcpy(destination, source, 8);
            std::memcpy(destination + 8, source + 8, 8);
            std::memcpy(destination + 16, source + 16, 8);
            std::memcpy(destination + 24, source + 24, 8);
            std::memcpy(destination + 32, source + 32, 8);
            destination += 40;
            source += 40;
            while (destination < output_next) {
              std::memcpy(destination, source, 8);
              destination += 8;
              source += 8;
            }
          } else if (distance == 1) {
            const std::uint64_t pattern{0x0101010101010101ULL * *source};
            std::memcpy(destination, &pattern, 8);
            std::memcpy(destination + 8, &pattern, 8);
            std::memcpy(destination + 16, &pattern, 8);
            std::memcpy(destination + 24, &pattern, 8);
            destination += 32;
            while (destination < output_next) {
              std::memcpy(destination, &pattern, 8);
              destination += 8;
            }
          } else {
            // Once one whole step of the repeating pattern is written,
            // the rest is a word copy from that far back
            const auto step{SHORT_DISTANCE_STEP[distance]};
            const auto *const expanded{destination + step};
            while (destination < expanded) {
              *destination++ = *source++;
            }

            while (destination < output_next) {
              std::memcpy(destination, destination - step, 8);
              destination += 8;
            }
          }

          if (input_next >= input_limit || output_next >= output_limit) {
            break;
          }

          continue;
        }

        if (input_next >= input_limit || output_next >= output_limit) {
          break;
        }

        bit_buffer |= load_word(input_next) << (bits_available & 63U);
        input_next += (63U - (bits_available & 63U)) >> 3;
        bits_available |= 56;
        entry = literal_length_table[low_bits(
            bit_buffer, INFLATE_LITERAL_LENGTH_TABLE_BITS)];
      }
    }

    buffers.input_next = input_next;
    buffers.output_next = output_next;
    this->bit_buffer_ = bit_buffer;
    this->bits_available_ = bits_available & 63U;
    if (block_ended) {
      this->end_block(buffers);
    }

    return block_ended;
  }

  auto decode_huffman_careful(InflateBuffers &buffers) -> InflateStatus {
    while (true) {
      if (!buffers.input_final && available_input(buffers) < 8) {
        return InflateStatus::NeedInput;
      }

      this->refill_careful(buffers);
      auto entry{this->literal_length_table_[low_bits(
          this->bit_buffer_, INFLATE_LITERAL_LENGTH_TABLE_BITS)]};
      unsigned int prefix{0};
      if ((entry & INFLATE_ENTRY_SUBTABLE) != 0) {
        prefix = INFLATE_LITERAL_LENGTH_TABLE_BITS;
        entry = this->literal_length_table_
                    [(entry >> 16) +
                     low_bits(this->bit_buffer_ >> prefix,
                              INFLATE_MAXIMUM_CODEWORD_LENGTH -
                                  INFLATE_LITERAL_LENGTH_TABLE_BITS)];
      }

      if ((entry & INFLATE_ENTRY_INVALID_CODE) != 0) {
        this->fail("Invalid Huffman code", INFLATE_MAXIMUM_CODEWORD_LENGTH);
      }

      const unsigned int consumed{prefix + (entry & 0x1fU)};
      if ((entry & INFLATE_ENTRY_INVALID_SYMBOL) != 0) {
        this->fail("Invalid literal/length code", consumed);
      }

      if ((entry & INFLATE_ENTRY_END_OF_BLOCK) != 0) {
        this->consume(consumed);
        this->check_not_overread();
        this->end_block(buffers);
        return InflateStatus::Done;
      }

      // Nothing past the end of the block needs output space, so only a
      // literal or a match makes a full output buffer suspend the decoder
      if (available_output(buffers) == 0) {
        return InflateStatus::OutputFull;
      }

      if ((entry & INFLATE_ENTRY_LITERAL) != 0) {
        this->consume(consumed);
        this->check_not_overread();
        *buffers.output_next++ = static_cast<std::uint8_t>(entry >> 16);
        continue;
      }

      const unsigned int length_codeword{prefix + ((entry >> 8) & 0x1fU)};
      const std::size_t length{(entry >> 16) +
                               low_bits(this->bit_buffer_ >> length_codeword,
                                        consumed - length_codeword)};
      this->consume(consumed);

      auto distance_entry{this->distance_table_[low_bits(
          this->bit_buffer_, INFLATE_DISTANCE_TABLE_BITS)]};
      unsigned int distance_prefix{0};
      if ((distance_entry & INFLATE_ENTRY_SUBTABLE) != 0) {
        distance_prefix = INFLATE_DISTANCE_TABLE_BITS;
        distance_entry =
            this->distance_table_[(distance_entry >> 16) +
                                  low_bits(this->bit_buffer_ >> distance_prefix,
                                           INFLATE_MAXIMUM_CODEWORD_LENGTH -
                                               INFLATE_DISTANCE_TABLE_BITS)];
      }

      if ((distance_entry & INFLATE_ENTRY_INVALID_CODE) != 0) {
        this->fail("Invalid Huffman code", INFLATE_MAXIMUM_CODEWORD_LENGTH);
      }

      const unsigned int distance_consumed{distance_prefix +
                                           (distance_entry & 0x1fU)};
      if ((distance_entry & INFLATE_ENTRY_INVALID_SYMBOL) != 0) {
        this->fail("Invalid distance code", distance_consumed);
      }

      const unsigned int distance_codeword{distance_prefix +
                                           ((distance_entry >> 8) & 0x1fU)};
      const std::size_t distance{
          (distance_entry >> 16) +
          low_bits(this->bit_buffer_ >> distance_codeword,
                   distance_consumed - distance_codeword)};
      this->consume(distance_consumed);
      this->check_not_overread();

      if (std::cmp_greater(distance,
                           buffers.output_next - this->history_start_)) {
        throw GZIPError{"Backref distance exceeds bytes available"};
      }

      this->pending_length_ = length;
      this->pending_distance_ = distance;
      if (this->copy_match_careful(buffers) != InflateStatus::Done) {
        return InflateStatus::OutputFull;
      }
    }
  }

  State state_{State::MemberStart};
  bool first_member_{true};
  bool final_block_{false};
  bool fixed_tables_{false};
  std::uint8_t flags_{0};
  std::uint64_t bit_buffer_{0};
  unsigned int bits_available_{0};
  unsigned int overread_{0};
  std::size_t skip_remaining_{0};
  std::size_t stored_remaining_{0};
  std::size_t pending_length_{0};
  std::size_t pending_distance_{0};
  std::uint32_t header_checksum_{0};
  std::uint32_t member_checksum_{0};
  std::uint32_t member_size_{0};
  std::size_t member_history_{0};
  const std::uint8_t *checksum_position_{nullptr};
  const std::uint8_t *history_start_{nullptr};
  // Every table is built before it is read, so zeroing them upfront would
  // only slow down every construction
  // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
  std::array<std::uint32_t, INFLATE_LITERAL_LENGTH_TABLE_SIZE>
      literal_length_table_;
  std::array<std::uint32_t, INFLATE_DISTANCE_TABLE_SIZE> distance_table_;
  std::array<std::uint32_t, INFLATE_CODE_LENGTH_TABLE_SIZE> code_length_table_;
  // NOLINTEND(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
};

} // namespace sourcemeta::core

#endif
