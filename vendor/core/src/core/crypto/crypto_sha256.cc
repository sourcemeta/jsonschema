#include <sourcemeta/core/crypto_sha256.h>

#include <array>   // std::array
#include <cstdint> // std::uint32_t, std::uint64_t

#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
#include <openssl/evp.h> // EVP_MD_CTX_new, EVP_DigestInit_ex, EVP_sha256, EVP_DigestUpdate, EVP_DigestFinal_ex, EVP_MD_CTX_free
#include <stdexcept>     // std::runtime_error
#else
#include <cstring> // std::memcpy
#endif

namespace {
constexpr std::array<char, 17> HEX_DIGITS{{'0', '1', '2', '3', '4', '5', '6',
                                           '7', '8', '9', 'a', 'b', 'c', 'd',
                                           'e', 'f', '\0'}};
} // namespace

#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL

namespace sourcemeta::core {

auto sha256(const std::string_view input, std::ostream &output) -> void {
  auto *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    throw std::runtime_error("Could not allocate OpenSSL digest context");
  }

  if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context, input.data(), input.size()) != 1) {
    EVP_MD_CTX_free(context);
    throw std::runtime_error("Could not compute SHA-256 digest");
  }

  std::array<unsigned char, 32> digest{};
  unsigned int length = 0;
  if (EVP_DigestFinal_ex(context, digest.data(), &length) != 1) {
    EVP_MD_CTX_free(context);
    throw std::runtime_error("Could not finalize SHA-256 digest");
  }

  EVP_MD_CTX_free(context);

  for (std::uint64_t index = 0; index < 32u; ++index) {
    output.put(HEX_DIGITS[(digest[index] >> 4u) & 0x0fu]);
    output.put(HEX_DIGITS[digest[index] & 0x0fu]);
  }
}

} // namespace sourcemeta::core

#else

namespace {

inline constexpr auto rotate_right(std::uint32_t value,
                                   std::uint64_t count) noexcept
    -> std::uint32_t {
  return (value >> count) | (value << (32u - count));
}

// FIPS 180-4 Section 4.1.2 logical functions
inline constexpr auto big_sigma_0(std::uint32_t value) noexcept
    -> std::uint32_t {
  return rotate_right(value, 2u) ^ rotate_right(value, 13u) ^
         rotate_right(value, 22u);
}

inline constexpr auto big_sigma_1(std::uint32_t value) noexcept
    -> std::uint32_t {
  return rotate_right(value, 6u) ^ rotate_right(value, 11u) ^
         rotate_right(value, 25u);
}

inline constexpr auto small_sigma_0(std::uint32_t value) noexcept
    -> std::uint32_t {
  return rotate_right(value, 7u) ^ rotate_right(value, 18u) ^ (value >> 3u);
}

inline constexpr auto small_sigma_1(std::uint32_t value) noexcept
    -> std::uint32_t {
  return rotate_right(value, 17u) ^ rotate_right(value, 19u) ^ (value >> 10u);
}

// Equivalent to (x & y) ^ (~x & z) but avoids a bitwise NOT
inline constexpr auto choice(std::uint32_t x, std::uint32_t y,
                             std::uint32_t z) noexcept -> std::uint32_t {
  return z ^ (x & (y ^ z));
}

inline constexpr auto majority(std::uint32_t x, std::uint32_t y,
                               std::uint32_t z) noexcept -> std::uint32_t {
  return (x & y) ^ (x & z) ^ (y & z);
}

inline auto sha256_process_block(const unsigned char *block,
                                 std::array<std::uint32_t, 8> &state) noexcept
    -> void {
  // First 32 bits of the fractional parts of the cube roots
  // of the first 64 prime numbers (FIPS 180-4 Section 4.2.2)
  static constexpr std::array<std::uint32_t, 64> round_constants = {
      {0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU,
       0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U,
       0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U,
       0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
       0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U,
       0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
       0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
       0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
       0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U,
       0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U, 0x1e376c08U,
       0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU,
       0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
       0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U}};

  // Decode 16 big-endian 32-bit words from the block
  std::array<std::uint32_t, 64> schedule;
  for (std::uint64_t word_index = 0; word_index < 16u; ++word_index) {
    const std::uint64_t byte_index = word_index * 4u;
    schedule[word_index] =
        (static_cast<std::uint32_t>(block[byte_index]) << 24u) |
        (static_cast<std::uint32_t>(block[byte_index + 1u]) << 16u) |
        (static_cast<std::uint32_t>(block[byte_index + 2u]) << 8u) |
        static_cast<std::uint32_t>(block[byte_index + 3u]);
  }

  // Extend the message schedule (FIPS 180-4 Section 6.2.2 step 1)
  for (std::uint64_t index = 16u; index < 64u; ++index) {
    schedule[index] =
        small_sigma_1(schedule[index - 2u]) + schedule[index - 7u] +
        small_sigma_0(schedule[index - 15u]) + schedule[index - 16u];
  }

  auto working = state;

  // Compression function (FIPS 180-4 Section 6.2.2 step 3)
  for (std::uint64_t round_index = 0u; round_index < 64u; ++round_index) {
    const auto temporary_1 = working[7] + big_sigma_1(working[4]) +
                             choice(working[4], working[5], working[6]) +
                             round_constants[round_index] +
                             schedule[round_index];
    const auto temporary_2 =
        big_sigma_0(working[0]) + majority(working[0], working[1], working[2]);

    working[7] = working[6];
    working[6] = working[5];
    working[5] = working[4];
    working[4] = working[3] + temporary_1;
    working[3] = working[2];
    working[2] = working[1];
    working[1] = working[0];
    working[0] = temporary_1 + temporary_2;
  }

  for (std::uint64_t index = 0u; index < 8u; ++index) {
    state[index] += working[index];
  }
}

} // namespace

