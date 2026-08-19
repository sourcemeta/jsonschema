#ifndef SOURCEMETA_CORE_CRYPTO_PKCS8_H_
#define SOURCEMETA_CORE_CRYPTO_PKCS8_H_

#include <sourcemeta/core/crypto_base64.h>
#include <sourcemeta/core/crypto_verify.h>

#include "crypto_der.h"
#include "crypto_helpers.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Decode a single-block PEM document into its DER bytes
inline auto pem_to_der(const std::string_view pem)
    -> std::optional<std::string> {
  const auto begin{pem.find("-----BEGIN")};
  if (begin == std::string_view::npos) {
    return std::nullopt;
  }

  const auto header_end{pem.find('\n', begin)};
  if (header_end == std::string_view::npos) {
    return std::nullopt;
  }

  const auto end{pem.find("-----END", header_end)};
  if (end == std::string_view::npos) {
    return std::nullopt;
  }

  std::string base64;
  for (const auto character :
       pem.substr(header_end + 1, end - header_end - 1)) {
    if (character != '\n' && character != '\r' && character != ' ' &&
        character != '\t') {
      base64.push_back(character);
    }
  }

  // The base64 body carries the whole private key, so it is wiped once decoded
  const SecureStringScope base64_scope{base64};
  return base64_decode(base64);
}

enum class PKCS8KeyKind : std::uint8_t { RSA, EllipticCurve, Edwards };

// The parsed shape of an RFC 5958 PrivateKeyInfo, where `key` views the
// algorithm specific privateKey octets that each backend parses further
struct PKCS8Key {
  PKCS8KeyKind kind;
  EllipticCurve curve;
  EdwardsCurve edwards_curve;
  std::string_view key;
  // Set when the algorithm is id-RSASSA-PSS rather than rsaEncryption, so that
  // such a key is refused for RSASSA-PKCS1-v1_5 signing (RFC 8017 Appendix A.2)
  bool rsa_pss_restricted{false};
};

// Whether a PKCS#1 RSAPrivateKey carries usable components (RFC 8017 Appendix
// A.1.2). The modulus and both exponents are decoded as canonical non-negative
// DER INTEGERs, so a negative or non-canonically encoded one cannot be silently
// reinterpreted as a different positive number and signed with, and the public
// exponent is range-checked per RFC 8017 Section 3.1. The version is required
// to be present as an INTEGER but its value is left to the backends, which
// differ on whether they accept the multi-prime form. Checked here rather than
// in each backend, since only the reference one reads these components itself
// and the rest hand the blob straight to a platform that does not apply the
// rule
inline auto rsa_private_key_acceptable(const std::string_view key) -> bool {
  // A canonical RSAPrivateKey is exactly one SEQUENCE, so bytes trailing it
  // mark a malformed encoding (X.690 Section 10.1), the same rule the enclosing
  // PrivateKeyInfo is held to
  const auto sequence{der_read(key)};
  if (!sequence.has_value() || sequence->tag != 0x30 ||
      !sequence->rest.empty()) {
    return false;
  }

  const auto version{der_read(sequence->content)};
  if (!version.has_value() || version->tag != 0x02) {
    return false;
  }

  const auto modulus{der_read(version->rest)};
  if (!modulus.has_value() || modulus->tag != 0x02) {
    return false;
  }

  const auto public_exponent{der_read(modulus->rest)};
  if (!public_exponent.has_value() || public_exponent->tag != 0x02) {
    return false;
  }

  const auto private_exponent{der_read(public_exponent->rest)};
  if (!private_exponent.has_value() || private_exponent->tag != 0x02) {
    return false;
  }

  const auto modulus_value{der_unsigned_integer(modulus->content)};
  const auto public_exponent_value{
      der_unsigned_integer(public_exponent->content)};
  const auto private_exponent_value{
      der_unsigned_integer(private_exponent->content)};
  return modulus_value.has_value() && public_exponent_value.has_value() &&
         private_exponent_value.has_value() && !modulus_value->empty() &&
         !private_exponent_value->empty() &&
         modulus_value->size() <= MAXIMUM_KEY_BYTES &&
         private_exponent_value->size() <= MAXIMUM_KEY_BYTES &&
         rsa_public_exponent_acceptable(public_exponent_value.value(),
                                        modulus_value.value());
}

