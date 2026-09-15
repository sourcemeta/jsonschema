#ifndef SOURCEMETA_CORE_CRYPTO_EDDSA_H_
#define SOURCEMETA_CORE_CRYPTO_EDDSA_H_

// Edwards-curve signatures (Ed25519 and Ed448, RFC 8032 Section 5, the pure
// variants) for the backends without a native EdDSA primitive. Points are kept
// in extended Edwards coordinates, so that the group law is a single set of
// complete formulas shared by both curves. Verification consumes only public
// inputs and stays variable time; the signing paths use the constant-time
// scalar multiplication, inverse, and encoding below, evaluated with the
// constant-time field layer, so they do not depend on the secret scalar

#include <sourcemeta/core/crypto_sha512.h>

#include "crypto_bignum.h"
#include "crypto_helpers.h"
#include "crypto_shake256.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// A point in extended Edwards coordinates (X : Y : Z : T), where the affine
// point is (X / Z, Y / Z) and T = X * Y / Z (RFC 8032 Section 5.1.4)
struct EdwardsPoint {
  CurveBignum x;
  CurveBignum y;
  CurveBignum z;
  CurveBignum t;
};

struct EdwardsParameters {
  CurveBignum prime;
  CurveBignum order;
  CurveBignum coefficient_a;
  CurveBignum coefficient_d;
  // Whether the coefficient a is -1, as on the twisted Ed25519 curve, rather
  // than 1, as on Ed448, which lets the point formulas negate or keep a value
  // instead of multiplying it by the coefficient
  bool coefficient_a_is_minus_one{false};
  // A square root of -1 modulo the Ed25519 prime, which recovers the second
  // candidate root when decoding a point, and left zero for Ed448
  CurveBignum square_root_of_minus_one;
  EdwardsPoint base;
  // The constant-time field and group order arithmetic contexts, built with the
  // parameters so that every ladder and signature does not rebuild them
  CurveBarrettContext field;
  CurveBarrettContext order_field;
};

// Interpret the bytes as a little-endian unsigned integer, the encoding EdDSA
// uses throughout (RFC 8032 Section 5.1.2), by reversing into the big-endian
// conversion
inline auto bignum_from_bytes_little_endian(const std::string_view input)
    -> CurveBignum {
  const std::string reversed{input.rbegin(), input.rend()};
  return bignum_from_bytes<CURVE_BIGNUM_CAPACITY>(reversed);
}

// Ed25519 field reduction in constant time, for the signing ladder. The prime
// is 2^255 - 19, so 2^256 is congruent to 38 modulo it, and the high half of a
// product folds onto the low half scaled by 38, carrying at most 38 out of the
// top word. A second fold takes that carry, worth 38 each, and bit 255, worth
// 19, back in at once, leaving a value below 2^255 + 1463. Such a value is at
// least the prime exactly when adding 19 to it reaches bit 255, and that sum
// without bit 255 is then the reduced value, so a single masked selection
// finishes the reduction
inline auto field_reduce_25519_ct(
    const CurveBignum &value,
    [[maybe_unused]] const CurveBarrettContext &context) noexcept
    -> CurveBignum {
  const auto *value_data{value.words.data()};
  CurveBignum folded;
  auto *folded_data{folded.words.data()};
  std::uint64_t carry{0};
  for (std::size_t index = 0; index < 4; ++index) {
    const auto total{
        static_cast<BignumDoubleWord>(value_data[index]) +
        (static_cast<BignumDoubleWord>(value_data[index + 4]) * 38U) + carry};
    folded_data[index] = static_cast<std::uint64_t>(total);
    carry = static_cast<std::uint64_t>(total >> 64U);
  }

  std::uint64_t addend{((folded_data[3] >> 63U) + (carry << 1U)) * 19U};
  folded_data[3] &= 0x7fffffffffffffffULL;
  for (std::size_t index = 0; index < 4; ++index) {
    const auto total{static_cast<BignumDoubleWord>(folded_data[index]) +
                     addend};
    folded_data[index] = static_cast<std::uint64_t>(total);
    addend = static_cast<std::uint64_t>(total >> 64U);
  }

  CurveBignum reduced;
  auto *reduced_data{reduced.words.data()};
  addend = 19U;
  for (std::size_t index = 0; index < 4; ++index) {
    const auto total{static_cast<BignumDoubleWord>(folded_data[index]) +
                     addend};
    reduced_data[index] = static_cast<std::uint64_t>(total);
    addend = static_cast<std::uint64_t>(total >> 64U);
  }

  const std::uint64_t mask{std::uint64_t{0} - (reduced_data[3] >> 63U)};
  reduced_data[3] &= 0x7fffffffffffffffULL;
  for (std::size_t index = 0; index < 4; ++index) {
    folded_data[index] =
        (reduced_data[index] & mask) | (folded_data[index] & ~mask);
  }

  folded.size = 4;
  return folded;
}

