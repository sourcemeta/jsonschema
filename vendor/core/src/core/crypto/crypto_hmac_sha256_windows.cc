#include <sourcemeta/core/crypto_hmac_sha256.h>
#include <sourcemeta/core/crypto_sha256.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include "crypto_windows.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <stdexcept>   // std::runtime_error
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto hmac_sha256_digest(const std::string_view key,
                        const std::span<const std::string_view> message)
    -> std::array<std::uint8_t, 32> {
  // A key longer than the block size is hashed first (RFC 2104 Section 2),
  // which also keeps the key length within the CNG length parameter. The
  // prehash runs before the provider is opened so that a throwing digest cannot
  // leak the handle. The secret interface is not const-qualified but never
  // writes through the pointer
  constexpr std::size_t block_size{64};
  std::array<std::uint8_t, 32> key_digest{};
  auto *secret{
      reinterpret_cast<unsigned char *>(const_cast<char *>(key.data()))};
  auto secret_size{key.size()};
  if (secret_size > block_size) {
    key_digest = sha256_digest(key);
    secret = key_digest.data();
    secret_size = key_digest.size();
  }

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                      nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG))) {
    throw std::runtime_error("Could not open the CNG HMAC-SHA256 provider");
  }

  BCRYPT_HASH_HANDLE hash{nullptr};
  if (!BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash, nullptr, 0, secret,
                                       static_cast<ULONG>(secret_size), 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    throw std::runtime_error("Could not create the CNG HMAC-SHA256 hash");
  }

  auto success{true};
  for (const auto part : message) {
    success = success && hash_data_chunked(hash, part);
  }

  std::array<std::uint8_t, 32> digest{};
  if (success) {
    success = BCRYPT_SUCCESS(BCryptFinishHash(
        hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
  }

  BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(algorithm, 0);
  if (!success) {
    throw std::runtime_error("Could not compute the CNG HMAC-SHA256 digest");
  }

  return digest;
}

auto hmac_sha256_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 32> {
  return hmac_sha256_digest(key,
                            std::span<const std::string_view>{&message, 1});
}

} // namespace sourcemeta::core
