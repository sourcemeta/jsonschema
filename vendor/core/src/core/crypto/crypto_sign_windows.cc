#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/text.h>

#include "crypto_eddsa.h"
#include "crypto_helpers.h"
#include "crypto_pkcs8.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG, LPCWSTR

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <array>       // std::array
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
// raw seed and sign through the reference implementation
struct PrivateKey::Internal {
  PrivateKey::Type kind;
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
  std::size_t field_bytes;
  std::string edwards_seed;
  EdwardsCurve edwards_curve;
  bool rsa_pss_restricted{false};
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

auto to_ecc_private_magic(const sourcemeta::core::EllipticCurve curve) noexcept
    -> ULONG {
  switch (curve) {
    case sourcemeta::core::EllipticCurve::P256:
      return BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    case sourcemeta::core::EllipticCurve::P384:
      return BCRYPT_ECDSA_PRIVATE_P384_MAGIC;
    case sourcemeta::core::EllipticCurve::P521:
      return BCRYPT_ECDSA_PRIVATE_P521_MAGIC;
  }

  std::unreachable();
}

struct KeyPair {
  BCRYPT_ALG_HANDLE algorithm;
  BCRYPT_KEY_HANDLE key;
};

auto import_key_pair(const LPCWSTR algorithm_id, const LPCWSTR blob_type,
                     const std::string_view blob) -> KeyPair {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptOpenAlgorithmProvider(&algorithm, algorithm_id, nullptr, 0))) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  if (!BCRYPT_SUCCESS(BCryptImportKeyPair(
          algorithm, nullptr, blob_type, &key,
          reinterpret_cast<unsigned char *>(const_cast<char *>(blob.data())),
          static_cast<ULONG>(blob.size()), 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return {.algorithm = nullptr, .key = nullptr};
  }

  return {.algorithm = algorithm, .key = key};
}

// Build a BCRYPT_RSAFULLPRIVATE_BLOB from the PKCS#1 RSAPrivateKey components
auto native_rsa_private_key(const std::string_view rsa_private_key) -> KeyPair {
  const auto sequence{sourcemeta::core::der_read(rsa_private_key)};
  if (!sequence.has_value() || sequence->tag != 0x30) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  std::array<std::string_view, 8> fields{};
  auto rest{sequence->content};
  // Read the version, then modulus, publicExponent, privateExponent, prime1,
  // prime2, exponent1, exponent2, coefficient. Only the two-prime form (version
  // zero, RFC 8017 Appendix A.1.2) maps onto the full private key blob, so a
  // multi-prime key with trailing otherPrimeInfos is rejected
  const auto version{sourcemeta::core::der_read(rest)};
  if (!version.has_value() || version->tag != 0x02 ||
      !sourcemeta::core::strip_left(version->content, '\x00').empty()) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  rest = version->rest;
  for (auto &field : fields) {
    const auto element{sourcemeta::core::der_read(rest)};
    if (!element.has_value() || element->tag != 0x02) {
      return {.algorithm = nullptr, .key = nullptr};
    }

    field = sourcemeta::core::strip_left(element->content, '\x00');
    // Every field feeds a blob length or padding width, so oversize input is
    // rejected before it drives a cast or an allocation
    if (field.size() > sourcemeta::core::MAXIMUM_KEY_BYTES) {
      return {.algorithm = nullptr, .key = nullptr};
    }

    rest = element->rest;
  }

  const auto modulus{fields[0]};
  const auto public_exponent{fields[1]};
  const auto private_exponent{fields[2]};
  const auto prime1{fields[3]};
  const auto prime2{fields[4]};
  const auto exponent1{fields[5]};
  const auto exponent2{fields[6]};
  const auto coefficient{fields[7]};
  // Bound the key size before the sizes drive allocations and are cast to the
  // fixed-width blob length fields, matching the public-key parsing limit
  if (modulus.empty() || modulus.size() > sourcemeta::core::MAXIMUM_KEY_BYTES) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  const auto modulus_bytes{modulus.size()};
  const auto prime_bytes{prime1.size() > prime2.size() ? prime1.size()
                                                       : prime2.size()};
  const auto modulus_bit_length{
      (modulus_bytes * 8u) - static_cast<std::size_t>(std::countl_zero(
                                 static_cast<std::uint8_t>(modulus.front())))};

  BCRYPT_RSAKEY_BLOB header{};
  header.Magic = BCRYPT_RSAFULLPRIVATE_MAGIC;
  header.BitLength = static_cast<ULONG>(modulus_bit_length);
  header.cbPublicExp = static_cast<ULONG>(public_exponent.size());
  header.cbModulus = static_cast<ULONG>(modulus_bytes);
  header.cbPrime1 = static_cast<ULONG>(prime_bytes);
  header.cbPrime2 = static_cast<ULONG>(prime_bytes);

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(public_exponent);
  blob.append(modulus);
  blob.append(sourcemeta::core::pad_left(prime1, prime_bytes, '\x00'));
  blob.append(sourcemeta::core::pad_left(prime2, prime_bytes, '\x00'));
  blob.append(sourcemeta::core::pad_left(exponent1, prime_bytes, '\x00'));
  blob.append(sourcemeta::core::pad_left(exponent2, prime_bytes, '\x00'));
  blob.append(sourcemeta::core::pad_left(coefficient, prime_bytes, '\x00'));
  blob.append(
      sourcemeta::core::pad_left(private_exponent, modulus_bytes, '\x00'));
  return import_key_pair(BCRYPT_RSA_ALGORITHM, BCRYPT_RSAFULLPRIVATE_BLOB,
                         blob);
}

// Build a BCRYPT_ECCPRIVATE_BLOB from the SEC1 ECPrivateKey scalar and point
auto native_ec_private_key(const sourcemeta::core::EllipticCurve curve,
                           const std::string_view sec1,
                           const std::size_t field_bytes) -> KeyPair {
  const auto sequence{sourcemeta::core::der_read(sec1)};
  if (!sequence.has_value() || sequence->tag != 0x30) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  const auto version{sourcemeta::core::der_read(sequence->content)};
  if (!version.has_value() || version->tag != 0x02) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  const auto scalar{sourcemeta::core::der_read(version->rest)};
  if (!scalar.has_value() || scalar->tag != 0x04) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  std::string_view point;
  auto rest{scalar->rest};
  while (!rest.empty()) {
    const auto element{sourcemeta::core::der_read(rest)};
    if (!element.has_value()) {
      return {.algorithm = nullptr, .key = nullptr};
    }

    if (element->tag == 0xa1) {
      const auto bit_string{sourcemeta::core::der_read(element->content)};
      if (!bit_string.has_value() || bit_string->tag != 0x03 ||
          bit_string->content.size() != (2 * field_bytes) + 2) {
        return {.algorithm = nullptr, .key = nullptr};
      }

      // Skip the unused-bits count and the uncompressed point lead byte
      point = bit_string->content.substr(2);
      break;
    }

    rest = element->rest;
  }

  const auto stripped_scalar{
      sourcemeta::core::strip_left(scalar->content, '\x00')};
  if (point.size() != 2 * field_bytes || stripped_scalar.empty() ||
      stripped_scalar.size() > field_bytes) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  BCRYPT_ECCKEY_BLOB header{};
  header.dwMagic = to_ecc_private_magic(curve);
  header.cbKey = static_cast<ULONG>(field_bytes);

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(point);
  blob.append(sourcemeta::core::pad_left(stripped_scalar, field_bytes, '\x00'));
  return import_key_pair(to_ecdsa_algorithm(curve), BCRYPT_ECCPRIVATE_BLOB,
                         blob);
}

// Build a BCRYPT_ECCPRIVATE_BLOB directly from the caller-provided components
auto native_ec_private_key_components(
    const sourcemeta::core::EllipticCurve curve, const std::size_t field_bytes,
    const std::string_view scalar, const std::string_view coordinate_x,
    const std::string_view coordinate_y) -> KeyPair {
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  const auto stripped_scalar{sourcemeta::core::strip_left(scalar, '\x00')};
  if (stripped_scalar.empty() || stripped_x.size() > field_bytes ||
      stripped_y.size() > field_bytes || stripped_scalar.size() > field_bytes) {
    return {.algorithm = nullptr, .key = nullptr};
  }

  BCRYPT_ECCKEY_BLOB header{};
  header.dwMagic = to_ecc_private_magic(curve);
  header.cbKey = static_cast<ULONG>(field_bytes);

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(sourcemeta::core::pad_left(stripped_x, field_bytes, '\x00'));
  blob.append(sourcemeta::core::pad_left(stripped_y, field_bytes, '\x00'));
  blob.append(sourcemeta::core::pad_left(stripped_scalar, field_bytes, '\x00'));
  return import_key_pair(to_ecdsa_algorithm(curve), BCRYPT_ECCPRIVATE_BLOB,
                         blob);
}

auto sign_hash(BCRYPT_KEY_HANDLE key, void *padding,
               const std::string_view digest, const ULONG flags)
    -> std::optional<std::string> {
  ULONG length{0};
  auto *digest_data{
      reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data()))};
  if (!BCRYPT_SUCCESS(BCryptSignHash(key, padding, digest_data,
                                     static_cast<ULONG>(digest.size()), nullptr,
                                     0, &length, flags))) {
    return std::nullopt;
  }

  std::string signature(length, '\x00');
  if (!BCRYPT_SUCCESS(BCryptSignHash(
          key, padding, digest_data, static_cast<ULONG>(digest.size()),
          reinterpret_cast<unsigned char *>(signature.data()), length, &length,
          flags))) {
    return std::nullopt;
  }

  signature.resize(length);
  return signature;
}

} // namespace

