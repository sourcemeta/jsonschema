#include <sourcemeta/core/oauth_random.h>

#include <sourcemeta/core/crypto.h>

#include <algorithm>   // std::ranges::copy
#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto oauth_random_token() -> std::array<char, 43> {
  // The token is not secret, so unlike the PKCE verifier the entropy needs no
  // wiping storage, matching the state and nonce classification of the design
  std::array<std::uint8_t, 32> entropy{};
  random_bytes(entropy);
  const auto encoded{base64url_encode(
      std::string_view{reinterpret_cast<const char *>(entropy.data()), // NOLINT
                       entropy.size()})};

  std::array<char, 43> result{};
  std::ranges::copy(encoded, result.begin());
  return result;
}

} // namespace sourcemeta::core
