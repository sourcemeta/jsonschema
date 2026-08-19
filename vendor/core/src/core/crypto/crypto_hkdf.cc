#include <sourcemeta/core/crypto_hkdf.h>
#include <sourcemeta/core/crypto_secure.h>

#include "crypto_kdf.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// RFC 5869 Section 2.3: L is at most 255*HashLen octets
auto is_valid_length(const sourcemeta::core::KDFHash hash,
                     const std::size_t length) noexcept -> bool {
  return length <= 255 * sourcemeta::core::kdf_digest_bytes(hash);
}

auto as_bytes(std::string &value) noexcept -> unsigned char * {
  return reinterpret_cast<unsigned char *>(value.data());
}

template <std::size_t Size>
auto extract(const sourcemeta::core::KDFHash hash, const std::string_view salt,
             const std::string_view input_key_material)
    -> std::array<std::uint8_t, Size> {
  std::array<std::uint8_t, Size> output{};
  // A refused extraction would otherwise leave the zero-filled buffer standing
  // in for a pseudorandom key, which is both wrong and predictable, so it
  // throws as the HMAC underneath does rather than reporting a derivation that
  // never happened
  if (!sourcemeta::core::hkdf_extract(hash, salt, input_key_material,
                                      output.data())) {
    throw std::runtime_error("Could not extract an HKDF pseudorandom key");
  }

  return output;
}

auto expand(const sourcemeta::core::KDFHash hash,
            const std::string_view pseudorandom_key,
            const std::string_view info, const std::size_t length)
    -> std::optional<std::string> {
  // RFC 5869 Section 2.3: the pseudorandom key is at least HashLen octets,
  // which the output of the extract step always is
  if (pseudorandom_key.size() < sourcemeta::core::kdf_digest_bytes(hash) ||
      !is_valid_length(hash, length)) {
    return std::nullopt;
  }

  // An empty request needs no block at all, and asking a backend for one
  // invites disagreement over whether that is an error
  if (length == 0) {
    return std::string{};
  }

  std::string output(length, '\0');
  if (!sourcemeta::core::hkdf_expand(hash, pseudorandom_key, info,
                                     as_bytes(output), length)) {
    return std::nullopt;
  }

  return output;
}

auto derive(const sourcemeta::core::KDFHash hash,
            const std::string_view input_key_material,
            const std::string_view salt, const std::string_view info,
            const std::size_t length) -> std::optional<std::string> {
  if (!is_valid_length(hash, length)) {
    return std::nullopt;
  }

  if (length == 0) {
    return std::string{};
  }

  std::string output(length, '\0');
  if (!sourcemeta::core::hkdf_derive(hash, input_key_material, salt, info,
                                     as_bytes(output), length)) {
    return std::nullopt;
  }

  return output;
}

} // namespace

namespace sourcemeta::core {

auto hkdf_sha256_extract(const std::string_view salt,
                         const std::string_view input_key_material)
    -> std::array<std::uint8_t, 32> {
  return extract<32>(KDFHash::SHA256, salt, input_key_material);
}

auto hkdf_sha256_expand(const std::string_view pseudorandom_key,
                        const std::string_view info, const std::size_t length)
    -> std::optional<std::string> {
  return expand(KDFHash::SHA256, pseudorandom_key, info, length);
}

auto hkdf_sha256(const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 const std::size_t length) -> std::optional<std::string> {
  return derive(KDFHash::SHA256, input_key_material, salt, info, length);
}

auto hkdf_sha384_extract(const std::string_view salt,
                         const std::string_view input_key_material)
    -> std::array<std::uint8_t, 48> {
  return extract<48>(KDFHash::SHA384, salt, input_key_material);
}

auto hkdf_sha384_expand(const std::string_view pseudorandom_key,
                        const std::string_view info, const std::size_t length)
    -> std::optional<std::string> {
  return expand(KDFHash::SHA384, pseudorandom_key, info, length);
}

auto hkdf_sha384(const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 const std::size_t length) -> std::optional<std::string> {
  return derive(KDFHash::SHA384, input_key_material, salt, info, length);
}

auto hkdf_sha512_extract(const std::string_view salt,
                         const std::string_view input_key_material)
    -> std::array<std::uint8_t, 64> {
  return extract<64>(KDFHash::SHA512, salt, input_key_material);
}

auto hkdf_sha512_expand(const std::string_view pseudorandom_key,
                        const std::string_view info, const std::size_t length)
    -> std::optional<std::string> {
  return expand(KDFHash::SHA512, pseudorandom_key, info, length);
}

auto hkdf_sha512(const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 const std::size_t length) -> std::optional<std::string> {
  return derive(KDFHash::SHA512, input_key_material, salt, info, length);
}

} // namespace sourcemeta::core