namespace sourcemeta::core {

PrivateKey::PrivateKey(Internal *internal) noexcept : internal_{internal} {}

PrivateKey::~PrivateKey() {
  if (internal_ != nullptr) {
    if (internal_->key != nullptr) {
      BCryptDestroyKey(internal_->key);
    }

    if (internal_->algorithm != nullptr) {
      BCryptCloseAlgorithmProvider(internal_->algorithm, 0);
    }

    secure_zero(internal_->edwards_seed);
    delete internal_;
  }
}

PrivateKey::PrivateKey(PrivateKey &&other) noexcept
    : internal_{other.internal_} {
  other.internal_ = nullptr;
}

auto PrivateKey::operator=(PrivateKey &&other) noexcept -> PrivateKey & {
  if (this != &other) {
    if (internal_ != nullptr) {
      if (internal_->key != nullptr) {
        BCryptDestroyKey(internal_->key);
      }

      if (internal_->algorithm != nullptr) {
        BCryptCloseAlgorithmProvider(internal_->algorithm, 0);
      }

      secure_zero(internal_->edwards_seed);
      delete internal_;
    }

    internal_ = other.internal_;
    other.internal_ = nullptr;
  }

  return *this;
}

auto PrivateKey::type() const noexcept -> Type {
  // A moved-from key holds no state, so reading its kind is a use-after-move
  assert(internal_ != nullptr);
  return internal_->kind;
}

auto make_private_key(const std::string_view pem) -> std::optional<PrivateKey> {
  auto der{pem_to_der(pem)};
  if (!der.has_value()) {
    return std::nullopt;
  }

  // The decoded PKCS#8 holds the whole private key, so it is wiped on return
  const SecureScope der_scope{der.value()};
  const auto parsed{parse_pkcs8(der.value())};
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (parsed->kind) {
    case PKCS8KeyKind::RSA: {
      const auto pair{native_rsa_private_key(parsed->key)};
      if (pair.key == nullptr) {
        return std::nullopt;
      }

      return PrivateKey{new PrivateKey::Internal{
          .kind = PrivateKey::Type::RSA,
          .algorithm = pair.algorithm,
          .key = pair.key,
          .field_bytes = 0,
          .edwards_seed = {},
          .edwards_curve = {},
          .rsa_pss_restricted = parsed->rsa_pss_restricted}};
    }
    case PKCS8KeyKind::EllipticCurve: {
      const auto field_bytes{curve_field_bytes(parsed->curve)};
      const auto pair{
          native_ec_private_key(parsed->curve, parsed->key, field_bytes)};
      if (pair.key == nullptr) {
        return std::nullopt;
      }

      return PrivateKey{
          new PrivateKey::Internal{.kind = PrivateKey::Type::EllipticCurve,
                                   .algorithm = pair.algorithm,
                                   .key = pair.key,
                                   .field_bytes = field_bytes,
                                   .edwards_seed = {},
                                   .edwards_curve = {}}};
    }
    case PKCS8KeyKind::Edwards: {
      const auto seed{der_read(parsed->key)};
      if (!seed.has_value() || seed->tag != 0x04 ||
          seed->content.size() !=
              eddsa_public_key_bytes(parsed->edwards_curve)) {
        return std::nullopt;
      }

      return PrivateKey{
          new PrivateKey::Internal{.kind = PrivateKey::Type::Edwards,
                                   .algorithm = nullptr,
                                   .key = nullptr,
                                   .field_bytes = 0,
                                   .edwards_seed = std::string{seed->content},
                                   .edwards_curve = parsed->edwards_curve}};
    }
  }

  std::unreachable();
}

auto make_ec_private_key(const EllipticCurve curve,
                         const std::string_view scalar,
                         const std::string_view coordinate_x,
                         const std::string_view coordinate_y)
    -> std::optional<PrivateKey> {
  if (!ec_private_scalar_in_range(scalar, curve)) {
    return std::nullopt;
  }

  const auto field_bytes{curve_field_bytes(curve)};
  const auto pair{native_ec_private_key_components(curve, field_bytes, scalar,
                                                   coordinate_x, coordinate_y)};
  if (pair.key == nullptr) {
    return std::nullopt;
  }

  return PrivateKey{
      new PrivateKey::Internal{.kind = PrivateKey::Type::EllipticCurve,
                               .algorithm = pair.algorithm,
                               .key = pair.key,
                               .field_bytes = field_bytes,
                               .edwards_seed = {},
                               .edwards_curve = {}}};
}

auto make_edwards_private_key(const EdwardsCurve curve,
                              const std::string_view seed)
    -> std::optional<PrivateKey> {
  if (seed.size() != eddsa_public_key_bytes(curve)) {
    return std::nullopt;
  }

  return PrivateKey{new PrivateKey::Internal{.kind = PrivateKey::Type::Edwards,
                                             .algorithm = nullptr,
                                             .key = nullptr,
                                             .field_bytes = 0,
                                             .edwards_seed = std::string{seed},
                                             .edwards_curve = curve}};
}

auto rsassa_pkcs1_v15_sign(const PrivateKey &key,
                           const SignatureHashFunction hash,
                           const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA) {
    return std::nullopt;
  }

