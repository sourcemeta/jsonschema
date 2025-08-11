#include <sourcemeta/core/md5.h>

#include <array>   // std::array
#include <cstdint> // std::uint32_t, std::uint64_t
#include <cstring> // std::memcpy
#include <iomanip> // std::hex, std::setfill

namespace {

inline constexpr auto rotate_left(std::uint32_t value,
                                  std::uint64_t count) noexcept
    -> std::uint32_t {
  return (value << count) | (value >> (32u - count));
}

inline auto md5_process_block(const unsigned char *block,
                              std::array<std::uint32_t, 4> &state) noexcept
    -> void {
  // Constants defined by the MD5 RFC (sine table)
  static constexpr std::array<std::uint32_t, 64> k = {
      {0xd76aa478U, 0xe8c7b756U, 0x242070dbU, 0xc1bdceeeU, 0xf57c0fafU,
       0x4787c62aU, 0xa8304613U, 0xfd469501U, 0x698098d8U, 0x8b44f7afU,
       0xffff5bb1U, 0x895cd7beU, 0x6b901122U, 0xfd987193U, 0xa679438eU,
       0x49b40821U, 0xf61e2562U, 0xc040b340U, 0x265e5a51U, 0xe9b6c7aaU,
       0xd62f105dU, 0x02441453U, 0xd8a1e681U, 0xe7d3fbc8U, 0x21e1cde6U,
       0xc33707d6U, 0xf4d50d87U, 0x455a14edU, 0xa9e3e905U, 0xfcefa3f8U,
       0x676f02d9U, 0x8d2a4c8aU, 0xfffa3942U, 0x8771f681U, 0x6d9d6122U,
       0xfde5380cU, 0xa4beea44U, 0x4bdecfa9U, 0xf6bb4b60U, 0xbebfbc70U,
       0x289b7ec6U, 0xeaa127faU, 0xd4ef3085U, 0x04881d05U, 0xd9d4d039U,
       0xe6db99e5U, 0x1fa27cf8U, 0xc4ac5665U, 0xf4292244U, 0x432aff97U,
       0xab9423a7U, 0xfc93a039U, 0x655b59c3U, 0x8f0ccc92U, 0xffeff47dU,
       0x85845dd1U, 0x6fa87e4fU, 0xfe2ce6e0U, 0xa3014314U, 0x4e0811a1U,
       0xf7537e82U, 0xbd3af235U, 0x2ad7d2bbU, 0xeb86d391U}};

  static constexpr std::array<std::uint64_t, 64> s = {
      {7u,  12u, 17u, 22u, 7u,  12u, 17u, 22u, 7u,  12u, 17u, 22u, 7u,
       12u, 17u, 22u, 5u,  9u,  14u, 20u, 5u,  9u,  14u, 20u, 5u,  9u,
       14u, 20u, 5u,  9u,  14u, 20u, 4u,  11u, 16u, 23u, 4u,  11u, 16u,
       23u, 4u,  11u, 16u, 23u, 4u,  11u, 16u, 23u, 6u,  10u, 15u, 21u,
       6u,  10u, 15u, 21u, 6u,  10u, 15u, 21u, 6u,  10u, 15u, 21u}};

  // Decode 16 little-endian 32-bit words from the block
  std::array<std::uint32_t, 16> message_words{};
  for (std::uint64_t word_index = 0; word_index < 16u; ++word_index) {
    const std::uint64_t byte_index = word_index * 4u;
    message_words[word_index] =
        static_cast<std::uint32_t>(block[byte_index]) |
        (static_cast<std::uint32_t>(block[byte_index + 1u]) << 8u) |
        (static_cast<std::uint32_t>(block[byte_index + 2u]) << 16u) |
        (static_cast<std::uint32_t>(block[byte_index + 3u]) << 24u);
  }

  std::uint32_t a = state[0];
  std::uint32_t b = state[1];
  std::uint32_t c = state[2];
  std::uint32_t d = state[3];

  for (std::uint64_t round_index = 0u; round_index < 64u; ++round_index) {
    std::uint32_t f;
    std::uint64_t g;

    if (round_index < 16u) {
      f = (b & c) | ((~b) & d);
      g = round_index;
    } else if (round_index < 32u) {
      f = (d & b) | ((~d) & c);
      g = (5u * round_index + 1u) % 16u;
    } else if (round_index < 48u) {
      f = b ^ c ^ d;
      g = (3u * round_index + 5u) % 16u;
    } else {
      f = c ^ (b | (~d));
      g = (7u * round_index) % 16u;
    }

    const std::uint32_t temp = d;
    d = c;
    c = b;
    const std::uint32_t computed = a + f + k[round_index] + message_words[g];
    b = b + rotate_left(computed, s[round_index]);
    a = temp;
  }

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

} // namespace

namespace sourcemeta::core {

auto md5(std::string_view input, std::ostream &output) -> void {
  // Initial state as per RFC 1321
  std::array<std::uint32_t, 4> state{};
  state[0] = 0x67452301U;
  state[1] = 0xefcdab89U;
  state[2] = 0x98badcfeU;
  state[3] = 0x10325476U;

  const auto *const input_bytes =
      reinterpret_cast<const unsigned char *>(input.data());
  const std::size_t input_length = input.size();

  // Process all full 64-byte blocks directly from the input (streaming)
  std::size_t processed_bytes = 0u;
  while (input_length - processed_bytes >= 64u) {
    md5_process_block(input_bytes + processed_bytes, state);
    processed_bytes += 64u;
  }

  // Prepare the final block(s) (one or two 64-byte blocks)
  std::array<unsigned char, 128> final_block{};
  const std::size_t remaining_bytes = input_length - processed_bytes;
  if (remaining_bytes > 0u) {
    std::memcpy(final_block.data(), input_bytes + processed_bytes,
                remaining_bytes);
  }

  // Append the 0x80 byte after the message data
  final_block[remaining_bytes] = 0x80u;

  // Append length in bits as little-endian 64-bit at the end of the padding
  const std::uint64_t message_length_bits =
      static_cast<std::uint64_t>(input_length) * 8ull;

  if (remaining_bytes < 56u) {
    // Enough room for length in the first final block
    // place length at final_block[56..63]
    for (std::uint64_t index = 0u; index < 8u; ++index) {
      final_block[56u + index] = static_cast<unsigned char>(
          (message_length_bits >> (8u * index)) & 0xffu);
    }
    md5_process_block(final_block.data(), state);
  } else {
    // Need two blocks: process final_block[0..63] then final_block[64..127]
    // with length
    for (std::uint64_t index = 0u; index < 8u; ++index) {
      final_block[64u + 56u + index] = static_cast<unsigned char>(
          (message_length_bits >> (8u * index)) & 0xffu);
    }

    md5_process_block(final_block.data(), state);
    md5_process_block(final_block.data() + 64u, state);
  }

  // Produce the final digest (little-endian)
  std::array<unsigned char, 16> digest;
  for (std::uint64_t state_index = 0u; state_index < 4u; ++state_index) {
    const std::uint32_t value = state[state_index];
    const std::uint64_t base_index = state_index * 4u;
    digest[base_index + 0u] = static_cast<unsigned char>(value & 0xffu);
    digest[base_index + 1u] = static_cast<unsigned char>((value >> 8u) & 0xffu);
    digest[base_index + 2u] =
        static_cast<unsigned char>((value >> 16u) & 0xffu);
    digest[base_index + 3u] =
        static_cast<unsigned char>((value >> 24u) & 0xffu);
  }

  output << std::hex << std::setfill('0');
  for (const unsigned char octet : digest) {
    output << std::setw(2) << static_cast<std::uint64_t>(octet);
  }

  output.unsetf(std::ios_base::hex);
}

} // namespace sourcemeta::core
