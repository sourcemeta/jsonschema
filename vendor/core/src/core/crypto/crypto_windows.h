#ifndef SOURCEMETA_CORE_CRYPTO_WINDOWS_H_
#define SOURCEMETA_CORE_CRYPTO_WINDOWS_H_

// CNG key export helper shared by the signing and verification backends, so the
// native blob export stays single-sourced

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // ULONG, LPCWSTR

#include <bcrypt.h> // BCryptExportKey, BCRYPT_KEY_HANDLE, BCRYPT_SUCCESS

#include <optional> // std::optional, std::nullopt
#include <string>   // std::string

namespace sourcemeta::core {

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
