#include <sourcemeta/core/crypto_hmac_sha512.h>

#include <CommonCrypto/CommonHMAC.h> // CCHmac, kCCHmacAlgSHA512

#include <array>   // std::array
#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

auto hmac_sha512_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 64> {
  std::array<std::uint8_t, 64> digest{};
  CCHmac(kCCHmacAlgSHA512, key.data(), key.size(), message.data(),
         message.size(), digest.data());
  return digest;
}

} // namespace sourcemeta::core
