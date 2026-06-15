#include <sourcemeta/core/crypto_sha512.h>
#include <sourcemeta/core/text.h>

#include <ostream>     // std::ostream, std::streamsize
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto sha512(const std::string_view input) -> std::string {
  const auto digest{sha512_digest(input)};
  return bytes_to_hex(
      {reinterpret_cast<const char *>(digest.data()), digest.size()});
}

auto sha512(const std::string_view input, std::ostream &output) -> void {
  const auto result = sha512(input);
  output.write(result.data(), static_cast<std::streamsize>(result.size()));
}

} // namespace sourcemeta::core
