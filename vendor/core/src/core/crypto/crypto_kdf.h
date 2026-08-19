#ifndef SOURCEMETA_CORE_CRYPTO_KDF_H_
#define SOURCEMETA_CORE_CRYPTO_KDF_H_

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

// The hash a key derivation instantiates its HMAC with (RFC 5869 Section 2.1)
enum class KDFHash : std::uint8_t { SHA256, SHA384, SHA512 };

// The digest length in octets, which RFC 5869 calls HashLen
inline auto kdf_digest_bytes(const KDFHash hash) noexcept -> std::size_t {
  switch (hash) {
    case KDFHash::SHA256:
      return 32;
    case KDFHash::SHA384:
      return 48;
    case KDFHash::SHA512:
      return 64;
  }

  std::unreachable();
}

// The widest digest any of the above produces, so a caller can hold one on the
// stack without knowing which hash it asked for
inline constexpr std::size_t KDF_MAXIMUM_DIGEST_BYTES{64};

// The raw RFC 5869 steps, defined once per backend. Every length check lives
// in the shared functions, so these assume an output buffer of the digest
// length for the extract step, and of the requested length, itself no more
// than 255 digests, for the expand step

// RFC 5869 Section 2.2, where an empty salt stands for the digest-length run
// of zeros that section substitutes when none is provided, the two being
// identical once HMAC pads the key out to its block size
auto hkdf_extract(const KDFHash hash, const std::string_view salt,
                  const std::string_view input_key_material,
                  unsigned char *const output) -> bool;

// RFC 5869 Section 2.3
auto hkdf_expand(const KDFHash hash, const std::string_view pseudorandom_key,
                 const std::string_view info, unsigned char *const output,
                 const std::size_t length) -> bool;

// Both steps at once, which a backend with a dedicated key derivation
// primitive performs without surfacing the intermediate pseudorandom key
auto hkdf_derive(const KDFHash hash, const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 unsigned char *const output, const std::size_t length) -> bool;

} // namespace sourcemeta::core

#endif
