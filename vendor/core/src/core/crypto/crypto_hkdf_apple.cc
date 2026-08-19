#include "crypto_hkdf_loop.h"
#include "crypto_kdf.h"
#include "crypto_kdf_apple.h"

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

// The selector the Swift shim switches on, kept in step with the shared
// enumeration rather than relying on its underlying values
auto to_selector(const sourcemeta::core::KDFHash hash) noexcept -> int {
  switch (hash) {
    case sourcemeta::core::KDFHash::SHA256:
      return 0;
    case sourcemeta::core::KDFHash::SHA384:
      return 1;
    case sourcemeta::core::KDFHash::SHA512:
      return 2;
  }

  std::unreachable();
}

auto octets(const std::string_view value) noexcept -> const unsigned char * {
  return reinterpret_cast<const unsigned char *>(value.data());
}

} // namespace

namespace sourcemeta::core {

auto hkdf_extract(const KDFHash hash, const std::string_view salt,
                  const std::string_view input_key_material,
                  unsigned char *const output) -> bool {
  if (sourcemeta_core_hkdf_extract_cryptokit(
          to_selector(hash), octets(input_key_material),
          input_key_material.size(), octets(salt), salt.size(), output,
          kdf_digest_bytes(hash))) {
    return true;
  }

  return hkdf_extract_loop(hash, salt, input_key_material, output);
}

auto hkdf_expand(const KDFHash hash, const std::string_view pseudorandom_key,
                 const std::string_view info, unsigned char *const output,
                 const std::size_t length) -> bool {
  if (sourcemeta_core_hkdf_expand_cryptokit(
          to_selector(hash), octets(pseudorandom_key), pseudorandom_key.size(),
          octets(info), info.size(), output, length)) {
    return true;
  }

  return hkdf_expand_loop(hash, pseudorandom_key, info, output, length);
}

auto hkdf_derive(const KDFHash hash, const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 unsigned char *const output, const std::size_t length)
    -> bool {
  if (sourcemeta_core_hkdf_derive_cryptokit(
          to_selector(hash), octets(input_key_material),
          input_key_material.size(), octets(salt), salt.size(), octets(info),
          info.size(), output, length)) {
    return true;
  }

  return hkdf_derive_loop(hash, input_key_material, salt, info, output, length);
}

} // namespace sourcemeta::core
