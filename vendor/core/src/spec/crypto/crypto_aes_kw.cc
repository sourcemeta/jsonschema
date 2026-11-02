#include <sourcemeta/core/crypto_aes_kw.h>

#include "crypto_aes.h"

#include <cstddef>     // std::size_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {
// AES Key Wrap operates on 64-bit semiblocks (RFC 3394 Section 2)
constexpr std::size_t SEMIBLOCK_BYTES{8};
// An upper bound the backends can all process without narrowing their lengths
constexpr std::size_t MAX_INPUT_BYTES{
    static_cast<std::size_t>(std::numeric_limits<int>::max())};

auto is_valid_key_size(const std::size_t size) -> bool {
  return size == 16 || size == 24 || size == 32;
}
} // namespace

auto aes_key_wrap(const std::string_view key_encryption_key,
                  const std::string_view key) -> std::optional<std::string> {
  // The key must be at least two semiblocks (RFC 3394 Section 2), and the bound
  // leaves headroom for the extra semiblock the wrap prepends
  if (!is_valid_key_size(key_encryption_key.size()) ||
      key.size() < 2 * SEMIBLOCK_BYTES || key.size() % SEMIBLOCK_BYTES != 0 ||
      key.size() > MAX_INPUT_BYTES - SEMIBLOCK_BYTES) {
    return std::nullopt;
  }

  return aes_wrap(key_encryption_key, key);
}

auto aes_key_unwrap(const std::string_view key_encryption_key,
                    const std::string_view wrapped_key)
    -> std::optional<std::string> {
  // The wrapped key is the integrity semiblock followed by at least two more
  if (!is_valid_key_size(key_encryption_key.size()) ||
      wrapped_key.size() < 3 * SEMIBLOCK_BYTES ||
      wrapped_key.size() % SEMIBLOCK_BYTES != 0 ||
      wrapped_key.size() > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  return aes_unwrap(key_encryption_key, wrapped_key);
}

} // namespace sourcemeta::core