inline auto edwards_point_conditional_select(const bool condition,
                                             const EdwardsPoint &when_true,
                                             const EdwardsPoint &when_false,
                                             const std::size_t words) noexcept
    -> EdwardsPoint {
  return {.x = bignum_conditional_select(condition, when_true.x, when_false.x,
                                         words),
          .y = bignum_conditional_select(condition, when_true.y, when_false.y,
                                         words),
          .z = bignum_conditional_select(condition, when_true.z, when_false.z,
                                         words),
          .t = bignum_conditional_select(condition, when_true.t, when_false.t,
                                         words)};
}

// The complete unified Edwards addition formulas in extended coordinates
// (Hisil, Wong, Carter, and Dawson 2008), which hold for any two points,
// including equal points and the identity, since the curve coefficient is a
// square and d is a non-square modulo p. They run over the constant-time field
// arithmetic, which signing needs for its secret operands and verification
// reuses on its public ones
inline auto edwards_point_add_constant_time(
    const EdwardsPoint &left, const EdwardsPoint &right,
    const EdwardsParameters &parameters,
    const CurveBarrettContext &field) noexcept -> EdwardsPoint {
  const auto a{field_mod_multiply_ct(left.x, right.x, field)};
  const auto b{field_mod_multiply_ct(left.y, right.y, field)};
  const auto c{field_mod_multiply_ct(
      field_mod_multiply_ct(parameters.coefficient_d, left.t, field), right.t,
      field)};
  const auto d{field_mod_multiply_ct(left.z, right.z, field)};
  const auto e{field_subtract_ct(
      field_mod_multiply_ct(field_add_ct(left.x, left.y, field),
                            field_add_ct(right.x, right.y, field), field),
      field_add_ct(a, b, field), field)};
  const auto f{field_subtract_ct(d, c, field)};
  const auto g{field_add_ct(d, c, field)};
  // H = B - a * A, where a is -1 or 1
  const auto h{parameters.coefficient_a_is_minus_one
                   ? field_add_ct(b, a, field)
                   : field_subtract_ct(b, a, field)};
  return EdwardsPoint{.x = field_mod_multiply_ct(e, f, field),
                      .y = field_mod_multiply_ct(g, h, field),
                      .z = field_mod_multiply_ct(f, g, field),
                      .t = field_mod_multiply_ct(e, h, field)};
}

// Dedicated doubling in extended coordinates (Hisil, Wong, Carter, and Dawson
// 2008, Section 3.3), the formula RFC 8032 Section 5.1.4 recommends for
// Ed25519, written for a coefficient a of -1 or 1. It reads neither d nor the T
// coordinate of the input and, like the unified addition, holds for every point
// of these curves, the identity included. The T coordinate of the result is
// only computed when requested, as a doubling that feeds another doubling never
// reads it
inline auto edwards_point_double_constant_time(
    const EdwardsPoint &point, const EdwardsParameters &parameters,
    const CurveBarrettContext &field, const bool extended) noexcept
    -> EdwardsPoint {
  const auto a{field_square_ct(point.x, field)};
  const auto b{field_square_ct(point.y, field)};
  const auto z_squared{field_square_ct(point.z, field)};
  const auto c{field_add_ct(z_squared, z_squared, field)};
  // D = a * A, where a is -1 or 1
  const auto d{parameters.coefficient_a_is_minus_one
                   ? field_subtract_ct(CurveBignum{}, a, field)
                   : a};
  const auto e{field_subtract_ct(
      field_square_ct(field_add_ct(point.x, point.y, field), field),
      field_add_ct(a, b, field), field)};
  const auto g{field_add_ct(d, b, field)};
  const auto f{field_subtract_ct(g, c, field)};
  const auto h{field_subtract_ct(d, b, field)};
  return EdwardsPoint{.x = field_mod_multiply_ct(e, f, field),
                      .y = field_mod_multiply_ct(g, h, field),
                      .z = field_mod_multiply_ct(f, g, field),
                      .t = extended ? field_mod_multiply_ct(e, h, field)
                                    : CurveBignum{}};
}

// For the signing path, where the scalar is secret: a fixed four-bit window
// ladder over the complete Edwards formulas evaluated in constant time. The
// window count is fixed by the public field size, every window doubles four
// times and adds one table entry taken through a masked scan over the whole
// table, and the complete formulas absorb the identity entry of a zero window,
// so neither the control flow nor the field arithmetic depends on the scalar
inline auto edwards_point_scalar_multiply_constant_time(
    const CurveBignum &scalar, const EdwardsPoint &point,
    const EdwardsParameters &parameters) -> EdwardsPoint {
  const auto &field{parameters.field};
  std::array<EdwardsPoint, 16> multiples{};
  // The identity element is (0 : 1 : 1 : 0)
  multiples[0] = EdwardsPoint{.x = CurveBignum{},
                              .y = bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1),
                              .z = bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1),
                              .t = CurveBignum{}};
  multiples[1] = point;
  for (std::size_t index = 2; index < multiples.size(); ++index) {
    multiples[index] = edwards_point_add_constant_time(
        multiples[index - 1], point, parameters, field);
  }

  auto result{multiples[0]};
  const auto windows{(bignum_bit_length(parameters.prime) + 3) / 4};
  for (std::size_t window = windows; window > 0; --window) {
    for (std::size_t step = 0; step < 4; ++step) {
      result = edwards_point_double_constant_time(result, parameters, field,
                                                  step == 3);
    }

    std::size_t digit{0};
    for (std::size_t bit = 0; bit < 4; ++bit) {
      digit |= static_cast<std::size_t>(
                   bignum_get_bit_fixed(scalar, ((window - 1) * 4) + bit))
               << bit;
    }

    EdwardsPoint selected{};
    for (std::size_t index = 0; index < multiples.size(); ++index) {
      selected = edwards_point_conditional_select(
          digit == index, multiples[index], selected, field.words);
    }

    result =
        edwards_point_add_constant_time(result, selected, parameters, field);
  }

  return result;
}

