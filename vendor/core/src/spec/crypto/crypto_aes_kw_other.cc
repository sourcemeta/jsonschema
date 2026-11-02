#include "crypto_aes.h"
#include "crypto_aes_block.h"
#include "crypto_aes_kw_loop.h"

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

// A from-scratch AES Key Wrap (RFC 3394) for the reference backend, running the
// shared loop over the shared AES block cipher. This is not constant-time,
// which is acceptable only because this backend is the non-production fallback

namespace sourcemeta::core {

auto aes_wrap(const std::string_view key_encryption_key,
              const std::string_view key) -> std::optional<std::string> {
  const auto schedule{aes_expand_key(key_encryption_key)};
  return aes_kw_wrap_loop(
      [&schedule](
          const AesKeyWrapBlock &block) -> std::optional<AesKeyWrapBlock> {
        return aes_encrypt_block(schedule, block);
      },
      key);
}

auto aes_unwrap(const std::string_view key_encryption_key,
                const std::string_view wrapped_key)
    -> std::optional<std::string> {
  const auto schedule{aes_expand_key(key_encryption_key)};
  return aes_kw_unwrap_loop(
      [&schedule](
          const AesKeyWrapBlock &block) -> std::optional<AesKeyWrapBlock> {
        return aes_decrypt_block(schedule, block);
      },
      wrapped_key);
}

} // namespace sourcemeta::core
