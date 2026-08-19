#ifndef SOURCEMETA_CORE_CRYPTO_HKDF_LOOP_H_
#define SOURCEMETA_CORE_CRYPTO_HKDF_LOOP_H_

#include <sourcemeta/core/crypto_hmac_sha256.h>
#include <sourcemeta/core/crypto_hmac_sha384.h>
#include <sourcemeta/core/crypto_hmac_sha512.h>
#include <sourcemeta/core/crypto_secure.h>

#include "crypto_kdf.h"

#include <algorithm>   // std::copy_n, std::min
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

// RFC 5869 builds entirely on HMAC, so a backend without a dedicated key
// derivation primitive composes one from the HMAC it already provides

// The counterpart of the string scope for a fixed digest buffer, since the
// HMAC underneath throws on two of the backends and an unwind would otherwise
// leave a block of derived key material behind
struct SecureDigestScope {
  explicit SecureDigestScope(
      std::array<std::uint8_t, KDF_MAXIMUM_DIGEST_BYTES> &value) noexcept
      : target{value} {}
  SecureDigestScope(const SecureDigestScope &) = delete;
  auto operator=(const SecureDigestScope &) -> SecureDigestScope & = delete;
  SecureDigestScope(SecureDigestScope &&) = delete;
  auto operator=(SecureDigestScope &&) -> SecureDigestScope & = delete;
  ~SecureDigestScope() {
    secure_zero(this->target.data(), this->target.size());
  }
  std::array<std::uint8_t, KDF_MAXIMUM_DIGEST_BYTES> &target;
};

inline auto
hkdf_loop_hmac(const KDFHash hash, const std::string_view key,
               const std::string_view message,
               std::array<std::uint8_t, KDF_MAXIMUM_DIGEST_BYTES> &output)
    -> std::size_t {
  switch (hash) {
    case KDFHash::SHA256: {
      auto digest{hmac_sha256_digest(key, message)};
      std::copy_n(digest.begin(), digest.size(), output.begin());
      secure_zero(digest.data(), digest.size());
      return digest.size();
    }
    case KDFHash::SHA384: {
      auto digest{hmac_sha384_digest(key, message)};
      std::copy_n(digest.begin(), digest.size(), output.begin());
      secure_zero(digest.data(), digest.size());
      return digest.size();
    }
    case KDFHash::SHA512: {
      auto digest{hmac_sha512_digest(key, message)};
      std::copy_n(digest.begin(), digest.size(), output.begin());
      secure_zero(digest.data(), digest.size());
      return digest.size();
    }
  }

  std::unreachable();
}

// RFC 5869 Section 2.2: PRK = HMAC-Hash(salt, IKM), noting that the salt keys
// the HMAC while the input keying material is the message
inline auto hkdf_extract_loop(const KDFHash hash, const std::string_view salt,
                              const std::string_view input_key_material,
                              unsigned char *const output) -> bool {
  std::array<std::uint8_t, KDF_MAXIMUM_DIGEST_BYTES> digest{};
  const SecureDigestScope scope{digest};
  const auto size{hkdf_loop_hmac(hash, salt, input_key_material, digest)};
  std::copy_n(digest.begin(), size, output);
  return true;
}

// RFC 5869 Section 2.3: T(i) = HMAC-Hash(PRK, T(i-1) | info | i), with the
// output being the first L octets of the concatenated blocks
inline auto hkdf_expand_loop(const KDFHash hash,
                             const std::string_view pseudorandom_key,
                             const std::string_view info,
                             unsigned char *const output,
                             const std::size_t length) -> bool {
  std::array<std::uint8_t, KDF_MAXIMUM_DIGEST_BYTES> block{};
  const SecureDigestScope block_scope{block};
  std::size_t block_size{0};
  std::size_t produced{0};
  std::uint8_t counter{0};

  std::string message;
  const SecureStringScope message_scope{message};
  message.reserve(kdf_digest_bytes(hash) + info.size() + 1);

  while (produced < length) {
    // The shared entry point caps the length at 255 digests, so the single
    // octet the block index occupies cannot wrap
    counter = static_cast<std::uint8_t>(counter + 1);
    message.assign(reinterpret_cast<const char *>(block.data()), block_size);
    message.append(info);
    message.push_back(static_cast<char>(counter));
    block_size = hkdf_loop_hmac(hash, pseudorandom_key, message, block);
    const auto usable{std::min(block_size, length - produced)};
    std::copy_n(block.begin(), usable, output + produced);
    produced += usable;
  }

  return true;
}

// RFC 5869 Section 2: the extract-then-expand paradigm, with the intermediate
// pseudorandom key wiped once it has served the expansion
inline auto
hkdf_derive_loop(const KDFHash hash, const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 unsigned char *const output, const std::size_t length)
    -> bool {
  std::array<std::uint8_t, KDF_MAXIMUM_DIGEST_BYTES> pseudorandom_key{};
  const SecureDigestScope scope{pseudorandom_key};
  if (!hkdf_extract_loop(hash, salt, input_key_material,
                         pseudorandom_key.data())) {
    return false;
  }

  return hkdf_expand_loop(
      hash,
      std::string_view{reinterpret_cast<const char *>(pseudorandom_key.data()),
                       kdf_digest_bytes(hash)},
      info, output, length);
}

} // namespace sourcemeta::core

#endif
