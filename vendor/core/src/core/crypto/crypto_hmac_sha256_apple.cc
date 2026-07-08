#include <sourcemeta/core/crypto_hmac_sha256.h>

#include <CommonCrypto/CommonHMAC.h> // CCHmac, kCCHmacAlgSHA256

#include <array>   // std::array
#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

auto hmac_sha256_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 32> {
  std::array<std::uint8_t, 32> digest{};
  CCHmac(kCCHmacAlgSHA256, key.data(), key.size(), message.data(),
         message.size(), digest.data());
  return digest;
}

} // namespace sourcemeta::core
