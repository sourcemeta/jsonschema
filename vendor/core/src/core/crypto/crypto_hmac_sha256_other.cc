#include <sourcemeta/core/crypto_hmac_sha256.h>
#include <sourcemeta/core/crypto_sha256.h>

#include "crypto_sha256_other.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <cstring>     // std::memcpy
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto hmac_sha256_digest(const std::string_view key,
                        const std::span<const std::string_view> message)
    -> std::array<std::uint8_t, 32> {
  // SHA-256 operates on 64-byte blocks (FIPS 180-4 Section 5.1.1), which is the
  // block size the HMAC construction pads the key to (RFC 2104 Section 2)
  constexpr std::size_t block_size{64};

  // "If the key is longer than the block size, hash it and use the result"
  // (RFC 2104 Section 2)
  std::array<std::uint8_t, block_size> padded_key{};
  if (key.size() > block_size) {
    const auto digest{sha256_digest(key)};
    std::memcpy(padded_key.data(), digest.data(), digest.size());
  } else if (!key.empty()) {
    std::memcpy(padded_key.data(), key.data(), key.size());
  }

  // H((K XOR opad) || H((K XOR ipad) || message)) with ipad = 0x36 and
  // opad = 0x5c repeated to the block size (RFC 2104 Section 2)
  std::array<std::uint8_t, block_size> inner_key{};
  std::array<std::uint8_t, block_size> outer_key{};
  for (std::size_t index = 0; index < block_size; ++index) {
    inner_key[index] = static_cast<std::uint8_t>(padded_key[index] ^ 0x36);
    outer_key[index] = static_cast<std::uint8_t>(padded_key[index] ^ 0x5c);
  }

  Sha256Context inner_context;
  sha256_update(
      inner_context,
      {reinterpret_cast<const char *>(inner_key.data()), inner_key.size()});
  for (const auto part : message) {
    sha256_update(inner_context, part);
  }

  const auto inner_digest{sha256_finalize(inner_context)};

  Sha256Context outer_context;
  sha256_update(
      outer_context,
      {reinterpret_cast<const char *>(outer_key.data()), outer_key.size()});
  sha256_update(outer_context,
                {reinterpret_cast<const char *>(inner_digest.data()),
                 inner_digest.size()});
  return sha256_finalize(outer_context);
}

auto hmac_sha256_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 32> {
  return hmac_sha256_digest(key,
                            std::span<const std::string_view>{&message, 1});
}

} // namespace sourcemeta::core