// The negation of a point in extended coordinates, (-X : Y : Z : -T)
inline auto edwards_point_negate(const EdwardsPoint &point,
                                 const CurveBarrettContext &field) noexcept
    -> EdwardsPoint {
  return {.x = field_subtract_ct(CurveBignum{}, point.x, field),
          .y = point.y,
          .z = point.z,
          .t = field_subtract_ct(CurveBignum{}, point.t, field)};
}

// Compute [first_scalar] first_point + [second_scalar] second_point with
// Shamir's trick, a single double-and-add over the longer scalar that adds the
// precomputed sum whenever both scalars have a set bit. Only verification uses
// it, where both scalars and both points are public, so the bits may steer the
// additions
inline auto edwards_point_double_scalar_multiply(
    const CurveBignum &first_scalar, const EdwardsPoint &first_point,
    const CurveBignum &second_scalar, const EdwardsPoint &second_point,
    const EdwardsParameters &parameters) -> EdwardsPoint {
  const auto &field{parameters.field};
  const auto combined{edwards_point_add_constant_time(first_point, second_point,
                                                      parameters, field)};
  // The identity element is (0 : 1 : 1 : 0)
  EdwardsPoint result{.x = CurveBignum{},
                      .y = bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1),
                      .z = bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1),
                      .t = CurveBignum{}};
  const auto first_bits{bignum_bit_length(first_scalar)};
  const auto second_bits{bignum_bit_length(second_scalar)};
  const auto bits{first_bits > second_bits ? first_bits : second_bits};
  for (std::size_t index = bits; index > 0; --index) {
    const auto first_bit{bignum_get_bit(first_scalar, index - 1)};
    const auto second_bit{bignum_get_bit(second_scalar, index - 1)};
    result = edwards_point_double_constant_time(result, parameters, field,
                                                first_bit || second_bit);
    if (first_bit && second_bit) {
      result =
          edwards_point_add_constant_time(result, combined, parameters, field);
    } else if (first_bit) {
      result = edwards_point_add_constant_time(result, first_point, parameters,
                                               field);
    } else if (second_bit) {
      result = edwards_point_add_constant_time(result, second_point, parameters,
                                               field);
    }
  }

  return result;
}

// Whether a projective point equals an affine one, whose Z coordinate is one,
// compared without leaving projective space as X = x * Z and Y = y * Z
inline auto edwards_point_matches_affine(const EdwardsPoint &point,
                                         const EdwardsPoint &affine,
                                         const CurveBarrettContext &field)
    -> bool {
  return field_equal_ct(
             point.x, field_mod_multiply_ct(affine.x, point.z, field), field) &&
         field_equal_ct(point.y,
                        field_mod_multiply_ct(affine.y, point.z, field), field);
}

// Encode a point into the little-endian y coordinate with the low bit of x in
// the final bit (RFC 8032 Section 5.1.2), the inverse of the point decoding
inline auto edwards_point_encode(const EdwardsPoint &point,
                                 const EdwardsParameters &parameters,
                                 const std::size_t length) -> std::string {
  // Only the signing path encodes points, and its projective z derives from the
  // secret scalar, so the coordinate recovery is taken in constant time
  const auto &field{parameters.field};
  const auto z_inverse{field_inverse_ct(point.z, field)};
  const auto x{field_mod_multiply_ct(point.x, z_inverse, field)};
  const auto y{field_mod_multiply_ct(point.y, z_inverse, field)};
  const auto big_endian{bignum_to_bytes(y, length)};
  std::string encoding{big_endian.rbegin(), big_endian.rend()};
  if (bignum_get_bit(x, 0)) {
    encoding.back() =
        static_cast<char>(static_cast<std::uint8_t>(encoding.back()) | 0x80U);
  }

  return encoding;
}

// The public key is the encoded base point multiplied by the pruned secret
// scalar (RFC 8032 Sections 5.1.5 and 5.2.5). The multiplication is
// constant-time because the scalar is secret, though the resulting point is
// public
inline auto edwards_public_key_point(const CurveBignum &scalar,
                                     const EdwardsParameters &parameters,
                                     const std::size_t length) -> std::string {
  return edwards_point_encode(edwards_point_scalar_multiply_constant_time(
                                  scalar, parameters.base, parameters),
                              parameters, length);
}

