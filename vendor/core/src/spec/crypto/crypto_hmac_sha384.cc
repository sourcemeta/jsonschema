#include <sourcemeta/core/crypto_hmac_sha384.h>
#include <sourcemeta/core/text.h>

#include <ostream>     // std::ostream, std::streamsize
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto hmac_sha384(const std::string_view key, const std::string_view message)
    -> std::string {
  const auto digest{hmac_sha384_digest(key, message)};
  return bytes_to_hex(
      {reinterpret_cast<const char *>(digest.data()), digest.size()});
}

auto hmac_sha384(const std::string_view key, const std::string_view message,
                 std::ostream &output) -> void {
  const auto result{hmac_sha384(key, message)};
  output.write(result.data(), static_cast<std::streamsize>(result.size()));
}

} // namespace sourcemeta::core
