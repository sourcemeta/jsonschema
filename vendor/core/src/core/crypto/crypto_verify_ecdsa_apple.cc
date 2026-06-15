#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"

#include <CoreFoundation/CoreFoundation.h> // CF*, kCF*
#include <Security/Security.h>             // Sec*, kSec*

#include <array>       // std::array
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

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

auto make_ec_public_key(const sourcemeta::core::EllipticCurve curve,
                        const std::string_view coordinate_x,
                        const std::string_view coordinate_y) -> SecKeyRef {
  const auto width{sourcemeta::core::curve_field_bytes(curve)};
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  if (stripped_x.size() > width || stripped_y.size() > width) {
    return nullptr;
  }

  // The platform infers the curve from the X9.63 uncompressed point, the
  // 0x04 lead byte followed by the two fixed-width coordinates
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
  // The raw form is the two integers concatenated, while the platform
  // expects the X9.62 DER sequence of those integers
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

} // namespace

namespace sourcemeta::core {

auto ecdsa_verify(const EllipticCurve curve, const SignatureHashFunction hash,
                  const std::string_view coordinate_x,
                  const std::string_view coordinate_y,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  if (signature.size() != sourcemeta::core::curve_field_bytes(curve) * 2) {
    return false;
  }

  auto key{make_ec_public_key(curve, coordinate_x, coordinate_y)};
  if (key == nullptr) {
    return false;
  }

  const auto der_signature{encode_ecdsa_signature(signature)};
  auto message_data{make_data(message)};
  auto signature_data{make_data(der_signature)};
  auto result{false};
  if (message_data != nullptr && signature_data != nullptr) {
    result =
        SecKeyVerifySignature(key, to_sec_key_ecdsa_algorithm(hash),
                              message_data, signature_data, nullptr) == true;
  }

  if (signature_data != nullptr) {
    CFRelease(signature_data);
  }

  if (message_data != nullptr) {
    CFRelease(message_data);
  }

  CFRelease(key);
  return result;
}

} // namespace sourcemeta::core
