#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_bignum.h"
#include "crypto_ecc.h"
#include "crypto_eddsa.h"
#include "crypto_helpers.h"

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

// The big integer capacity must hold the product of two maximum-size operands
// plus a normalisation limb and Knuth's implicit leading-zero digit, otherwise
// the modular reduction would index past the fixed-capacity word array
static_assert(Bignum::capacity >= 2 * ((MAXIMUM_KEY_BYTES + 7) / 8) + 2);

namespace {

// The DigestInfo prefixes for the EMSA-PKCS1-v1_5 encoding, taken verbatim
// from RFC 8017 Section 9.2 Note 1
constexpr std::array<std::uint8_t, 19> DIGEST_INFO_SHA256{
    {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
     0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20}};
constexpr std::array<std::uint8_t, 19> DIGEST_INFO_SHA384{
    {0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
     0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30}};
constexpr std::array<std::uint8_t, 19> DIGEST_INFO_SHA512{
    {0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
     0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40}};

auto digest_info_prefix(const SignatureHashFunction hash) -> std::string_view {
  switch (hash) {
    case SignatureHashFunction::SHA256:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA256.data()),
              DIGEST_INFO_SHA256.size()};
    case SignatureHashFunction::SHA384:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA384.data()),
              DIGEST_INFO_SHA384.size()};
    case SignatureHashFunction::SHA512:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA512.data()),
              DIGEST_INFO_SHA512.size()};
  }

  std::unreachable();
}

// EMSA-PKCS1-v1_5 encoding (RFC 8017 Section 9.2)
auto build_encoded_message(const SignatureHashFunction hash,
                           const std::string_view message,
                           const std::size_t key_length)
    -> std::optional<std::string> {
  const auto prefix{digest_info_prefix(hash)};
  const auto digest{digest_message(hash, message)};
  const auto encoded_length{prefix.size() + digest.size()};

  // RFC 8017 Section 9.2 step 3: "If emLen < tLen + 11, output 'intended
  // encoded message length too short'"
  if (key_length < encoded_length + 11) {
    return std::nullopt;
  }

  std::string result;
  result.reserve(key_length);
  result.push_back('\x00');
  result.push_back('\x01');
  result.append(key_length - encoded_length - 3, '\xff');
  result.push_back('\x00');
  result.append(prefix);
  result.append(digest);
  return result;
}

// MGF1 mask generation (RFC 8017 Appendix B.2.1)
auto mask_generation(const SignatureHashFunction hash,
                     const std::string_view seed, const std::size_t length)
    -> std::string {
  std::string result;
  result.reserve(length + 64);
  std::uint32_t counter{0};
  while (result.size() < length) {
    std::string block{seed};
    block.push_back(static_cast<char>((counter >> 24u) & 0xffu));
    block.push_back(static_cast<char>((counter >> 16u) & 0xffu));
    block.push_back(static_cast<char>((counter >> 8u) & 0xffu));
    block.push_back(static_cast<char>(counter & 0xffu));
    result.append(digest_message(hash, block));
    counter += 1;
  }

  result.resize(length);
  return result;
}

