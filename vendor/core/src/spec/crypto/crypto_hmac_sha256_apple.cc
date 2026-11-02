#include <sourcemeta/core/crypto_hmac_sha256.h>

// CCHmac, CCHmacContext, CCHmacInit, CCHmacUpdate, CCHmacFinal,
// kCCHmacAlgSHA256
#include <CommonCrypto/CommonHMAC.h>

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto hmac_sha256_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 32> {
  std::array<std::uint8_t, 32> digest{};
  CCHmac(kCCHmacAlgSHA256, key.data(), key.size(), message.data(),
         message.size(), digest.data());
  return digest;
}

auto hmac_sha256_digest(const std::string_view key,
                        const std::span<const std::string_view> message)
    -> std::array<std::uint8_t, 32> {
  CCHmacContext context;
  CCHmacInit(&context, kCCHmacAlgSHA256, key.data(), key.size());

  for (const auto part : message) {
    CCHmacUpdate(&context, part.data(), part.size());
  }

  std::array<std::uint8_t, 32> digest{};
  CCHmacFinal(&context, digest.data());
  return digest;
}

} // namespace sourcemeta::core
