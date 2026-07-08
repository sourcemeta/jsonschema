#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_eddsa.h"
#include "crypto_helpers.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG, LPCWSTR

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <bit>         // std::countl_zero
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <cstring>     // std::memcpy
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

// The parsed key keeps both the algorithm provider and the imported key handle
// alive for reuse. The Edwards curves have no CNG primitive, so they keep the
// raw encoded point and verify through the reference implementation
struct PublicKey::Internal {
  PublicKey::Type kind;
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
  std::size_t field_bytes;
  std::string modulus;
  std::string edwards_point;
  EdwardsCurve edwards_curve;
};

} // namespace sourcemeta::core

namespace {

auto to_cng_algorithm(
    const sourcemeta::core::SignatureHashFunction hash) noexcept -> LPCWSTR {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return BCRYPT_SHA256_ALGORITHM;
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return BCRYPT_SHA384_ALGORITHM;
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return BCRYPT_SHA512_ALGORITHM;
  }

  std::unreachable();
}

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

struct KeyPair {
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
};

auto native_rsa_key(const std::string_view modulus,
                    const std::string_view exponent) -> KeyPair {
  const auto modulus_bit_length{
      (modulus.size() * 8u) - static_cast<std::size_t>(std::countl_zero(
                                  static_cast<std::uint8_t>(modulus.front())))};

  BCRYPT_RSAKEY_BLOB header{};
  header.Magic = BCRYPT_RSAPUBLIC_MAGIC;
  header.BitLength = static_cast<ULONG>(modulus_bit_length);
  header.cbPublicExp = static_cast<ULONG>(exponent.size());
  header.cbModulus = static_cast<ULONG>(modulus.size());
  header.cbPrime1 = 0;
  header.cbPrime2 = 0;

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(exponent);
  blob.append(modulus);

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0))) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPUBLIC_BLOB, &key,
                              reinterpret_cast<unsigned char *>(blob.data()),
                              static_cast<ULONG>(blob.size()), 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return {.algorithm = nullptr, .key = nullptr};
  }

  return {.algorithm = algorithm, .key = key};
}

auto native_ec_key(const sourcemeta::core::EllipticCurve curve,
                   const std::string_view coordinate_x,
                   const std::string_view coordinate_y, const std::size_t width)
    -> KeyPair {
  BCRYPT_ECCKEY_BLOB header{};
  header.dwMagic = to_ecc_public_magic(curve);
  header.cbKey = static_cast<ULONG>(width);

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(sourcemeta::core::pad_left(coordinate_x, width, '\x00'));
  blob.append(sourcemeta::core::pad_left(coordinate_y, width, '\x00'));

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, to_ecdsa_algorithm(curve), nullptr, 0))) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptImportKeyPair(algorithm, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key,
                              reinterpret_cast<unsigned char *>(blob.data()),
                              static_cast<ULONG>(blob.size()), 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return {.algorithm = nullptr, .key = nullptr};
  }

  return {.algorithm = algorithm, .key = key};
}

auto verify_rsa(BCRYPT_KEY_HANDLE key,
                const sourcemeta::core::SignatureHashFunction hash,
                const std::string_view message,
                const std::string_view signature, const bool probabilistic)
    -> bool {
  const auto digest{sourcemeta::core::digest_message(hash, message)};

  if (probabilistic) {
    // The digest-length salt is what RFC 7518 Section 3.5 requires
    BCRYPT_PSS_PADDING_INFO padding{};
    padding.pszAlgId = to_cng_algorithm(hash);
    padding.cbSalt = static_cast<ULONG>(digest.size());
    return BCRYPT_SUCCESS(BCryptVerifySignature(
        key, &padding,
        reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data())),
        static_cast<ULONG>(digest.size()),
        reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
        static_cast<ULONG>(signature.size()), BCRYPT_PAD_PSS));
  }

  BCRYPT_PKCS1_PADDING_INFO padding{};
  padding.pszAlgId = to_cng_algorithm(hash);
  return BCRYPT_SUCCESS(BCryptVerifySignature(
      key, &padding,
      reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data())),
      static_cast<ULONG>(digest.size()),
      reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
      static_cast<ULONG>(signature.size()), BCRYPT_PAD_PKCS1));
}

} // namespace