  // An id-RSASSA-PSS key is restricted to PSS and must not sign PKCS1v15
  if (internal->rsa_pss_restricted) {
    return std::nullopt;
  }

  const auto digest{digest_message(hash, message)};
  BCRYPT_PKCS1_PADDING_INFO padding{};
  padding.pszAlgId = to_cng_algorithm(hash);
  return sign_hash(internal->key, &padding, digest, BCRYPT_PAD_PKCS1);
}

auto rsassa_pss_sign(const PrivateKey &key, const SignatureHashFunction hash,
                     const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA) {
    return std::nullopt;
  }

  const auto digest{digest_message(hash, message)};
  BCRYPT_PSS_PADDING_INFO padding{};
  padding.pszAlgId = to_cng_algorithm(hash);
  padding.cbSalt = static_cast<ULONG>(digest.size());
  return sign_hash(internal->key, &padding, digest, BCRYPT_PAD_PSS);
}

auto ecdsa_sign(const PrivateKey &key, const SignatureHashFunction hash,
                const std::string_view message) -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr ||
      internal->kind != PrivateKey::Type::EllipticCurve) {
    return std::nullopt;
  }

  // CNG produces the raw fixed-width R || S concatenation that JWS mandates
  const auto digest{digest_message(hash, message)};
  return sign_hash(internal->key, nullptr, digest, 0);
}

auto eddsa_sign(const PrivateKey &key, const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::Edwards) {
    return std::nullopt;
  }

  switch (internal->edwards_curve) {
    case EdwardsCurve::Ed25519:
      return edwards25519_sign(internal->edwards_seed, message);
    case EdwardsCurve::Ed448:
      return edwards448_sign(internal->edwards_seed, message);
  }

  std::unreachable();
}

} // namespace sourcemeta::core