// EMSA-PSS verification (RFC 8017 Section 9.1.2), with the salt length fixed to
// the hash function output as RFC 7518 Section 3.5 requires
auto emsa_pss_verify(const SignatureHashFunction hash,
                     const std::string_view message,
                     const std::string_view encoded_message,
                     const std::size_t encoded_bits) -> bool {
  const auto digest{digest_message(hash, message)};
  const auto hash_length{digest.size()};
  const auto salt_length{hash_length};
  const auto encoded_length{encoded_message.size()};

  // RFC 8017 Section 9.1.2 step 3: "If emLen < hLen + sLen + 2, output
  // 'inconsistent'"
  if (encoded_length < hash_length + salt_length + 2) {
    return false;
  }

  // RFC 8017 Section 9.1.2 step 4: "If the rightmost octet of EM does not have
  // hexadecimal value 0xbc, output 'inconsistent'"
  if (static_cast<std::uint8_t>(encoded_message.back()) != 0xbc) {
    return false;
  }

  const auto database_length{encoded_length - hash_length - 1};
  const auto masked_database{encoded_message.substr(0, database_length)};
  const auto hash_value{encoded_message.substr(database_length, hash_length)};

  // RFC 8017 Section 9.1.2 step 6: "If the leftmost 8emLen - emBits bits of the
  // leftmost octet in maskedDB are not all equal to zero, output
  // 'inconsistent'"
  const auto unused_bits{(8 * encoded_length) - encoded_bits};
  const auto unused_mask{
      static_cast<std::uint8_t>((0xff00u >> unused_bits) & 0xffu)};
  if ((static_cast<std::uint8_t>(masked_database.front()) & unused_mask) != 0) {
    return false;
  }

  auto database{mask_generation(hash, hash_value, database_length)};
  for (std::size_t index = 0; index < database_length; ++index) {
    database[index] =
        static_cast<char>(database[index] ^ masked_database[index]);
  }

  database[0] = static_cast<char>(static_cast<std::uint8_t>(database[0]) &
                                  static_cast<std::uint8_t>(~unused_mask));

  // RFC 8017 Section 9.1.2 step 10: "If the emLen - hLen - sLen - 2 leftmost
  // octets of DB are not zero or if the octet at position emLen - hLen - sLen -
  // 1 does not have hexadecimal value 0x01, output 'inconsistent'"
  const auto padding_length{encoded_length - hash_length - salt_length - 2};
  for (std::size_t index = 0; index < padding_length; ++index) {
    if (database[index] != '\x00') {
      return false;
    }
  }

  if (static_cast<std::uint8_t>(database[padding_length]) != 0x01) {
    return false;
  }

  // RFC 8017 Section 9.1.2 steps 12 and 13: hash the concatenation of eight
  // zero octets, the message digest, and the recovered salt
  std::string verification_input(8, '\x00');
  verification_input.append(digest);
  verification_input.append(database.substr(database_length - salt_length));
  const auto expected{digest_message(hash, verification_input)};
  return expected == hash_value;
}

auto to_curve_parameters(const EllipticCurve curve) -> EllipticCurveParameters {
  switch (curve) {
    case EllipticCurve::P256:
      return curve_p256();
    case EllipticCurve::P384:
      return curve_p384();
    case EllipticCurve::P521:
      return curve_p521();
  }

  std::unreachable();
}

// FIPS 186-4 Section 6.4 step 2, deriving the integer e from the leftmost bits
// of the message digest, truncated to the bit length of the order
auto digest_to_integer(const SignatureHashFunction hash,
                       const std::string_view message,
                       const std::size_t order_bits) -> Bignum {
  const auto digest{digest_message(hash, message)};
  auto value{bignum_from_bytes(digest)};
  const auto digest_bits{digest.size() * 8};
  if (digest_bits > order_bits) {
    value = bignum_shift_right(value, digest_bits - order_bits);
  }

  return value;
}

// RSASSA-PKCS1-v1_5 verification (RFC 8017 Section 8.2.2) over raw key material
auto verify_pkcs1(const SignatureHashFunction hash,
                  const std::string_view modulus,
                  const std::string_view exponent,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  // RFC 8017 Section 8.2.2 step 1: "If the length of S is not k octets, output
  // 'invalid signature'"
  const auto key_length{modulus.size()};
  if (signature.size() != key_length) {
    return false;
  }

  const auto modulus_number{bignum_from_bytes(modulus)};
  const auto signature_number{bignum_from_bytes(signature)};

  // RFC 8017 Section 5.2.2: "If the signature representative s is not between 0
  // and n - 1, output 'signature representative out of range'"
  if (bignum_compare(signature_number, modulus_number) >= 0) {
    return false;
  }

  const auto exponent_number{bignum_from_bytes(exponent)};
  const auto message_representative{
      bignum_mod_exp(signature_number, exponent_number, modulus_number)};
  const auto encoded_message{
      bignum_to_bytes(message_representative, key_length)};
  const auto expected{build_encoded_message(hash, message, key_length)};
  return expected.has_value() && encoded_message == expected.value();
}

