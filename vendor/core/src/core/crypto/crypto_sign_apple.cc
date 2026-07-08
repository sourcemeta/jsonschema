#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/text.h>

#include "crypto_eddsa.h"
#include "crypto_eddsa_apple.h"
#include "crypto_helpers.h"
#include "crypto_pkcs8.h"

#include <CoreFoundation/CoreFoundation.h> // CF*, kCF*
#include <Security/Security.h>             // Sec*, kSec*

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

// The parsed key keeps the platform key object alive for reuse. The Edwards
// curves have no Security framework primitive, so they keep the raw seed and
// sign through CryptoKit or the reference implementation
struct PrivateKey::Internal {
  PrivateKey::Type kind;
  SecKeyRef key;
  std::size_t field_bytes;
  std::string edwards_seed;
  EdwardsCurve edwards_curve;
  bool rsa_pss_restricted{false};
};

} // namespace sourcemeta::core

namespace {

auto to_sec_key_pkcs1_v15_algorithm(
    const sourcemeta::core::SignatureHashFunction hash) noexcept
    -> SecKeyAlgorithm {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256;
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA384;
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA512;
  }

  std::unreachable();
}

// These algorithm variants fix the salt length to the hash function output,
// which is exactly what RFC 7518 Section 3.5 requires
auto to_sec_key_pss_algorithm(
    const sourcemeta::core::SignatureHashFunction hash) noexcept
    -> SecKeyAlgorithm {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return kSecKeyAlgorithmRSASignatureMessagePSSSHA256;
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return kSecKeyAlgorithmRSASignatureMessagePSSSHA384;
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return kSecKeyAlgorithmRSASignatureMessagePSSSHA512;
  }

  std::unreachable();
}

auto to_sec_key_ecdsa_algorithm(
    const sourcemeta::core::SignatureHashFunction hash) noexcept
    -> SecKeyAlgorithm {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return kSecKeyAlgorithmECDSASignatureMessageX962SHA256;
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return kSecKeyAlgorithmECDSASignatureMessageX962SHA384;
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return kSecKeyAlgorithmECDSASignatureMessageX962SHA512;
  }

  std::unreachable();
}

auto make_data(const std::string_view value) -> CFDataRef {
  return CFDataCreate(kCFAllocatorDefault,
                      reinterpret_cast<const UInt8 *>(value.data()),
                      static_cast<CFIndex>(value.size()));
}

