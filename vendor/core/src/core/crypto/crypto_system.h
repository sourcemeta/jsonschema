#ifndef SOURCEMETA_CORE_CRYPTO_SYSTEM_H_
#define SOURCEMETA_CORE_CRYPTO_SYSTEM_H_

// Primitives that only the backends built on a system cryptography library
// reach for, kept apart from the ones that the reference backend shares

#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_der.h"
#include "crypto_helpers.h"

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair, std::unreachable

namespace sourcemeta::core {

// Whether a signature is a well-formed representative for the modulus. RFC 8017
// Section 8.2.2 step 1 and Section 8.1.2 step 1: "If the length of the
// signature S is not k octets, output "invalid signature" and stop". The length
// is checked because the range comparison reads both operands as bare integers,
// so a signature that merely dropped a leading zero octet would denote the same
// value and verify, giving a second encoding of one signature. Section 5.2.2
// then requires the range check, so that an unreduced signature, which an
// attacker forges by adding the modulus without changing the modular
// exponentiation result, is rejected. The modulus is stripped for the length so
// that a stored ASN.1 sign octet cannot inflate k
inline auto rsa_signature_acceptable(const std::string_view signature,
                                     const std::string_view modulus) noexcept
    -> bool {
  return signature.size() == strip_left(modulus, '\x00').size() &&
         octets_below(signature, modulus);
}

// The order bit length of each curve, which the platform key generators take as
// the requested key size. It is not the field width in bits, since P-521 has a
// 521-bit order that does not fill its 66 octets
inline auto curve_bit_length(const EllipticCurve curve) noexcept
    -> std::size_t {
  switch (curve) {
    case EllipticCurve::P256:
      return 256;
    case EllipticCurve::P384:
      return 384;
    case EllipticCurve::P521:
      return 521;
  }

  std::unreachable();
}

// Identify a curve from its field width, the inverse of the mapping from a
// curve to that width, so a backend that reports a key only by coordinate size
// resolves it the same way
inline auto ec_curve_from_field_bytes(const std::size_t field_bytes) noexcept
    -> std::optional<EllipticCurve> {
  switch (field_bytes) {
    case 32:
      return EllipticCurve::P256;
    case 48:
      return EllipticCurve::P384;
    case 66:
      return EllipticCurve::P521;
    default:
      return std::nullopt;
  }
}

// The signature octet length is fixed per curve (RFC 8032 Section 5.1.6)
inline auto eddsa_signature_bytes(const EdwardsCurve curve) noexcept
    -> std::size_t {
  switch (curve) {
    case EdwardsCurve::Ed25519:
      return 64;
    case EdwardsCurve::Ed448:
      return 114;
  }

  std::unreachable();
}

// Read the modulus and public exponent from a PKCS#1 RSAPublicKey structure
// (RFC 8017 Appendix A.1.1), a DER SEQUENCE of exactly the two integers, each
// returned as its minimal big-endian magnitude. Trailing bytes at either level
// are rejected so the input parses as exactly that structure
inline auto der_read_rsa_public_key(const std::string_view der)
    -> std::optional<std::pair<std::string, std::string>> {
  const auto sequence{der_read(der)};
  if (!sequence.has_value() || sequence->tag != 0x30 ||
      !sequence->rest.empty()) {
    return std::nullopt;
  }

  const auto modulus{der_read(sequence->content)};
  if (!modulus.has_value() || modulus->tag != 0x02) {
    return std::nullopt;
  }

  const auto exponent{der_read(modulus->rest)};
  if (!exponent.has_value() || exponent->tag != 0x02 ||
      !exponent->rest.empty()) {
    return std::nullopt;
  }

  const auto modulus_value{der_unsigned_integer(modulus->content)};
  const auto exponent_value{der_unsigned_integer(exponent->content)};
  if (!modulus_value.has_value() || !exponent_value.has_value()) {
    return std::nullopt;
  }

  return std::pair{std::string{modulus_value.value()},
                   std::string{exponent_value.value()}};
}

} // namespace sourcemeta::core

#endif