// RSASSA-PSS verification (RFC 8017 Section 8.1.2) over raw key material
auto verify_pss(const SignatureHashFunction hash,
                const std::string_view modulus, const std::string_view exponent,
                const std::string_view message,
                const std::string_view signature) -> bool {
  // RFC 8017 Section 8.1.2 step 1: "If the length of the signature S is not k
  // octets, output 'invalid signature'"
  const auto key_length{modulus.size()};
  if (signature.size() != key_length) {
    return false;
  }

  const auto modulus_number{bignum_from_bytes(modulus)};
  const auto signature_number{bignum_from_bytes(signature)};

  // RFC 8017 Section 5.2.2: "If the signature representative s is not between 0
  // and n - 1, output 'signature representative out of range'"
  if (bignum_compare(signature_number, modulus_number) >= 0) {
    return false;
  }

  const auto exponent_number{bignum_from_bytes(exponent)};
  const auto message_representative{
      bignum_mod_exp(signature_number, exponent_number, modulus_number)};

  // RFC 8017 Section 8.1.2 step 2c: the encoded message is emLen octets long,
  // where emLen equals the byte length of emBits = modBits - 1 bits, which is
  // one octet less than k when the modulus bit length is congruent to one
  // modulo eight
  const auto encoded_bits{bignum_bit_length(modulus_number) - 1};
  const auto encoded_length{(encoded_bits + 7) / 8};
  const auto full_representative{
      bignum_to_bytes(message_representative, key_length)};
  for (std::size_t index = 0; index < key_length - encoded_length; ++index) {
    if (full_representative[index] != '\x00') {
      return false;
    }
  }

  const auto encoded_message{std::string_view{full_representative}.substr(
      key_length - encoded_length)};
  return emsa_pss_verify(hash, message, encoded_message, encoded_bits);
}

// ECDSA verification (FIPS 186-4 Section 6.4) over the raw public point
auto verify_ecdsa(const EllipticCurve curve, const SignatureHashFunction hash,
                  const std::string_view coordinate_x,
                  const std::string_view coordinate_y,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto parameters{to_curve_parameters(curve)};
  const auto field_bytes{parameters.field_bytes};

  // RFC 7518 Section 3.4: the signature is the fixed-width concatenation of the
  // two integers, each as long as the curve field
  if (signature.size() != field_bytes * 2) {
    return false;
  }

  const auto r{bignum_from_bytes(signature.substr(0, field_bytes))};
  const auto s{bignum_from_bytes(signature.substr(field_bytes))};

  // FIPS 186-4 Section 6.4.2 step 1: both integers must lie in [1, n - 1]
  if (bignum_is_zero(r) || bignum_compare(r, parameters.order) >= 0 ||
      bignum_is_zero(s) || bignum_compare(s, parameters.order) >= 0) {
    return false;
  }

  // Reject coordinates wider than the field, matching the platform backends and
  // preventing an oversized input from being truncated into a valid key
  const auto stripped_x{strip_left(coordinate_x, '\x00')};
  const auto stripped_y{strip_left(coordinate_y, '\x00')};
  if (stripped_x.size() > field_bytes || stripped_y.size() > field_bytes) {
    return false;
  }

  const auto public_x{bignum_from_bytes(stripped_x)};
  const auto public_y{bignum_from_bytes(stripped_y)};

  // The public key must be a valid point: coordinates below the field prime and
  // satisfying the curve equation
  if (bignum_compare(public_x, parameters.prime) >= 0 ||
      bignum_compare(public_y, parameters.prime) >= 0 ||
      !point_on_curve(public_x, public_y, parameters)) {
    return false;
  }

  const auto order_bits{bignum_bit_length(parameters.order)};
  const auto digest_integer{digest_to_integer(hash, message, order_bits)};
  const auto s_inverse{bignum_mod_inverse(s, parameters.order)};
  const auto u1{
      bignum_mod_multiply(digest_integer, s_inverse, parameters.order)};
  const auto u2{bignum_mod_multiply(r, s_inverse, parameters.order)};

  const JacobianPoint generator{.x = parameters.generator_x,
                                .y = parameters.generator_y,
                                .z = bignum_from_u64(1)};
  const JacobianPoint public_point{
      .x = public_x, .y = public_y, .z = bignum_from_u64(1)};
  const auto point{point_double_scalar_multiply(u1, generator, u2, public_point,
                                                parameters)};

  // FIPS 186-4 Section 6.4.2 step 6: reject when the combination is the point
  // at infinity
  if (point_is_infinity(point)) {
    return false;
  }

  // FIPS 186-4 Section 6.4.2 step 7: the signature is valid when the affine x
  // coordinate, reduced modulo the order, equals r
  auto candidate{point_affine_x(point, parameters)};
  bignum_reduce(candidate, parameters.order);
  return bignum_compare(candidate, r) == 0;
}

} // namespace

