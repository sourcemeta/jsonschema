#include <sourcemeta/core/crypto_ecdh.h>

#include "crypto_windows.h"

#include <windows.h> // ULONG, PUCHAR, LPCWSTR
// clang-format off
#include <bcrypt.h> // BCrypt*, BCRYPT_*
// clang-format on

#include <algorithm> // std::reverse
#include <cstddef>   // std::size_t
#include <cstring>   // std::memcpy
#include <optional>  // std::optional, std::nullopt
#include <string>    // std::string
#include <utility>   // std::move

namespace sourcemeta::core {

namespace {
// The elliptic curve is recovered from the field width, since the sign backend
// stores that rather than the curve identifier
auto to_ecdh_algorithm(const std::size_t field_bytes) noexcept -> LPCWSTR {
  switch (field_bytes) {
    case 32:
      return BCRYPT_ECDH_P256_ALGORITHM;
    case 48:
      return BCRYPT_ECDH_P384_ALGORITHM;
    case 66:
      return BCRYPT_ECDH_P521_ALGORITHM;
    default:
      return nullptr;
  }
}

auto to_ecdh_private_magic(const std::size_t field_bytes) noexcept -> ULONG {
  switch (field_bytes) {
    case 32:
      return BCRYPT_ECDH_PRIVATE_P256_MAGIC;
    case 48:
      return BCRYPT_ECDH_PRIVATE_P384_MAGIC;
    default:
      return BCRYPT_ECDH_PRIVATE_P521_MAGIC;
  }
}

auto to_ecdh_public_magic(const std::size_t field_bytes) noexcept -> ULONG {
  switch (field_bytes) {
    case 32:
      return BCRYPT_ECDH_PUBLIC_P256_MAGIC;
    case 48:
      return BCRYPT_ECDH_PUBLIC_P384_MAGIC;
    default:
      return BCRYPT_ECDH_PUBLIC_P521_MAGIC;
  }
}

struct EcdhKey {
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
};

auto destroy(const EcdhKey &key) -> void {
  if (key.key != nullptr) {
    BCryptDestroyKey(key.key);
  }

  if (key.algorithm != nullptr) {
    BCryptCloseAlgorithmProvider(key.algorithm, 0);
  }
}

// The sign backend imports elliptic curve keys under the ECDSA provider, which
// cannot perform key agreement, so the key is exported and re-imported under
// the ECDH provider, rewriting the blob magic that identifies the algorithm
auto reimport_under_ecdh(const BCRYPT_KEY_HANDLE source,
                         const LPCWSTR blob_type, const LPCWSTR algorithm_id,
                         const ULONG magic) -> EcdhKey {
  ULONG length{0};
  if (!BCRYPT_SUCCESS(BCryptExportKey(source, nullptr, blob_type, nullptr, 0,
                                      &length, 0)) ||
      length < sizeof(magic)) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  std::string blob(length, '\x00');
  if (!BCRYPT_SUCCESS(BCryptExportKey(
          source, nullptr, blob_type, reinterpret_cast<PUCHAR>(blob.data()),
          static_cast<ULONG>(blob.size()), &length, 0))) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  std::memcpy(blob.data(), &magic, sizeof(magic));

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptOpenAlgorithmProvider(&algorithm, algorithm_id, nullptr, 0))) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  if (!BCRYPT_SUCCESS(BCryptImportKeyPair(algorithm, nullptr, blob_type, &key,
                                          reinterpret_cast<PUCHAR>(blob.data()),
                                          static_cast<ULONG>(blob.size()),
                                          0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return {.algorithm = nullptr, .key = nullptr};
  }

  return {.algorithm = algorithm, .key = key};
}
} // namespace

auto ecdh_derive(const PrivateKey &private_key, const PublicKey &public_key)
    -> std::optional<std::string> {
  const auto *const private_internal{private_key.internal()};
  const auto *const public_internal{public_key.internal()};
  if (private_internal == nullptr || public_internal == nullptr ||
      private_internal->kind != PrivateKey::Type::EllipticCurve ||
      public_internal->kind != PublicKey::Type::EllipticCurve ||
      private_internal->field_bytes != public_internal->field_bytes) {
    return std::nullopt;
  }

  const auto field_bytes{private_internal->field_bytes};
  const auto algorithm_id{to_ecdh_algorithm(field_bytes)};
  if (algorithm_id == nullptr) {
    return std::nullopt;
  }

  const auto private_ecdh{
      reimport_under_ecdh(private_internal->key, BCRYPT_ECCPRIVATE_BLOB,
                          algorithm_id, to_ecdh_private_magic(field_bytes))};
  if (private_ecdh.key == nullptr) {
    return std::nullopt;
  }

  const auto public_ecdh{
      reimport_under_ecdh(public_internal->key, BCRYPT_ECCPUBLIC_BLOB,
                          algorithm_id, to_ecdh_public_magic(field_bytes))};
  if (public_ecdh.key == nullptr) {
    destroy(private_ecdh);
    return std::nullopt;
  }

  std::optional<std::string> result;
  BCRYPT_SECRET_HANDLE secret{nullptr};
  if (BCRYPT_SUCCESS(BCryptSecretAgreement(private_ecdh.key, public_ecdh.key,
                                           &secret, 0))) {
    ULONG length{0};
    if (BCRYPT_SUCCESS(BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, nullptr,
                                       nullptr, 0, &length, 0))) {
      std::string secret_bytes(length, '\x00');
      ULONG written{0};
      if (BCRYPT_SUCCESS(BCryptDeriveKey(
              secret, BCRYPT_KDF_RAW_SECRET, nullptr,
              reinterpret_cast<PUCHAR>(secret_bytes.data()),
              static_cast<ULONG>(secret_bytes.size()), &written, 0)) &&
          written <= field_bytes) {
        secret_bytes.resize(written);
        // The raw secret is little-endian, so it is reversed into the
        // big-endian form the other backends return
        std::reverse(secret_bytes.begin(), secret_bytes.end());
        // Guarantee the fixed-length x coordinate the contract promises, since
        // a leading zero could otherwise shorten the platform output
        secret_bytes.insert(secret_bytes.begin(),
                            field_bytes - secret_bytes.size(), '\x00');
        result = std::move(secret_bytes);
      }
    }

    BCryptDestroySecret(secret);
  }

  destroy(public_ecdh);
  destroy(private_ecdh);
  return result;
}

} // namespace sourcemeta::core