// Recover an Ed25519 point from its 32-byte encoding (RFC 8032 Section 5.1.3),
// returning no value when the encoding does not name a point on the curve. The
// recovery runs over the field arithmetic context of the parameters
inline auto edwards25519_decode_point(const std::string_view encoding,
                                      const EdwardsParameters &parameters)
    -> std::optional<EdwardsPoint> {
  if (encoding.size() != 32) {
    return std::nullopt;
  }

  // The final bit holds the sign of x, the remaining bits the little-endian y
  std::string bytes{encoding};
  const auto sign_bit{
      static_cast<unsigned>(static_cast<std::uint8_t>(bytes.back()) >> 7) & 1U};
  bytes.back() =
      static_cast<char>(static_cast<std::uint8_t>(bytes.back()) & 0x7fU);
  const auto y{bignum_from_bytes_little_endian(bytes)};

  // A y coordinate at or beyond the field prime is not a canonical encoding
  const auto &prime{parameters.prime};
  if (bignum_compare(y, prime) >= 0) {
    return std::nullopt;
  }

  const auto &field{parameters.field};
  const auto one{bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1)};
  const auto y_squared{field_square_ct(y, field)};

  // Solve x^2 = (y^2 - 1) / (d * y^2 + 1) (mod p)
  const auto numerator{field_subtract_ct(y_squared, one, field)};
  const auto denominator{field_add_ct(
      field_mod_multiply_ct(parameters.coefficient_d, y_squared, field), one,
      field)};

  // The candidate root is x = numerator * denominator^3 *
  // (numerator * denominator^7)^((p - 5) / 8) (mod p), a single powering that
  // folds in the inversion of the denominator
  const auto denominator_squared{field_square_ct(denominator, field)};
  const auto denominator_cubed{
      field_mod_multiply_ct(denominator_squared, denominator, field)};
  const auto denominator_seventh{field_mod_multiply_ct(
      field_square_ct(denominator_cubed, field), denominator, field)};
  auto exponent{prime};
  bignum_subtract_in_place(exponent, bignum_from_u64<CURVE_BIGNUM_CAPACITY>(5));
  exponent = bignum_shift_right(exponent, 3);
  const auto root{field_power_ct(
      field_mod_multiply_ct(numerator, denominator_seventh, field), exponent,
      field)};
  auto candidate{field_mod_multiply_ct(
      field_mod_multiply_ct(numerator, denominator_cubed, field), root, field)};

  // The candidate is correct when denominator * x^2 equals the numerator, off
  // by sqrt(-1) when it equals its negation, and otherwise no root exists
  const auto check{field_mod_multiply_ct(
      denominator, field_square_ct(candidate, field), field)};
  if (!field_equal_ct(check, numerator, field)) {
    if (!field_equal_ct(
            check, field_subtract_ct(CurveBignum{}, numerator, field), field)) {
      return std::nullopt;
    }

    candidate = field_mod_multiply_ct(
        candidate, parameters.square_root_of_minus_one, field);
  }

  // Reject the non-canonical zero root with a set sign bit, then select the
  // root whose low bit matches the encoded sign
  bignum_normalize(candidate);
  if (bignum_is_zero(candidate) && sign_bit == 1) {
    return std::nullopt;
  }

  if (static_cast<unsigned>(bignum_get_bit(candidate, 0)) != sign_bit) {
    auto negated{prime};
    bignum_subtract_in_place(negated, candidate);
    candidate = negated;
  }

  return EdwardsPoint{.x = candidate,
                      .y = y,
                      .z = one,
                      .t = field_mod_multiply_ct(candidate, y, field)};
}

// The Edwards25519 domain parameters (RFC 8032 Section 5.1)
inline auto edwards25519_parameters() -> EdwardsParameters {
  EdwardsParameters parameters;
  parameters.prime = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
      "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed");
  parameters.order = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
      "1000000000000000000000000000000014def9dea2f79cd65812631a5cf5d3ed");

  // The curve coefficient a is -1 (mod p)
  parameters.coefficient_a = parameters.prime;
  bignum_subtract_in_place(parameters.coefficient_a,
                           bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1));
  parameters.coefficient_a_is_minus_one = true;

  // d = -121665 / 121666 (mod p)
  auto negated_numerator{parameters.prime};
  bignum_subtract_in_place(negated_numerator,
                           bignum_from_u64<CURVE_BIGNUM_CAPACITY>(121665));
  parameters.coefficient_d = bignum_mod_multiply(
      negated_numerator,
      bignum_mod_inverse(bignum_from_u64<CURVE_BIGNUM_CAPACITY>(121666),
                         parameters.prime),
      parameters.prime);

  // sqrt(-1) = 2^((p - 1) / 4) (mod p), used to recover the second root
  auto root_exponent{parameters.prime};
  bignum_subtract_in_place(root_exponent,
                           bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1));
  root_exponent = bignum_shift_right(root_exponent, 2);
  parameters.square_root_of_minus_one =
      bignum_mod_exp(bignum_from_u64<CURVE_BIGNUM_CAPACITY>(2), root_exponent,
                     parameters.prime);
  parameters.field = barrett_context(parameters.prime);
  parameters.field.reduce = &field_reduce_25519_ct;
  parameters.order_field = barrett_context(parameters.order);

  // The base point is recovered from its canonical encoding, y = 4/5 with a
  // clear sign bit (RFC 8032 Section 5.1)
  std::string base_encoding;
  base_encoding.push_back('\x58');
  base_encoding.append(31, '\x66');
  parameters.base =
      edwards25519_decode_point(base_encoding, parameters).value();
  return parameters;
}

