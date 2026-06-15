#include <sourcemeta/core/crypto_sha384.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <limits>    // std::numeric_limits
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto sha384_digest(const std::string_view input)
    -> std::array<std::uint8_t, 48> {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_SHA384_ALGORITHM, nullptr, 0))) {
    throw std::runtime_error("Could not open the CNG SHA-384 provider");
  }

  BCRYPT_HASH_HANDLE hash{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    throw std::runtime_error("Could not create the CNG SHA-384 hash");
  }

  // The data interface is not const-qualified but never writes through
  // the pointer, and it takes a 32-bit length, so larger inputs must be
  // fed in chunks
  auto *remaining_data{
      reinterpret_cast<unsigned char *>(const_cast<char *>(input.data()))};
  auto remaining_size{input.size()};
  constexpr std::size_t maximum_chunk{std::numeric_limits<ULONG>::max()};
  auto success{true};
  while (remaining_size > 0 && success) {
    const auto chunk_size{remaining_size > maximum_chunk ? maximum_chunk
                                                         : remaining_size};
    success = BCRYPT_SUCCESS(BCryptHashData(hash, remaining_data,
                                            static_cast<ULONG>(chunk_size), 0));
    remaining_data += chunk_size;
    remaining_size -= chunk_size;
  }

  std::array<std::uint8_t, 48> digest{};
  if (success) {
    success = BCRYPT_SUCCESS(BCryptFinishHash(
        hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
  }

  BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(algorithm, 0);
  if (!success) {
    throw std::runtime_error("Could not compute the CNG SHA-384 digest");
  }

  return digest;
}

} // namespace sourcemeta::core