// Parse an RFC 5958 PrivateKeyInfo, identifying the algorithm from its object
// identifier and returning the raw privateKey octets
inline auto parse_pkcs8(const std::string_view der) -> std::optional<PKCS8Key> {
  const auto outer{der_read(der)};
  // A canonical PrivateKeyInfo is exactly one SEQUENCE, so bytes trailing the
  // outer structure mark a malformed encoding (X.690 Section 10.1)
  if (!outer.has_value() || outer->tag != 0x30 || !outer->rest.empty()) {
    return std::nullopt;
  }

  // Only the two defined OneAsymmetricKey versions are accepted (RFC 5958
  // Section 2): v1 for the bare private key and v2 which may append a public
  // key, both encoded as a single-octet INTEGER
  const auto version{der_read(outer->content)};
  if (!version.has_value() || version->tag != 0x02 ||
      version->content.size() != 1 ||
      static_cast<unsigned char>(version->content.front()) > 1) {
    return std::nullopt;
  }

  const auto algorithm{der_read(version->rest)};
  if (!algorithm.has_value() || algorithm->tag != 0x30) {
    return std::nullopt;
  }

  const auto private_key{der_read(algorithm->rest)};
  if (!private_key.has_value() || private_key->tag != 0x04) {
    return std::nullopt;
  }

  const auto oid{der_read(algorithm->content)};
  if (!oid.has_value() || oid->tag != 0x06) {
    return std::nullopt;
  }

  // The object identifiers are raw DER content bytes, kept as escaped literals
  // NOLINTBEGIN(modernize-raw-string-literal)
  constexpr std::string_view RSA{"\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01", 9};
  constexpr std::string_view RSA_PSS{"\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0a", 9};
  constexpr std::string_view ELLIPTIC_CURVE{"\x2a\x86\x48\xce\x3d\x02\x01", 7};
  constexpr std::string_view ED25519{"\x2b\x65\x70", 3};
  constexpr std::string_view ED448{"\x2b\x65\x71", 3};
  // NOLINTEND(modernize-raw-string-literal)

  if (oid->content == RSA || oid->content == RSA_PSS) {
    if (!rsa_private_key_acceptable(private_key->content)) {
      return std::nullopt;
    }

    return PKCS8Key{.kind = PKCS8KeyKind::RSA,
                    .curve = {},
                    .edwards_curve = {},
                    .key = private_key->content,
                    .rsa_pss_restricted = oid->content == RSA_PSS};
  }

  if (oid->content == ED25519 || oid->content == ED448) {
    return PKCS8Key{.kind = PKCS8KeyKind::Edwards,
                    .curve = {},
                    .edwards_curve = oid->content == ED25519
                                         ? EdwardsCurve::Ed25519
                                         : EdwardsCurve::Ed448,
                    .key = private_key->content};
  }

  if (oid->content == ELLIPTIC_CURVE) {
    const auto curve_oid{der_read(oid->rest)};
    if (!curve_oid.has_value() || curve_oid->tag != 0x06) {
      return std::nullopt;
    }

    // NOLINTBEGIN(modernize-raw-string-literal)
    constexpr std::string_view P256{"\x2a\x86\x48\xce\x3d\x03\x01\x07", 8};
    constexpr std::string_view P384{"\x2b\x81\x04\x00\x22", 5};
    constexpr std::string_view P521{"\x2b\x81\x04\x00\x23", 5};
    // NOLINTEND(modernize-raw-string-literal)
    EllipticCurve curve{};
    if (curve_oid->content == P256) {
      curve = EllipticCurve::P256;
    } else if (curve_oid->content == P384) {
      curve = EllipticCurve::P384;
    } else if (curve_oid->content == P521) {
      curve = EllipticCurve::P521;
    } else {
      return std::nullopt;
    }

    return PKCS8Key{.kind = PKCS8KeyKind::EllipticCurve,
                    .curve = curve,
                    .edwards_curve = {},
                    .key = private_key->content};
  }

  return std::nullopt;
}

} // namespace sourcemeta::core

#endif
