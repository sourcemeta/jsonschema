#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG, LPCWSTR

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <cstring>     // std::memcpy
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto to_ecdsa_algorithm(const sourcemeta::core::EllipticCurve curve) noexcept
    -> LPCWSTR {
  switch (curve) {
    case sourcemeta::core::EllipticCurve::P256:
      return BCRYPT_ECDSA_P256_ALGORITHM;
    case sourcemeta::core::EllipticCurve::P384:
      return BCRYPT_ECDSA_P384_ALGORITHM;
    case sourcemeta::core::EllipticCurve::P521:
      return BCRYPT_ECDSA_P521_ALGORITHM;
  }

  std::unreachable();
}

auto to_ecc_public_magic(const sourcemeta::core::EllipticCurve curve) noexcept
    -> ULONG {
  switch (curve) {
    case sourcemeta::core::EllipticCurve::P256:
      return BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    case sourcemeta::core::EllipticCurve::P384:
      return BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
    case sourcemeta::core::EllipticCurve::P521:
      return BCRYPT_ECDSA_PUBLIC_P521_MAGIC;
  }

  std::unreachable();
}

} // namespace

namespace sourcemeta::core {

auto ecdsa_verify(const EllipticCurve curve, const SignatureHashFunction hash,
                  const std::string_view coordinate_x,
                  const std::string_view coordinate_y,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto width{sourcemeta::core::curve_field_bytes(curve)};
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  if (signature.size() != width * 2 || stripped_x.size() > width ||
      stripped_y.size() > width) {
    return false;
  }

  // The public key blob is the header followed by the two fixed-width
  // coordinates
  BCRYPT_ECCKEY_BLOB header{};
  header.dwMagic = to_ecc_public_magic(curve);
  header.cbKey = static_cast<ULONG>(width);

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(sourcemeta::core::pad_left(stripped_x, width, '\x00'));
  blob.append(sourcemeta::core::pad_left(stripped_y, width, '\x00'));

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, to_ecdsa_algorithm(curve), nullptr, 0))) {
    return false;
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptImportKeyPair(algorithm, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key,
                              reinterpret_cast<unsigned char *>(blob.data()),
                              static_cast<ULONG>(blob.size()), 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return false;
  }

  const auto digest{sourcemeta::core::digest_message(hash, message)};

  // The CNG signature format is the raw fixed-width R || S concatenation, so
  // the input passes through unchanged
  const auto result{BCRYPT_SUCCESS(BCryptVerifySignature(
      key, nullptr,
      reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data())),
      static_cast<ULONG>(digest.size()),
      reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
      static_cast<ULONG>(signature.size()), 0))};

  BCryptDestroyKey(key);
  BCryptCloseAlgorithmProvider(algorithm, 0);
  return result;
}

} // namespace sourcemeta::core
