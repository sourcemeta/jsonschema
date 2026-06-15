#include <sourcemeta/core/crypto_sha512.h>

#include <CommonCrypto/CommonDigest.h> // CC_SHA512*, CC_LONG

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <limits>  // std::numeric_limits

namespace sourcemeta::core {

auto sha512_digest(const std::string_view input)
    -> std::array<std::uint8_t, 64> {
  CC_SHA512_CTX context;
  CC_SHA512_Init(&context);

  // The platform update interface takes a 32-bit length, so larger
  // inputs must be fed in chunks
  const auto *remaining_data{input.data()};
  auto remaining_size{input.size()};
  constexpr std::size_t maximum_chunk{std::numeric_limits<CC_LONG>::max()};
  while (remaining_size > 0) {
    const auto chunk_size{remaining_size > maximum_chunk ? maximum_chunk
                                                         : remaining_size};
    CC_SHA512_Update(&context, remaining_data,
                     static_cast<CC_LONG>(chunk_size));
    remaining_data += chunk_size;
    remaining_size -= chunk_size;
  }

  std::array<std::uint8_t, 64> digest{};
  CC_SHA512_Final(digest.data(), &context);
  return digest;
}

} // namespace sourcemeta::core