auto native_private_key(const void *type, const std::string_view key)
    -> SecKeyRef {
  auto key_data{make_data(key)};
  if (key_data == nullptr) {
    return nullptr;
  }

  std::array<const void *, 2> attribute_keys{
      {kSecAttrKeyType, kSecAttrKeyClass}};
  std::array<const void *, 2> attribute_values{{type, kSecAttrKeyClassPrivate}};
  auto attributes{CFDictionaryCreate(
      kCFAllocatorDefault, attribute_keys.data(), attribute_values.data(),
      static_cast<CFIndex>(attribute_keys.size()),
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
  if (attributes == nullptr) {
    CFRelease(key_data);
    return nullptr;
  }

  auto key_reference{SecKeyCreateWithData(key_data, attributes, nullptr)};
  CFRelease(attributes);
  CFRelease(key_data);
  return key_reference;
}

// The platform expects an elliptic curve private key as the X9.63 public point
// followed by the private scalar, both recovered from the SEC1 ECPrivateKey
auto native_ec_private_key(const std::string_view sec1,
                           const std::size_t field_bytes) -> SecKeyRef {
  const auto sequence{sourcemeta::core::der_read(sec1)};
  if (!sequence.has_value() || sequence->tag != 0x30) {
    return nullptr;
  }

  const auto version{sourcemeta::core::der_read(sequence->content)};
  if (!version.has_value() || version->tag != 0x02) {
    return nullptr;
  }

  const auto scalar{sourcemeta::core::der_read(version->rest)};
  if (!scalar.has_value() || scalar->tag != 0x04) {
    return nullptr;
  }

  std::string_view point;
  auto rest{scalar->rest};
  while (!rest.empty()) {
    const auto element{sourcemeta::core::der_read(rest)};
    if (!element.has_value()) {
      return nullptr;
    }

    if (element->tag == 0xa1) {
      const auto bit_string{sourcemeta::core::der_read(element->content)};
      if (!bit_string.has_value() || bit_string->tag != 0x03 ||
          bit_string->content.empty()) {
        return nullptr;
      }

      // The BIT STRING opens with an unused-bits count, then the point
      point = bit_string->content.substr(1);
      break;
    }

    rest = element->rest;
  }

  const auto stripped_scalar{
      sourcemeta::core::strip_left(scalar->content, '\x00')};
  if (point.empty() || stripped_scalar.empty() ||
      stripped_scalar.size() > field_bytes) {
    return nullptr;
  }

  std::string data{point};
  data.append(sourcemeta::core::pad_left(stripped_scalar, field_bytes, '\x00'));
  return native_private_key(kSecAttrKeyTypeECSECPrimeRandom, data);
}

// The platform expects the X9.63 public point followed by the private scalar,
// built here directly from the caller-provided components
auto native_ec_private_key_components(const std::size_t field_bytes,
                                      const std::string_view scalar,
                                      const std::string_view coordinate_x,
                                      const std::string_view coordinate_y)
    -> SecKeyRef {
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  const auto stripped_scalar{sourcemeta::core::strip_left(scalar, '\x00')};
  if (stripped_scalar.empty() || stripped_x.size() > field_bytes ||
      stripped_y.size() > field_bytes || stripped_scalar.size() > field_bytes) {
    return nullptr;
  }

  std::string data;
  data.push_back('\x04');
  data.append(sourcemeta::core::pad_left(stripped_x, field_bytes, '\x00'));
  data.append(sourcemeta::core::pad_left(stripped_y, field_bytes, '\x00'));
  data.append(sourcemeta::core::pad_left(stripped_scalar, field_bytes, '\x00'));
  return native_private_key(kSecAttrKeyTypeECSECPrimeRandom, data);
}

auto sign_with_algorithm(SecKeyRef key, const SecKeyAlgorithm algorithm,
                         const std::string_view message)
    -> std::optional<std::string> {
  auto message_data{make_data(message)};
  if (message_data == nullptr) {
    return std::nullopt;
  }

  auto signature{SecKeyCreateSignature(key, algorithm, message_data, nullptr)};
  CFRelease(message_data);
  if (signature == nullptr) {
    return std::nullopt;
  }

  std::string result{
      reinterpret_cast<const char *>(CFDataGetBytePtr(signature)),
      static_cast<std::size_t>(CFDataGetLength(signature))};
  CFRelease(signature);
  return result;
}

// Convert the X9.62 DER signature that the platform produces into the raw
// fixed-width R || S concatenation that JWS mandates (RFC 7518 Section 3.4)
auto encode_ecdsa_signature(const std::string_view der,
                            const std::size_t field_bytes)
    -> std::optional<std::string> {
  const auto sequence{sourcemeta::core::der_read(der)};
  if (!sequence.has_value() || sequence->tag != 0x30) {
    return std::nullopt;
  }

  const auto r{sourcemeta::core::der_read(sequence->content)};
  if (!r.has_value() || r->tag != 0x02) {
    return std::nullopt;
  }

  const auto s{sourcemeta::core::der_read(r->rest)};
  if (!s.has_value() || s->tag != 0x02) {
    return std::nullopt;
  }

  std::string raw;
  raw.append(sourcemeta::core::pad_left(
      sourcemeta::core::strip_left(r->content, '\x00'), field_bytes, '\x00'));
  raw.append(sourcemeta::core::pad_left(
      sourcemeta::core::strip_left(s->content, '\x00'), field_bytes, '\x00'));
  return raw;
}

} // namespace

namespace sourcemeta::core {

PrivateKey::PrivateKey(Internal *internal) noexcept : internal_{internal} {}

PrivateKey::~PrivateKey() {
  if (internal_ != nullptr) {
    if (internal_->key != nullptr) {
      CFRelease(internal_->key);
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
        CFRelease(internal_->key);
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
      auto *key{native_private_key(kSecAttrKeyTypeRSA, parsed->key)};
      if (key == nullptr) {
        return std::nullopt;
      }

      return PrivateKey{new PrivateKey::Internal{
          .kind = PrivateKey::Type::RSA,
          .key = key,
          .field_bytes = 0,
          .edwards_seed = {},
          .edwards_curve = {},
          .rsa_pss_restricted = parsed->rsa_pss_restricted}};
    }
    case PKCS8KeyKind::EllipticCurve: {
      const auto field_bytes{curve_field_bytes(parsed->curve)};
      auto *key{native_ec_private_key(parsed->key, field_bytes)};
      if (key == nullptr) {
        return std::nullopt;
      }

      return PrivateKey{
          new PrivateKey::Internal{.kind = PrivateKey::Type::EllipticCurve,
                                   .key = key,
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
  auto *key{native_ec_private_key_components(field_bytes, scalar, coordinate_x,
                                             coordinate_y)};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PrivateKey{
      new PrivateKey::Internal{.kind = PrivateKey::Type::EllipticCurve,
                               .key = key,
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

  return sign_with_algorithm(internal->key,
                             to_sec_key_pkcs1_v15_algorithm(hash), message);
}

auto rsassa_pss_sign(const PrivateKey &key, const SignatureHashFunction hash,
                     const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA) {
    return std::nullopt;
  }

  return sign_with_algorithm(internal->key, to_sec_key_pss_algorithm(hash),
                             message);
}

auto ecdsa_sign(const PrivateKey &key, const SignatureHashFunction hash,
                const std::string_view message) -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr ||
      internal->kind != PrivateKey::Type::EllipticCurve) {
    return std::nullopt;
  }

  const auto der{sign_with_algorithm(
      internal->key, to_sec_key_ecdsa_algorithm(hash), message)};
  if (!der.has_value()) {
    return std::nullopt;
  }

  return encode_ecdsa_signature(der.value(), internal->field_bytes);
}

auto eddsa_sign(const PrivateKey &key, const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::Edwards) {
    return std::nullopt;
  }

  switch (internal->edwards_curve) {
    case EdwardsCurve::Ed25519: {
      std::string signature(64, '\x00');
      if (!sourcemeta_core_eddsa_ed25519_sign_cryptokit(
              reinterpret_cast<const unsigned char *>(
                  internal->edwards_seed.data()),
              internal->edwards_seed.size(),
              reinterpret_cast<const unsigned char *>(message.data()),
              message.size(),
              reinterpret_cast<unsigned char *>(signature.data()))) {
        return std::nullopt;
      }

      return signature;
    }
    case EdwardsCurve::Ed448:
      return edwards448_sign(internal->edwards_seed, message);
  }

  std::unreachable();
}

} // namespace sourcemeta::core
