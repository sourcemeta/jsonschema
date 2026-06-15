#include <sourcemeta/core/crypto_sha1.h>

#include <ostream>     // std::ostream, std::streamsize
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto sha1(const std::string_view input, std::ostream &output) -> void {
  const auto result = sha1(input);
  output.write(result.data(), static_cast<std::streamsize>(result.size()));
}

} // namespace sourcemeta::core
