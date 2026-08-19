#include "crypto_hkdf_loop.h"
#include "crypto_kdf.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG, PUCHAR

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <cstddef>     // std::size_t
#include <limits>      // std::numeric_limits
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto hash_name(const sourcemeta::core::KDFHash hash) noexcept -> LPCWSTR {
  switch (hash) {
    case sourcemeta::core::KDFHash::SHA256:
      return BCRYPT_SHA256_ALGORITHM;
    case sourcemeta::core::KDFHash::SHA384:
      return BCRYPT_SHA384_ALGORITHM;
    case sourcemeta::core::KDFHash::SHA512:
      return BCRYPT_SHA512_ALGORITHM;
  }

  std::unreachable();
}

auto hash_name_bytes(const sourcemeta::core::KDFHash hash) noexcept -> ULONG {
  switch (hash) {
    case sourcemeta::core::KDFHash::SHA256:
      return sizeof(BCRYPT_SHA256_ALGORITHM);
    case sourcemeta::core::KDFHash::SHA384:
      return sizeof(BCRYPT_SHA384_ALGORITHM);
    case sourcemeta::core::KDFHash::SHA512:
      return sizeof(BCRYPT_SHA512_ALGORITHM);
  }

  std::unreachable();
}

auto as_native(const std::string_view value) noexcept -> PUCHAR {
  return reinterpret_cast<PUCHAR>(const_cast<char *>(value.data()));
}

// The provider carries the derivation from the input keying material through
// to the output. Whether the secret is finalized as input keying material to
// be salted, or as an already extracted pseudorandom key, is what separates
// the two RFC 5869 steps here
auto derive(const sourcemeta::core::KDFHash hash, const std::string_view secret,
            const std::string_view salt, const bool secret_is_pseudorandom_key,
            const std::string_view info, unsigned char *const output,
            const std::size_t length) -> bool {
  // The provider takes its lengths as a 32-bit count
  constexpr auto maximum{
      static_cast<std::size_t>(std::numeric_limits<ULONG>::max())};
  if (secret.size() > maximum || salt.size() > maximum ||
      info.size() > maximum || length > maximum) {
    return false;
  }

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  // The key derivation provider is absent before Windows 10 version 1803, and
  // a caller falls back to composing the derivation from HMAC
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_HKDF_ALGORITHM, nullptr, 0))) {
    return false;
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  auto success{BCRYPT_SUCCESS(
      BCryptGenerateSymmetricKey(algorithm, &key, nullptr, 0, as_native(secret),
                                 static_cast<ULONG>(secret.size()), 0))};

  success =
      success &&
      BCRYPT_SUCCESS(BCryptSetProperty(
          key, BCRYPT_HKDF_HASH_ALGORITHM,
          reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(hash_name(hash))),
          hash_name_bytes(hash), 0));

  if (secret_is_pseudorandom_key) {
    // RFC 5869 Section 2.3 alone, where the secret is already the
    // pseudorandom key and no extract step runs
    success = success && BCRYPT_SUCCESS(BCryptSetProperty(
                             key, BCRYPT_HKDF_PRK_AND_FINALIZE, nullptr, 0, 0));
  } else {
    // RFC 5869 Section 2.2 followed by Section 2.3
    success =
        success && BCRYPT_SUCCESS(BCryptSetProperty(
                       key, BCRYPT_HKDF_SALT_AND_FINALIZE, as_native(salt),
                       static_cast<ULONG>(salt.size()), 0));
  }

  BCryptBuffer buffers[1]{};
  buffers[0].BufferType = KDF_HKDF_INFO;
  buffers[0].cbBuffer = static_cast<ULONG>(info.size());
  buffers[0].pvBuffer = as_native(info);
  BCryptBufferDesc parameters{};
  parameters.ulVersion = BCRYPTBUFFER_VERSION;
  parameters.cBuffers = 1;
  parameters.pBuffers = buffers;

  ULONG produced{0};
  success = success && BCRYPT_SUCCESS(BCryptKeyDerivation(
                           key, &parameters, output, static_cast<ULONG>(length),
                           &produced, 0));
  success = success && produced == static_cast<ULONG>(length);

  if (key != nullptr) {
    BCryptDestroyKey(key);
  }

  BCryptCloseAlgorithmProvider(algorithm, 0);
  return success;
}

} // namespace

namespace sourcemeta::core {

auto hkdf_extract(const KDFHash hash, const std::string_view salt,
                  const std::string_view input_key_material,
                  unsigned char *const output) -> bool {
  // The provider finalizes the pseudorandom key inside the key object and
  // never surfaces it, so the extract step alone is composed from HMAC, which
  // is itself a native primitive here
  return hkdf_extract_loop(hash, salt, input_key_material, output);
}

auto hkdf_expand(const KDFHash hash, const std::string_view pseudorandom_key,
                 const std::string_view info, unsigned char *const output,
                 const std::size_t length) -> bool {
  if (derive(hash, pseudorandom_key, {}, true, info, output, length)) {
    return true;
  }

  return hkdf_expand_loop(hash, pseudorandom_key, info, output, length);
}

auto hkdf_derive(const KDFHash hash, const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 unsigned char *const output, const std::size_t length)
    -> bool {
  if (derive(hash, input_key_material, salt, false, info, output, length)) {
    return true;
  }

  return hkdf_derive_loop(hash, input_key_material, salt, info, output, length);
}

} // namespace sourcemeta::core
