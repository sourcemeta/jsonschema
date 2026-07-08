#include <sourcemeta/core/crypto_hmac_sha384.h>

#include <CommonCrypto/CommonHMAC.h> // CCHmac, kCCHmacAlgSHA384

#include <array>   // std::array
#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

auto hmac_sha384_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 48> {
  std::array<std::uint8_t, 48> digest{};
  CCHmac(kCCHmacAlgSHA384, key.data(), key.size(), message.data(),
         message.size(), digest.data());
  return digest;
}

} // namespace sourcemeta::core