namespace sourcemeta::core {

auto sha256(const std::string_view input, std::ostream &output) -> void {
  // Initial hash values: first 32 bits of the fractional parts of the
  // square roots of the first 8 primes (FIPS 180-4 Section 5.3.3)
  std::array<std::uint32_t, 8> state{};
  state[0] = 0x6a09e667U;
  state[1] = 0xbb67ae85U;
  state[2] = 0x3c6ef372U;
  state[3] = 0xa54ff53aU;
  state[4] = 0x510e527fU;
  state[5] = 0x9b05688cU;
  state[6] = 0x1f83d9abU;
  state[7] = 0x5be0cd19U;

  const auto *const input_bytes =
      reinterpret_cast<const unsigned char *>(input.data());
  const std::size_t input_length = input.size();

  // Process all full 64-byte blocks directly from the input (streaming)
  std::size_t processed_bytes = 0u;
  while (input_length - processed_bytes >= 64u) {
    sha256_process_block(input_bytes + processed_bytes, state);
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

  // Append length in bits as big-endian 64-bit at the end of the padding
  const std::uint64_t message_length_bits =
      static_cast<std::uint64_t>(input_length) * 8ull;

  if (remaining_bytes < 56u) {
    for (std::uint64_t index = 0u; index < 8u; ++index) {
      final_block[56u + index] = static_cast<unsigned char>(
          (message_length_bits >> (8u * (7u - index))) & 0xffu);
    }
    sha256_process_block(final_block.data(), state);
  } else {
    for (std::uint64_t index = 0u; index < 8u; ++index) {
      final_block[64u + 56u + index] = static_cast<unsigned char>(
          (message_length_bits >> (8u * (7u - index))) & 0xffu);
    }

    sha256_process_block(final_block.data(), state);
    sha256_process_block(final_block.data() + 64u, state);
  }

  for (std::uint64_t state_index = 0u; state_index < 8u; ++state_index) {
    const auto value = state[state_index];
    for (std::uint64_t nibble = 0u; nibble < 8u; ++nibble) {
      const auto shift = 28u - nibble * 4u;
      output.put(HEX_DIGITS[(value >> shift) & 0x0fu]);
    }
  }
}

} // namespace sourcemeta::core

#endif