// The Edwards25519 domain parameters derived once and shared, as every signing
// and verification would otherwise repeat the modular inverse and the
// exponentiations the derivation spends
inline auto edwards25519() -> const EdwardsParameters & {
  static const EdwardsParameters PARAMETERS{edwards25519_parameters()};
  return PARAMETERS;
}

// Verify an Ed25519 signature over a message (RFC 8032 Section 5.1.7), given
// the 32-byte public key and the 64-byte signature
inline auto edwards25519_verify(const std::string_view public_key,
                                const std::string_view message,
                                const std::string_view signature) -> bool {
  if (public_key.size() != 32 || signature.size() != 64) {
    return false;
  }

  const auto &parameters{edwards25519()};
  const auto public_point{edwards25519_decode_point(public_key, parameters)};
  if (!public_point.has_value()) {
    return false;
  }

  // The signature is the encoded point R followed by the little-endian scalar
  // S, which must lie below the group order
  const auto encoded_r{signature.substr(0, 32)};
  const auto point_r{edwards25519_decode_point(encoded_r, parameters)};
  if (!point_r.has_value()) {
    return false;
  }

  const auto scalar_s{bignum_from_bytes_little_endian(signature.substr(32))};
  if (bignum_compare(scalar_s, parameters.order) >= 0) {
    return false;
  }

  // k = SHA-512(R || A || M) reduced modulo the group order
  std::string preimage;
  preimage.reserve(encoded_r.size() + public_key.size() + message.size());
  preimage.append(encoded_r);
  preimage.append(public_key);
  preimage.append(message);
  const auto digest{sha512_digest(preimage)};
  auto scalar_k{bignum_from_bytes_little_endian(std::string_view{
      reinterpret_cast<const char *>(digest.data()), digest.size()})};
  bignum_reduce(scalar_k, parameters.order);

  // The signature holds when [S]B = R + [k]A, checked as [S]B + [k](-A) = R so
  // that both scalar multiplications share a single pass
  const auto combination{edwards_point_double_scalar_multiply(
      scalar_s, parameters.base, scalar_k,
      edwards_point_negate(public_point.value(), parameters.field),
      parameters)};
  return edwards_point_matches_affine(combination, point_r.value(),
                                      parameters.field);
}

// Prune the 32-byte secret scalar in place (RFC 8032 Section 5.1.5): "The
// lowest three bits of the first octet are cleared, the highest bit of the last
// octet is cleared, and the second highest bit of the last octet is set"
inline auto edwards25519_prune_scalar(std::string &scalar_bytes) noexcept
    -> void {
  scalar_bytes.front() = static_cast<char>(
      static_cast<std::uint8_t>(scalar_bytes.front()) & 0xf8U);
  scalar_bytes.back() = static_cast<char>(
      (static_cast<std::uint8_t>(scalar_bytes.back()) & 0x7fU) | 0x40U);
}

// The Ed25519 public key derived from the 32-byte private seed, the encoded
// point [s]B where s is the pruned first half of the seed hash (RFC 8032
// Section 5.1.5)
inline auto edwards25519_public_key(const std::string_view secret)
    -> std::optional<std::string> {
  if (secret.size() != 32) {
    return std::nullopt;
  }

  const auto &parameters{edwards25519()};
  auto hashed{sha512_digest(secret)};
  const SecureBufferScope hashed_scope{hashed.data(), hashed.size()};
  const std::string_view digest{reinterpret_cast<const char *>(hashed.data()),
                                hashed.size()};
  std::string scalar_bytes{digest.substr(0, 32)};
  const SecureStringScope scalar_bytes_scope{scalar_bytes};
  edwards25519_prune_scalar(scalar_bytes);
  auto scalar_a{bignum_from_bytes_little_endian(scalar_bytes)};
  const SecureBignumScope scalar_a_scope{scalar_a};
  return edwards_public_key_point(scalar_a, parameters, 32);
}

