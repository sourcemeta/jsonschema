#include <sourcemeta/core/crypto_sha256.h>

#include "crypto_sha256_other.h"

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto sha256_digest(const std::span<const std::string_view> input)
    -> std::array<std::uint8_t, 32> {
  Sha256Context context;
  for (const auto part : input) {
    sha256_update(context, part);
  }

  return sha256_finalize(context);
}

auto sha256_digest(const std::string_view input)
    -> std::array<std::uint8_t, 32> {
  return sha256_digest(std::span<const std::string_view>{&input, 1});
}

} // namespace sourcemeta::core
