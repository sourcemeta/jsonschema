#include <sourcemeta/core/crypto_sha256.h>

#include <CommonCrypto/CommonDigest.h> // CC_SHA256*, CC_LONG

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <limits>  // std::numeric_limits

namespace sourcemeta::core {

auto sha256_digest(const std::string_view input)
    -> std::array<std::uint8_t, 32> {
  CC_SHA256_CTX context;
  CC_SHA256_Init(&context);

  // The platform update interface takes a 32-bit length, so larger
  // inputs must be fed in chunks
  const auto *remaining_data{input.data()};
  auto remaining_size{input.size()};
  constexpr std::size_t maximum_chunk{std::numeric_limits<CC_LONG>::max()};
  while (remaining_size > 0) {
    const auto chunk_size{remaining_size > maximum_chunk ? maximum_chunk
                                                         : remaining_size};
    CC_SHA256_Update(&context, remaining_data,
                     static_cast<CC_LONG>(chunk_size));
    remaining_data += chunk_size;
    remaining_size -= chunk_size;
  }

  std::array<std::uint8_t, CC_SHA256_DIGEST_LENGTH> digest{};
  CC_SHA256_Final(digest.data(), &context);
  return digest;
}

} // namespace sourcemeta::core
