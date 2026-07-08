#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_der.h"
#include "crypto_eddsa.h"
#include "crypto_eddsa_apple.h"
#include "crypto_helpers.h"

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
// curves have no Security framework primitive, so they keep the raw encoded
// point and verify through CryptoKit or the reference implementation
struct PublicKey::Internal {
  PublicKey::Type kind;
  SecKeyRef key;
  std::string modulus;
  std::size_t field_bytes;
  std::string edwards_point;
  EdwardsCurve edwards_curve;
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

auto native_rsa_key(const std::string_view modulus,
                    const std::string_view exponent) -> SecKeyRef {
  // The platform expects the PKCS#1 RSAPublicKey structure, a DER sequence of
  // the modulus and exponent integers (RFC 8017 Appendix A.1.1)
  std::string body;
  sourcemeta::core::der_append_unsigned_integer(body, modulus);
  sourcemeta::core::der_append_unsigned_integer(body, exponent);
  std::string der;
  der.push_back('\x30');
  sourcemeta::core::der_append_length(der, body.size());
  der.append(body);

  auto key_data{make_data(der)};
  if (key_data == nullptr) {
    return nullptr;
  }

  std::array<const void *, 2> attribute_keys{
      {kSecAttrKeyType, kSecAttrKeyClass}};
  std::array<const void *, 2> attribute_values{
      {kSecAttrKeyTypeRSA, kSecAttrKeyClassPublic}};
  auto attributes{CFDictionaryCreate(
      kCFAllocatorDefault, attribute_keys.data(), attribute_values.data(),
      static_cast<CFIndex>(attribute_keys.size()),
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
  if (attributes == nullptr) {
    CFRelease(key_data);
    return nullptr;
  }

  auto key{SecKeyCreateWithData(key_data, attributes, nullptr)};
  CFRelease(attributes);
  CFRelease(key_data);
  return key;
}

auto native_ec_key(const sourcemeta::core::EllipticCurve curve,
                   const std::string_view coordinate_x,
                   const std::string_view coordinate_y) -> SecKeyRef {
  const auto width{sourcemeta::core::curve_field_bytes(curve)};
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  if (stripped_x.size() > width || stripped_y.size() > width) {
    return nullptr;
  }

  // The platform infers the curve from the X9.63 uncompressed point, the 0x04
  // lead byte followed by the two fixed-width coordinates
  std::string point;
  point.push_back('\x04');
  point.append(sourcemeta::core::pad_left(stripped_x, width, '\x00'));
  point.append(sourcemeta::core::pad_left(stripped_y, width, '\x00'));

  auto key_data{make_data(point)};
  if (key_data == nullptr) {
    return nullptr;
  }

  std::array<const void *, 2> attribute_keys{
      {kSecAttrKeyType, kSecAttrKeyClass}};
  std::array<const void *, 2> attribute_values{
      {kSecAttrKeyTypeECSECPrimeRandom, kSecAttrKeyClassPublic}};
  auto attributes{CFDictionaryCreate(
      kCFAllocatorDefault, attribute_keys.data(), attribute_values.data(),
      static_cast<CFIndex>(attribute_keys.size()),
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
  if (attributes == nullptr) {
    CFRelease(key_data);
    return nullptr;
  }

  auto key{SecKeyCreateWithData(key_data, attributes, nullptr)};
  CFRelease(attributes);
  CFRelease(key_data);
  return key;
}

auto encode_ecdsa_signature(const std::string_view raw_signature)
    -> std::string {
  // The raw form is the two integers concatenated, while the platform expects
  // the X9.62 DER sequence of those integers
  const auto half{raw_signature.size() / 2};
  std::string body;
  sourcemeta::core::der_append_unsigned_integer(body,
                                                raw_signature.substr(0, half));
  sourcemeta::core::der_append_unsigned_integer(body,
                                                raw_signature.substr(half));
  std::string der;
  der.push_back('\x30');
  sourcemeta::core::der_append_length(der, body.size());
  der.append(body);
  return der;
}

auto verify_with_algorithm(SecKeyRef key, const SecKeyAlgorithm algorithm,
                           const std::string_view message,
                           const std::string_view signature) -> bool {
  auto message_data{make_data(message)};
  auto signature_data{make_data(signature)};
  auto result{false};
  if (message_data != nullptr && signature_data != nullptr) {
    result = SecKeyVerifySignature(key, algorithm, message_data, signature_data,
                                   nullptr) == true;
  }

  if (signature_data != nullptr) {
    CFRelease(signature_data);
  }

  if (message_data != nullptr) {
    CFRelease(message_data);
  }

  return result;
}

} // namespace

namespace sourcemeta::core {

PublicKey::PublicKey(Internal *internal) noexcept : internal_{internal} {}

PublicKey::~PublicKey() {
  if (internal_ != nullptr) {
    if (internal_->key != nullptr) {
      CFRelease(internal_->key);
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
        CFRelease(internal_->key);
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

  auto *key{native_rsa_key(stripped_modulus, stripped_exponent)};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::RSA,
                              .key = key,
                              .modulus = std::move(stripped_modulus),
                              .field_bytes = 0,
                              .edwards_point = {},
                              .edwards_curve = {}}};
}

auto make_ec_public_key(const EllipticCurve curve,
                        const std::string_view coordinate_x,
                        const std::string_view coordinate_y)
    -> std::optional<PublicKey> {
  auto *key{native_ec_key(curve, coordinate_x, coordinate_y)};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::EllipticCurve,
                              .key = key,
                              .modulus = {},
                              .field_bytes = curve_field_bytes(curve),
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
                              .key = nullptr,
                              .modulus = {},
                              .field_bytes = 0,
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

  return verify_with_algorithm(
      internal->key, to_sec_key_pkcs1_v15_algorithm(hash), message, signature);
}

auto rsassa_pss_verify(const PublicKey &key, const SignatureHashFunction hash,
                       const std::string_view message,
                       const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA ||
      !rsa_signature_in_range(signature, internal->modulus)) {
    return false;
  }

  return verify_with_algorithm(internal->key, to_sec_key_pss_algorithm(hash),
                               message, signature);
}

auto ecdsa_verify(const PublicKey &key, const SignatureHashFunction hash,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::EllipticCurve ||
      signature.size() != internal->field_bytes * 2) {
    return false;
  }

  return verify_with_algorithm(internal->key, to_sec_key_ecdsa_algorithm(hash),
                               message, encode_ecdsa_signature(signature));
}

auto eddsa_verify(const PublicKey &key, const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::Edwards ||
      signature.size() != eddsa_signature_bytes(internal->edwards_curve)) {
    return false;
  }

  switch (internal->edwards_curve) {
    case EdwardsCurve::Ed25519:
      return sourcemeta_core_eddsa_ed25519_verify_cryptokit(
          reinterpret_cast<const unsigned char *>(
              internal->edwards_point.data()),
          internal->edwards_point.size(),
          reinterpret_cast<const unsigned char *>(message.data()),
          message.size(),
          reinterpret_cast<const unsigned char *>(signature.data()),
          signature.size());
    case EdwardsCurve::Ed448:
      return edwards448_verify(internal->edwards_point, message, signature);
  }

  std::unreachable();
}

} // namespace sourcemeta::core
