#include "crypto_aes.h"

#include <CommonCrypto/CommonCryptor.h> // kCCSuccess
#include <CommonCrypto/CommonSymmetricKeywrap.h> // CCSymmetricKey*, CCrfc3394_iv, kCCWRAPAES

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
constexpr std::size_t SEMIBLOCK_BYTES{8};
} // namespace

namespace sourcemeta::core {

auto aes_wrap(const std::string_view key_encryption_key,
              const std::string_view key) -> std::optional<std::string> {
  std::string wrapped(key.size() + SEMIBLOCK_BYTES, '\x00');
  std::size_t wrapped_size{wrapped.size()};
  const auto status{CCSymmetricKeyWrap(
      kCCWRAPAES, CCrfc3394_iv, CCrfc3394_ivLen,
      reinterpret_cast<const std::uint8_t *>(key_encryption_key.data()),
      key_encryption_key.size(),
      reinterpret_cast<const std::uint8_t *>(key.data()), key.size(),
      reinterpret_cast<std::uint8_t *>(wrapped.data()), &wrapped_size)};
  if (status != kCCSuccess || wrapped_size != wrapped.size()) {
    return std::nullopt;
  }

  return wrapped;
}

auto aes_unwrap(const std::string_view key_encryption_key,
                const std::string_view wrapped_key)
    -> std::optional<std::string> {
  std::string key(wrapped_key.size() - SEMIBLOCK_BYTES, '\x00');
  std::size_t key_size{key.size()};
  // A failed integrity check surfaces as a non-success status
  const auto status{CCSymmetricKeyUnwrap(
      kCCWRAPAES, CCrfc3394_iv, CCrfc3394_ivLen,
      reinterpret_cast<const std::uint8_t *>(key_encryption_key.data()),
      key_encryption_key.size(),
      reinterpret_cast<const std::uint8_t *>(wrapped_key.data()),
      wrapped_key.size(), reinterpret_cast<std::uint8_t *>(key.data()),
      &key_size)};
  if (status != kCCSuccess || key_size != key.size()) {
    return std::nullopt;
  }

  return key;
}

} // namespace sourcemeta::core
