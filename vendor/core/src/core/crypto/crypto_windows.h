#ifndef SOURCEMETA_CORE_CRYPTO_WINDOWS_H_
#define SOURCEMETA_CORE_CRYPTO_WINDOWS_H_

// The CNG key internals and the helpers shared across the CNG backends, so the
// parsed key layout, the native blob export, and the chunked hash ingestion
// stay single-sourced

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // ULONG, LPCWSTR

// BCryptExportKey, BCryptHashData, BCRYPT_KEY_HANDLE, BCRYPT_HASH_HANDLE,
// BCRYPT_SUCCESS
#include <bcrypt.h>

#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/crypto_verify.h>

#include <cstddef>     // std::size_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// The parsed key keeps both the algorithm provider and the imported key handle
// alive for reuse. The Edwards curves have no CNG primitive, so they keep the
// raw seed or point and operate through the reference implementation
struct PublicKey::Internal {
  PublicKey::Type kind;
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
  std::size_t field_bytes;
  std::string modulus;
  std::string edwards_point;
  EdwardsCurve edwards_curve;
};

struct PrivateKey::Internal {
  PrivateKey::Type kind;
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
  std::size_t field_bytes;
  std::string edwards_seed;
  EdwardsCurve edwards_curve;
  bool rsa_pss_restricted{false};
};

// Feed a buffer into a hash object. The data interface is not const-qualified
// but never writes through the pointer, and it takes a 32-bit length, so
// larger inputs must be fed in chunks
inline auto hash_data_chunked(BCRYPT_HASH_HANDLE hash,
                              const std::string_view input) -> bool {
  auto *remaining_data{
      reinterpret_cast<unsigned char *>(const_cast<char *>(input.data()))};
  auto remaining_size{input.size()};
  constexpr std::size_t maximum_chunk{std::numeric_limits<ULONG>::max()};
  while (remaining_size > 0) {
    const auto chunk_size{remaining_size > maximum_chunk ? maximum_chunk
                                                         : remaining_size};
    if (!BCRYPT_SUCCESS(BCryptHashData(hash, remaining_data,
                                       static_cast<ULONG>(chunk_size), 0))) {
      return false;
    }

    remaining_data += chunk_size;
    remaining_size -= chunk_size;
  }

  return true;
}

// Export a native key blob, sizing the buffer in a first call and filling it in
// a second
inline auto export_key_blob(BCRYPT_KEY_HANDLE key, LPCWSTR blob_type)
    -> std::optional<std::string> {
  ULONG size{0};
  if (!BCRYPT_SUCCESS(
          BCryptExportKey(key, nullptr, blob_type, nullptr, 0, &size, 0))) {
    return std::nullopt;
  }

  std::string blob;
  blob.resize(size);
  if (!BCRYPT_SUCCESS(BCryptExportKey(
          key, nullptr, blob_type,
          reinterpret_cast<unsigned char *>(blob.data()), size, &size, 0))) {
    return std::nullopt;
  }

  blob.resize(size);
  return blob;
}

} // namespace sourcemeta::core

#endif
