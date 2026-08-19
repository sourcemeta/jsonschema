#include "crypto_hkdf_loop.h"
#include "crypto_kdf.h"

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto hkdf_extract(const KDFHash hash, const std::string_view salt,
                  const std::string_view input_key_material,
                  unsigned char *const output) -> bool {
  return hkdf_extract_loop(hash, salt, input_key_material, output);
}

auto hkdf_expand(const KDFHash hash, const std::string_view pseudorandom_key,
                 const std::string_view info, unsigned char *const output,
                 const std::size_t length) -> bool {
  return hkdf_expand_loop(hash, pseudorandom_key, info, output, length);
}

auto hkdf_derive(const KDFHash hash, const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 unsigned char *const output, const std::size_t length)
    -> bool {
  return hkdf_derive_loop(hash, input_key_material, salt, info, output, length);
}

} // namespace sourcemeta::core