inline auto edwards25519_sign(const std::string_view secret,
                              const std::string_view message)
    -> std::optional<std::string> {
  if (secret.size() != 32) {
    return std::nullopt;
  }

  const auto &parameters{edwards25519()};
  // The key derivation hash carries both the secret scalar and the nonce
  // prefix, so it and everything derived from it below is wiped before
  // returning
  auto hashed{sha512_digest(secret)};
  const SecureBufferScope hashed_scope{hashed.data(), hashed.size()};
  const std::string_view digest{reinterpret_cast<const char *>(hashed.data()),
                                hashed.size()};

  // The secret scalar is the pruned first half, the prefix the second half
  std::string scalar_bytes{digest.substr(0, 32)};
  const SecureStringScope scalar_bytes_scope{scalar_bytes};
  edwards25519_prune_scalar(scalar_bytes);
  auto scalar_a{bignum_from_bytes_little_endian(scalar_bytes)};
  const SecureBignumScope scalar_a_scope{scalar_a};
  const auto prefix{digest.substr(32)};

  const auto public_key{edwards_public_key_point(scalar_a, parameters, 32)};

  // r = SHA-512(prefix || M) reduced, then R = [r]B
  std::string nonce_preimage{prefix};
  const SecureStringScope nonce_preimage_scope{nonce_preimage};
  nonce_preimage.append(message);
  auto nonce_digest{sha512_digest(nonce_preimage)};
  const SecureBufferScope nonce_digest_scope{nonce_digest.data(),
                                             nonce_digest.size()};
  auto scalar_r{bignum_from_bytes_little_endian(
      std::string_view{reinterpret_cast<const char *>(nonce_digest.data()),
                       nonce_digest.size()})};
  const SecureBignumScope scalar_r_scope{scalar_r};
  bignum_reduce(scalar_r, parameters.order);
  const auto encoded_r{
      edwards_point_encode(edwards_point_scalar_multiply_constant_time(
                               scalar_r, parameters.base, parameters),
                           parameters, 32)};

  // k = SHA-512(R || A || M) reduced, then S = (r + k * a) mod L. The k * a
  // product carries the secret scalar, so it is wiped; r, k, and the resulting
  // S are the public signature material
  std::string challenge_preimage{encoded_r};
  challenge_preimage.append(public_key);
  challenge_preimage.append(message);
  const auto challenge_digest{sha512_digest(challenge_preimage)};
  auto scalar_k{bignum_from_bytes_little_endian(
      std::string_view{reinterpret_cast<const char *>(challenge_digest.data()),
                       challenge_digest.size()})};
  bignum_reduce(scalar_k, parameters.order);
  // The k * a product and its sum with the nonce carry the secret scalar and
  // nonce, so both run over the constant-time field arithmetic modulo the
  // order; k is public and r, k, and the resulting S are the public signature
  // material
  const auto &order_field{parameters.order_field};
  auto scalar_a_reduced{barrett_reduce(scalar_a, order_field)};
  const SecureBignumScope scalar_a_reduced_scope{scalar_a_reduced};
  auto challenge_product{
      field_mod_multiply_ct(scalar_k, scalar_a_reduced, order_field)};
  const SecureBignumScope challenge_product_scope{challenge_product};
  auto scalar_s{field_add_ct(scalar_r, challenge_product, order_field)};
  bignum_normalize(scalar_s);

  const auto scalar_s_big_endian{bignum_to_bytes(scalar_s, 32)};
  std::string signature{encoded_r};
  signature.append(scalar_s_big_endian.rbegin(), scalar_s_big_endian.rend());
  return signature;
}

// Recover an Ed448 point from its 57-byte encoding (RFC 8032 Section 5.2.3),
// returning no value when the encoding does not name a point on the curve. The
// recovery runs over the field arithmetic context of the parameters
inline auto edwards448_decode_point(const std::string_view encoding,
                                    const EdwardsParameters &parameters)
    -> std::optional<EdwardsPoint> {
  if (encoding.size() != 57) {
    return std::nullopt;
  }

  // The final bit holds the sign of x, the remaining bits the little-endian y
  std::string bytes{encoding};
  const auto sign_bit{
      static_cast<unsigned>(static_cast<std::uint8_t>(bytes.back()) >> 7) & 1U};
  bytes.back() =
      static_cast<char>(static_cast<std::uint8_t>(bytes.back()) & 0x7fU);
  const auto y{bignum_from_bytes_little_endian(bytes)};

  // A y coordinate at or beyond the field prime is not a canonical encoding
  const auto &prime{parameters.prime};
  if (bignum_compare(y, prime) >= 0) {
    return std::nullopt;
  }

  const auto &field{parameters.field};
  const auto one{bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1)};
  const auto y_squared{field_square_ct(y, field)};

  // Solve x^2 = (y^2 - 1) / (d * y^2 - 1) (mod p)
  const auto numerator{field_subtract_ct(y_squared, one, field)};
  const auto denominator{field_subtract_ct(
      field_mod_multiply_ct(parameters.coefficient_d, y_squared, field), one,
      field)};

  // The candidate root is x = numerator^3 * denominator *
  // (numerator^5 * denominator^3)^((p - 3) / 4) (mod p), the field having
  // p congruent to 3 modulo 4
  const auto numerator_squared{field_square_ct(numerator, field)};
  const auto numerator_cubed{
      field_mod_multiply_ct(numerator_squared, numerator, field)};
  const auto numerator_fifth{
      field_mod_multiply_ct(numerator_squared, numerator_cubed, field)};
  const auto denominator_squared{field_square_ct(denominator, field)};
  const auto denominator_cubed{
      field_mod_multiply_ct(denominator_squared, denominator, field)};
  auto exponent{prime};
  bignum_subtract_in_place(exponent, bignum_from_u64<CURVE_BIGNUM_CAPACITY>(3));
  exponent = bignum_shift_right(exponent, 2);
  const auto root{field_power_ct(
      field_mod_multiply_ct(numerator_fifth, denominator_cubed, field),
      exponent, field)};
  auto candidate{field_mod_multiply_ct(
      field_mod_multiply_ct(numerator_cubed, denominator, field), root, field)};

  // The candidate is correct when denominator * x^2 equals the numerator, and
  // otherwise no root exists, as the field admits a single square root
  const auto check{field_mod_multiply_ct(
      denominator, field_square_ct(candidate, field), field)};
  if (!field_equal_ct(check, numerator, field)) {
    return std::nullopt;
  }

  // Reject the non-canonical zero root with a set sign bit, then select the
  // root whose low bit matches the encoded sign
  bignum_normalize(candidate);
  if (bignum_is_zero(candidate) && sign_bit == 1) {
    return std::nullopt;
  }

  if (static_cast<unsigned>(bignum_get_bit(candidate, 0)) != sign_bit) {
    auto negated{prime};
    bignum_subtract_in_place(negated, candidate);
    candidate = negated;
  }

  return EdwardsPoint{.x = candidate,
                      .y = y,
                      .z = one,
                      .t = field_mod_multiply_ct(candidate, y, field)};
}

