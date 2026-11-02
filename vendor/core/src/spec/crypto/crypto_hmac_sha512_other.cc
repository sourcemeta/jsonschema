#include <sourcemeta/core/crypto_hmac_sha512.h>
#include <sourcemeta/core/crypto_sha512.h>

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <cstring> // std::memcpy
#include <string>  // std::string

namespace sourcemeta::core {

auto hmac_sha512_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 64> {
  // SHA-512 operates on 128-byte blocks (FIPS 180-4 Section 5.1.2), which is
  // the block size the HMAC construction pads the key to (RFC 2104 Section 2)
  constexpr std::size_t block_size{128};

  // "If the key is longer than the block size, hash it and use the result"
  // (RFC 2104 Section 2)
  std::array<std::uint8_t, block_size> padded_key{};
  if (key.size() > block_size) {
    const auto digest{sha512_digest(key)};
    std::memcpy(padded_key.data(), digest.data(), digest.size());
  } else if (!key.empty()) {
    std::memcpy(padded_key.data(), key.data(), key.size());
  }

  // H((K XOR opad) || H((K XOR ipad) || message)) with ipad = 0x36 and
  // opad = 0x5c repeated to the block size (RFC 2104 Section 2)
  std::string inner_input(block_size, '\x00');
  std::string outer_input(block_size, '\x00');
  for (std::size_t index = 0; index < block_size; ++index) {
    inner_input[index] = static_cast<char>(padded_key[index] ^ 0x36);
    outer_input[index] = static_cast<char>(padded_key[index] ^ 0x5c);
  }

  inner_input.append(message);
  const auto inner_digest{sha512_digest(inner_input)};
  outer_input.append(reinterpret_cast<const char *>(inner_digest.data()),
                     inner_digest.size());
  return sha512_digest(outer_input);
}

} // namespace sourcemeta::core
