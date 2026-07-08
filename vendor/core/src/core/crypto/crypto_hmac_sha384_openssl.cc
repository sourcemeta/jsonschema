#include <sourcemeta/core/crypto_hmac_sha384.h>
#include <sourcemeta/core/crypto_sha384.h>

#include <openssl/evp.h>  // EVP_sha384
#include <openssl/hmac.h> // HMAC

#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto hmac_sha384_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 48> {
  // A key longer than the block size is hashed first (RFC 2104 Section 2),
  // which also keeps the key length within the OpenSSL length parameter
  constexpr std::size_t block_size{128};
  std::array<std::uint8_t, 48> key_digest{};
  const unsigned char *key_data{
      reinterpret_cast<const unsigned char *>(key.data())};
  auto key_size{key.size()};
  if (key_size > block_size) {
    key_digest = sha384_digest(key);
    key_data = key_digest.data();
    key_size = key_digest.size();
  }

  std::array<std::uint8_t, 48> digest{};
  unsigned int length{0};
  if (HMAC(EVP_sha384(), key_data, static_cast<int>(key_size),
           reinterpret_cast<const unsigned char *>(message.data()),
           message.size(), digest.data(), &length) == nullptr) {
    throw std::runtime_error("Could not compute HMAC-SHA384 digest");
  }

  return digest;
}

} // namespace sourcemeta::core