// The Edwards448 domain parameters (RFC 8032 Section 5.2)
inline auto edwards448_parameters() -> EdwardsParameters {
  EdwardsParameters parameters;
  // clang-format off
  parameters.prime = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("fffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  parameters.order = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("3fffffffffffffffffffffffffffffffffffffffffffffffffffffff7cca23e9c44edb49aed63690216cc2728dc58f552378c292ab5844f3");
  // clang-format on

  // The curve coefficient a is 1, and d is -39081 (mod p)
  parameters.coefficient_a = bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1);
  parameters.coefficient_a_is_minus_one = false;
  parameters.coefficient_d = parameters.prime;
  bignum_subtract_in_place(parameters.coefficient_d,
                           bignum_from_u64<CURVE_BIGNUM_CAPACITY>(39081));

  // The base point is recovered from its canonical 57-octet encoding (RFC 8032
  // Section 5.2)
  // clang-format off
  const auto base_encoding{bignum_to_bytes(bignum_from_hex<CURVE_BIGNUM_CAPACITY>("14fa30f25b790898adc8d74e2c13bdfdc4397ce61cffd33ad7c2a0051e9c78874098a36c7373ea4b62c7c9563720768824bcb66e71463f6900"), 57)};
  // clang-format on
  parameters.field = barrett_context(parameters.prime);
  parameters.order_field = barrett_context(parameters.order);
  parameters.base = edwards448_decode_point(base_encoding, parameters).value();
  return parameters;
}

// The Edwards448 domain parameters derived once and shared, as every signing
// and verification would otherwise repeat the exponentiation that decoding the
// base point spends
inline auto edwards448() -> const EdwardsParameters & {
  static const EdwardsParameters PARAMETERS{edwards448_parameters()};
  return PARAMETERS;
}

// Verify an Ed448 signature over a message (RFC 8032 Section 5.2.7), given the
// 57-byte public key and the 114-byte signature
inline auto edwards448_verify(const std::string_view public_key,
                              const std::string_view message,
                              const std::string_view signature) -> bool {
  if (public_key.size() != 57 || signature.size() != 114) {
    return false;
  }

  const auto &parameters{edwards448()};
  const auto public_point{edwards448_decode_point(public_key, parameters)};
  if (!public_point.has_value()) {
    return false;
  }

  // The signature is the encoded point R followed by the little-endian scalar
  // S, which must lie below the group order
  const auto encoded_r{signature.substr(0, 57)};
  const auto point_r{edwards448_decode_point(encoded_r, parameters)};
  if (!point_r.has_value()) {
    return false;
  }

  const auto scalar_s{bignum_from_bytes_little_endian(signature.substr(57))};
  if (bignum_compare(scalar_s, parameters.order) >= 0) {
    return false;
  }

  // k = SHAKE256(dom4 || R || A || M) reduced modulo the group order, where
  // dom4 is "SigEd448" followed by the zero pre-hash flag and an empty context
  // (RFC 8032 Section 5.2.7 and Section 2)
  std::string preimage{"SigEd448"};
  preimage.push_back('\x00');
  preimage.push_back('\x00');
  preimage.append(encoded_r);
  preimage.append(public_key);
  preimage.append(message);
  const auto digest{shake256(preimage, 114)};
  auto scalar_k{bignum_from_bytes_little_endian(digest)};
  bignum_reduce(scalar_k, parameters.order);

  // The signature holds when [S]B = R + [k]A, checked as [S]B + [k](-A) = R so
  // that both scalar multiplications share a single pass
  const auto combination{edwards_point_double_scalar_multiply(
      scalar_s, parameters.base, scalar_k,
      edwards_point_negate(public_point.value(), parameters.field),
      parameters)};
  return edwards_point_matches_affine(combination, point_r.value(),
                                      parameters.field);
}