namespace sourcemeta::core {

PublicKey::PublicKey(Internal *internal) noexcept : internal_{internal} {}

PublicKey::~PublicKey() {
  if (internal_ != nullptr) {
    if (internal_->key != nullptr) {
      BCryptDestroyKey(internal_->key);
    }

    if (internal_->algorithm != nullptr) {
      BCryptCloseAlgorithmProvider(internal_->algorithm, 0);
    }

    delete internal_;
  }
}

PublicKey::PublicKey(PublicKey &&other) noexcept : internal_{other.internal_} {
  other.internal_ = nullptr;
}

auto PublicKey::operator=(PublicKey &&other) noexcept -> PublicKey & {
  if (this != &other) {
    if (internal_ != nullptr) {
      if (internal_->key != nullptr) {
        BCryptDestroyKey(internal_->key);
      }

      if (internal_->algorithm != nullptr) {
        BCryptCloseAlgorithmProvider(internal_->algorithm, 0);
      }

      delete internal_;
    }

    internal_ = other.internal_;
    other.internal_ = nullptr;
  }

  return *this;
}

auto PublicKey::type() const noexcept -> Type {
  // A moved-from key holds no state, so reading its kind is a use-after-move
  assert(internal_ != nullptr);
  return internal_->kind;
}

auto make_rsa_public_key(const std::string_view modulus,
                         const std::string_view exponent)
    -> std::optional<PublicKey> {
  auto stripped_modulus{std::string{strip_left(modulus, '\x00')}};
  const auto stripped_exponent{strip_left(exponent, '\x00')};
  if (stripped_modulus.empty() || stripped_exponent.empty() ||
      stripped_modulus.size() > MAXIMUM_KEY_BYTES ||
      stripped_exponent.size() > MAXIMUM_KEY_BYTES ||
      !rsa_public_exponent_acceptable(exponent, modulus)) {
    return std::nullopt;
  }

  const auto pair{native_rsa_key(stripped_modulus, stripped_exponent)};
  if (pair.key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::RSA,
                              .algorithm = pair.algorithm,
                              .key = pair.key,
                              .field_bytes = 0,
                              .modulus = std::move(stripped_modulus),
                              .edwards_point = {},
                              .edwards_curve = {}}};
}

auto make_ec_public_key(const EllipticCurve curve,
                        const std::string_view coordinate_x,
                        const std::string_view coordinate_y)
    -> std::optional<PublicKey> {
  const auto width{curve_field_bytes(curve)};
  const auto stripped_x{strip_left(coordinate_x, '\x00')};
  const auto stripped_y{strip_left(coordinate_y, '\x00')};
  if (stripped_x.size() > width || stripped_y.size() > width) {
    return std::nullopt;
  }

  const auto pair{native_ec_key(curve, stripped_x, stripped_y, width)};
  if (pair.key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::EllipticCurve,
                              .algorithm = pair.algorithm,
                              .key = pair.key,
                              .field_bytes = width,
                              .modulus = {},
                              .edwards_point = {},
                              .edwards_curve = {}}};
}

auto make_eddsa_public_key(const EdwardsCurve curve,
                           const std::string_view public_key)
    -> std::optional<PublicKey> {
  if (public_key.size() != eddsa_public_key_bytes(curve)) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::Edwards,
                              .algorithm = nullptr,
                              .key = nullptr,
                              .field_bytes = 0,
                              .modulus = {},
                              .edwards_point = std::string{public_key},
                              .edwards_curve = curve}};
}

auto rsassa_pkcs1_v15_verify(const PublicKey &key,
                             const SignatureHashFunction hash,
                             const std::string_view message,
                             const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA ||
      !rsa_signature_in_range(signature, internal->modulus)) {
    return false;
  }

  return verify_rsa(internal->key, hash, message, signature, false);
}

auto rsassa_pss_verify(const PublicKey &key, const SignatureHashFunction hash,
                       const std::string_view message,
                       const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA ||
      !rsa_signature_in_range(signature, internal->modulus)) {
    return false;
  }

  return verify_rsa(internal->key, hash, message, signature, true);
}

auto ecdsa_verify(const PublicKey &key, const SignatureHashFunction hash,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::EllipticCurve ||
      signature.size() != internal->field_bytes * 2) {
    return false;
  }

  const auto digest{digest_message(hash, message)};

  // The CNG signature format is the raw fixed-width R || S concatenation, so
  // the input passes through unchanged
  return BCRYPT_SUCCESS(BCryptVerifySignature(
      internal->key, nullptr,
      reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data())),
      static_cast<ULONG>(digest.size()),
      reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
      static_cast<ULONG>(signature.size()), 0));
}

auto eddsa_verify(const PublicKey &key, const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::Edwards) {
    return false;
  }

  switch (internal->edwards_curve) {
    case EdwardsCurve::Ed25519:
      return edwards25519_verify(internal->edwards_point, message, signature);
    case EdwardsCurve::Ed448:
      return edwards448_verify(internal->edwards_point, message, signature);
  }

  std::unreachable();
}

} // namespace sourcemeta::core
