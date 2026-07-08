#include <sourcemeta/core/crypto.h>

#include "crypto_random.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <string>      // std::string
#include <type_traits> // std::is_same_v

namespace sourcemeta::core {

// Writing random bytes into the string's storage reinterprets its char buffer
// as bytes, which is well-defined only because std::uint8_t aliases unsigned
// char, the type permitted to alias any object representation
static_assert(std::is_same_v<std::uint8_t, unsigned char>);

auto random_bytes(std::span<std::uint8_t> buffer) -> void {
  if (buffer.empty()) {
    return;
  }

  fill_random_bytes(buffer);
}

auto random_bytes(const std::size_t length) -> std::string {
  std::string result(length, '\0');
  random_bytes(std::span<std::uint8_t>{
      reinterpret_cast<std::uint8_t *>(result.data()), length});
  return result;
}

} // namespace sourcemeta::core