// Prune the 57-byte secret scalar in place (RFC 8032 Section 5.2.5): "The two
// least significant bits of the first octet are cleared, all eight bits of the
// last octet are cleared, and the highest bit of the second to last octet is
// set"
inline auto edwards448_prune_scalar(std::string &scalar_bytes) noexcept
    -> void {
  scalar_bytes.front() = static_cast<char>(
      static_cast<std::uint8_t>(scalar_bytes.front()) & 0xfcU);
  scalar_bytes[55] =
      static_cast<char>(static_cast<std::uint8_t>(scalar_bytes[55]) | 0x80U);
  scalar_bytes[56] = '\x00';
}

// The Ed448 public key derived from the 57-byte private seed (RFC 8032
// Section 5.2.5)
inline auto edwards448_public_key(const std::string_view secret)
    -> std::optional<std::string> {
  if (secret.size() != 57) {
    return std::nullopt;
  }

  const auto &parameters{edwards448()};
  auto digest{shake256(secret, 114)};
  const SecureStringScope digest_scope{digest};
  std::string scalar_bytes{digest.substr(0, 57)};
  const SecureStringScope scalar_bytes_scope{scalar_bytes};
  edwards448_prune_scalar(scalar_bytes);
  auto scalar_a{bignum_from_bytes_little_endian(scalar_bytes)};
  const SecureBignumScope scalar_a_scope{scalar_a};
  return edwards_public_key_point(scalar_a, parameters, 57);
}

inline auto edwards448_sign(const std::string_view secret,
                            const std::string_view message)
    -> std::optional<std::string> {
  if (secret.size() != 57) {
    return std::nullopt;
  }

  const auto &parameters{edwards448()};
  // The key derivation hash carries both the secret scalar and the nonce
  // prefix, so it and everything derived from it below is wiped before
  // returning
  auto digest{shake256(secret, 114)};
  const SecureStringScope digest_scope{digest};

  // The secret scalar is the pruned first half, the prefix the second half
  std::string scalar_bytes{digest.substr(0, 57)};
  const SecureStringScope scalar_bytes_scope{scalar_bytes};
  edwards448_prune_scalar(scalar_bytes);
  auto scalar_a{bignum_from_bytes_little_endian(scalar_bytes)};
  const SecureBignumScope scalar_a_scope{scalar_a};
  const auto prefix{std::string_view{digest}.substr(57)};

  const auto public_key{edwards_public_key_point(scalar_a, parameters, 57)};

  // dom4 is "SigEd448" followed by the zero pre-hash flag and an empty context
  std::string domain{"SigEd448"};
  domain.push_back('\x00');
  domain.push_back('\x00');

  // r = SHAKE256(dom4 || prefix || M) reduced, then R = [r]B
  std::string nonce_preimage{domain};
  const SecureStringScope nonce_preimage_scope{nonce_preimage};
  nonce_preimage.append(prefix);
  nonce_preimage.append(message);
  auto nonce_hash{shake256(nonce_preimage, 114)};
  const SecureStringScope nonce_hash_scope{nonce_hash};
  auto scalar_r{bignum_from_bytes_little_endian(nonce_hash)};
  const SecureBignumScope scalar_r_scope{scalar_r};
  bignum_reduce(scalar_r, parameters.order);
  const auto encoded_r{
      edwards_point_encode(edwards_point_scalar_multiply_constant_time(
                               scalar_r, parameters.base, parameters),
                           parameters, 57)};

  // k = SHAKE256(dom4 || R || A || M) reduced, then S = (r + k * a) mod L. The
  // k * a product carries the secret scalar, so it is wiped; r, k, and the
  // resulting S are the public signature material
  std::string challenge_preimage{domain};
  challenge_preimage.append(encoded_r);
  challenge_preimage.append(public_key);
  challenge_preimage.append(message);
  auto scalar_k{
      bignum_from_bytes_little_endian(shake256(challenge_preimage, 114))};
  bignum_reduce(scalar_k, parameters.order);
  // The k * a product and its sum with the nonce carry the secret scalar and
  // nonce, so both run over the constant-time field arithmetic modulo the
  // order; k is public and r, k, and the resulting S are the public signature
  // material
  const auto &order_field{parameters.order_field};
  auto scalar_a_reduced{barrett_reduce(scalar_a, order_field)};
  const SecureBignumScope scalar_a_reduced_scope{scalar_a_reduced};
  auto challenge_product{
      field_mod_multiply_ct(scalar_k, scalar_a_reduced, order_field)};
  const SecureBignumScope challenge_product_scope{challenge_product};
  auto scalar_s{field_add_ct(scalar_r, challenge_product, order_field)};
  bignum_normalize(scalar_s);

  const auto scalar_s_big_endian{bignum_to_bytes(scalar_s, 57)};
  std::string signature{encoded_r};
  signature.append(scalar_s_big_endian.rbegin(), scalar_s_big_endian.rend());
  return signature;
}

} // namespace sourcemeta::core

#endif
