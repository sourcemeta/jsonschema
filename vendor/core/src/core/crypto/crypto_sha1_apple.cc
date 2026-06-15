#include <sourcemeta/core/crypto_sha1.h>
#include <sourcemeta/core/text.h>

#include <CommonCrypto/CommonDigest.h> // CC_SHA1*, CC_LONG

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <limits>  // std::numeric_limits

namespace sourcemeta::core {

auto sha1(const std::string_view input) -> std::string {
  // The platform marks its SHA-1 interfaces as deprecated because the
  // algorithm is cryptographically broken, but this module keeps exposing
  // SHA-1 for non-security use cases
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  CC_SHA1_CTX context;
  CC_SHA1_Init(&context);

  // The platform update interface takes a 32-bit length, so larger
  // inputs must be fed in chunks
  const auto *remaining_data{input.data()};
  auto remaining_size{input.size()};
  constexpr std::size_t maximum_chunk{std::numeric_limits<CC_LONG>::max()};
  while (remaining_size > 0) {
    const auto chunk_size{remaining_size > maximum_chunk ? maximum_chunk
                                                         : remaining_size};
    CC_SHA1_Update(&context, remaining_data, static_cast<CC_LONG>(chunk_size));
    remaining_data += chunk_size;
    remaining_size -= chunk_size;
  }

  std::array<unsigned char, CC_SHA1_DIGEST_LENGTH> digest{};
  CC_SHA1_Final(digest.data(), &context);
#pragma clang diagnostic pop

  return bytes_to_hex(
      {reinterpret_cast<const char *>(digest.data()), digest.size()});
}

} // namespace sourcemeta::core