// The reference backend parses the key material into big integers inside each
// verification, which is cheap next to the modular arithmetic, so the parsed
// key simply holds the raw material
struct PublicKey::Internal {
  PublicKey::Type kind;
  std::string modulus;
  std::string exponent;
  std::string coordinate_x;
  std::string coordinate_y;
  EllipticCurve elliptic_curve;
  EdwardsCurve edwards_curve;
};

PublicKey::PublicKey(Internal *internal) noexcept : internal_{internal} {}

PublicKey::~PublicKey() { delete internal_; }

PublicKey::PublicKey(PublicKey &&other) noexcept : internal_{other.internal_} {
  other.internal_ = nullptr;
}

auto PublicKey::operator=(PublicKey &&other) noexcept -> PublicKey & {
  if (this != &other) {
    delete internal_;
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
  const auto stripped_modulus{strip_left(modulus, '\x00')};
  const auto stripped_exponent{strip_left(exponent, '\x00')};
  if (stripped_modulus.empty() || stripped_modulus.size() > MAXIMUM_KEY_BYTES ||
      stripped_exponent.size() > MAXIMUM_KEY_BYTES ||
      !rsa_public_exponent_acceptable(exponent, modulus)) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::RSA,
                              .modulus = std::string{stripped_modulus},
                              .exponent = std::string{stripped_exponent},
                              .coordinate_x = {},
                              .coordinate_y = {},
                              .elliptic_curve = {},
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

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::EllipticCurve,
                              .modulus = {},
                              .exponent = {},
                              .coordinate_x = std::string{stripped_x},
                              .coordinate_y = std::string{stripped_y},
                              .elliptic_curve = curve,
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
                              .modulus = {},
                              .exponent = {},
                              .coordinate_x = std::string{public_key},
                              .coordinate_y = {},
                              .elliptic_curve = {},
                              .edwards_curve = curve}};
}

auto rsassa_pkcs1_v15_verify(const PublicKey &key,
                             const SignatureHashFunction hash,
                             const std::string_view message,
                             const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  return internal != nullptr && internal->kind == PublicKey::Type::RSA &&
         verify_pkcs1(hash, internal->modulus, internal->exponent, message,
                      signature);
}

auto rsassa_pss_verify(const PublicKey &key, const SignatureHashFunction hash,
                       const std::string_view message,
                       const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  return internal != nullptr && internal->kind == PublicKey::Type::RSA &&
         verify_pss(hash, internal->modulus, internal->exponent, message,
                    signature);
}

auto ecdsa_verify(const PublicKey &key, const SignatureHashFunction hash,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  return internal != nullptr &&
         internal->kind == PublicKey::Type::EllipticCurve &&
         verify_ecdsa(internal->elliptic_curve, hash, internal->coordinate_x,
                      internal->coordinate_y, message, signature);
}

auto eddsa_verify(const PublicKey &key, const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::Edwards) {
    return false;
  }

  switch (internal->edwards_curve) {
    case EdwardsCurve::Ed25519:
      return edwards25519_verify(internal->coordinate_x, message, signature);
    case EdwardsCurve::Ed448:
      return edwards448_verify(internal->coordinate_x, message, signature);
  }

  std::unreachable();
}

} // namespace sourcemeta::core
