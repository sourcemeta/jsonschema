#ifndef SOURCEMETA_CORE_CRYPTO_ECC_H_
#define SOURCEMETA_CORE_CRYPTO_ECC_H_

// Short Weierstrass elliptic curve arithmetic over the NIST prime curves for
// the reference signature backend. Verification keeps points in Jacobian
// coordinates so that scalar multiplication needs a single modular inversion at
// the end, and consumes only public inputs, so it stays variable time. Signing
// uses the constant-time scalar multiplication below, a fixed-length ladder
// over the complete projective formula evaluated with the constant-time field
// layer, so it does not depend on the secret scalar

#include "crypto_bignum.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Identifies the field prime so that modular reduction can take the fast
// generalized Mersenne path specific to each NIST curve
enum class NISTPrime : std::uint8_t { P256, P384, P521 };

struct EllipticCurveParameters {
  CurveBignum prime;
  CurveBignum coefficient_a;
  CurveBignum coefficient_b;
  CurveBignum generator_x;
  CurveBignum generator_y;
  CurveBignum order;
  std::size_t field_bytes;
  NISTPrime reduction;
};

// A point in Jacobian coordinates, where the affine point is
// (X / Z^2, Y / Z^3). A zero Z marks the point at infinity
struct JacobianPoint {
  CurveBignum x;
  CurveBignum y;
  CurveBignum z;
};

// FIPS 186-4 Appendix D.1.2 curve domain parameters
inline auto curve_p256() -> EllipticCurveParameters {
  return {.prime = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
              "ffffffff00000001000000000000000000000000ffffffff"
              "ffffffffffffffff"),
          .coefficient_a = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
              "ffffffff00000001000000000000000000000000ffffffff"
              "fffffffffffffffc"),
          .coefficient_b = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
              "5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f6"
              "3bce3c3e27d2604b"),
          .generator_x = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
              "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0"
              "f4a13945d898c296"),
          .generator_y = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
              "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ece"
              "cbb6406837bf51f5"),
          .order = bignum_from_hex<CURVE_BIGNUM_CAPACITY>(
              "ffffffff00000000ffffffffffffffffbce6faada7179e84"
              "f3b9cac2fc632551"),
          .field_bytes = 32,
          .reduction = NISTPrime::P256};
}

inline auto curve_p384() -> EllipticCurveParameters {
  // The hexadecimal constants are single string literals so that no digit
  // is ever lost across a line break
  // clang-format off
  return {
      .prime = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff"),
      .coefficient_a = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000fffffffc"),
      .coefficient_b = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef"),
      .generator_x = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7"),
      .generator_y = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f"),
      .order = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973"),
      .field_bytes = 48,
      .reduction = NISTPrime::P384};
  // clang-format on
}

inline auto curve_p521() -> EllipticCurveParameters {
  // clang-format off
  return {
      .prime = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"),
      .coefficient_a = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("01fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffc"),
      .coefficient_b = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("0051953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00"),
      .generator_x = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("00c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66"),
      .generator_y = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("011839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650"),
      .order = bignum_from_hex<CURVE_BIGNUM_CAPACITY>("01fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409"),
      .field_bytes = 66,
      .reduction = NISTPrime::P521};
  // clang-format on
}

// NIST P-521 field reduction. The prime is 2^521 - 1, so 2^521 is congruent
// to 1 modulo it, and a value below the square of the prime folds the bits
// above position 521 back into the low 521 bits with a single addition
inline auto field_reduce_p521(CurveBignum &value,
                              const CurveBignum &prime) noexcept -> void {
  auto high{bignum_shift_right(value, 521)};
  if (value.size > 8) {
    value.words[8] &= 0x1ffULL;
    for (std::size_t index = 9; index < value.size; ++index) {
      value.words[index] = 0;
    }

    value.size = 9;
    bignum_normalize(value);
  }

  value = bignum_add(value, high);
  while (bignum_compare(value, prime) >= 0) {
    bignum_subtract_in_place(value, prime);
  }
}

// Read the 32-bit limb at the given position, counting from the least
// significant, returning zero past the end of the value
inline auto field_word(const CurveBignum &value,
                       const std::size_t index) noexcept -> std::uint64_t {
  const auto word{index / 2};
  if (word >= value.size) {
    return 0;
  }

  return (value.words[word] >> (32 * (index % 2))) & 0xffffffffULL;
}

// Add a reduction term, given as its 32-bit limbs most significant first,
// into the column accumulator scaled by the multiplier. Column zero is the
// least significant, and accumulating into 64-bit columns rather than
// materializing a value per term keeps the reduction free of temporaries
template <std::size_t Count>
inline auto field_accumulate(std::array<std::uint64_t, Count + 1> &columns,
                             const std::array<std::uint64_t, Count> &limbs,
                             const std::uint64_t multiplier) noexcept -> void {
  auto *columns_data{columns.data()};
  const auto *limbs_data{limbs.data()};
  for (std::size_t index = 0; index < Count; ++index) {
    columns_data[Count - 1 - index] += multiplier * limbs_data[index];
  }
}

// Carry propagate the 32-bit columns and pack them into a value
template <std::size_t Count>
inline auto field_from_columns(std::array<std::uint64_t, Count + 1> &columns)
    -> CurveBignum {
  std::uint64_t carry{0};
  CurveBignum result;
  const auto *columns_data{columns.data()};
  auto *result_data{result.words.data()};
  for (std::size_t index = 0; index <= Count; ++index) {
    const auto current{columns_data[index] + carry};
    result_data[index / 2] |= (current & 0xffffffffULL) << (32 * (index % 2));
    carry = current >> 32;
  }

  result.size = (Count + 2) / 2;
  bignum_normalize(result);
  return result;
}

// Combine the positive and negative column sums of a generalized Mersenne
// reduction into the single reduced value below the prime
inline auto field_combine(CurveBignum &positive, const CurveBignum &negative,
                          const CurveBignum &prime) noexcept -> void {
  while (bignum_compare(positive, negative) < 0) {
    positive = bignum_add(positive, prime);
  }

  bignum_subtract_in_place(positive, negative);
  while (bignum_compare(positive, prime) >= 0) {
    bignum_subtract_in_place(positive, prime);
  }
}

// NIST P-256 field reduction. The prime is 2^256 - 2^224 + 2^192 + 2^96 - 1,
// a generalized Mersenne prime whose reduction recombines the 32-bit limbs of
// the product into a small signed sum of nine field-width terms
// (FIPS 186-4 Appendix D.2.3)
inline auto field_reduce_p256(CurveBignum &value,
                              const CurveBignum &prime) noexcept -> void {
  std::array<std::uint64_t, 16> limbs{};
  auto *c{limbs.data()};
  for (std::size_t index = 0; index < 16; ++index) {
    c[index] = field_word(value, index);
  }

  std::array<std::uint64_t, 9> positive_columns{};
  std::array<std::uint64_t, 9> negative_columns{};
  field_accumulate<8>(positive_columns,
                      {{c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0]}}, 1);
  field_accumulate<8>(positive_columns,
                      {{c[15], c[14], c[13], c[12], c[11], 0, 0, 0}}, 2);
  field_accumulate<8>(positive_columns,
                      {{0, c[15], c[14], c[13], c[12], 0, 0, 0}}, 2);
  field_accumulate<8>(positive_columns,
                      {{c[15], c[14], 0, 0, 0, c[10], c[9], c[8]}}, 1);
  field_accumulate<8>(positive_columns,
                      {{c[8], c[13], c[15], c[14], c[13], c[11], c[10], c[9]}},
                      1);
  field_accumulate<8>(negative_columns,
                      {{c[10], c[8], 0, 0, 0, c[13], c[12], c[11]}}, 1);
  field_accumulate<8>(negative_columns,
                      {{c[11], c[9], 0, 0, c[15], c[14], c[13], c[12]}}, 1);
  field_accumulate<8>(negative_columns,
                      {{c[12], 0, c[10], c[9], c[8], c[15], c[14], c[13]}}, 1);
  field_accumulate<8>(negative_columns,
                      {{c[13], 0, c[11], c[10], c[9], 0, c[15], c[14]}}, 1);

  auto positive{field_from_columns<8>(positive_columns)};
  const auto negative{field_from_columns<8>(negative_columns)};
  field_combine(positive, negative, prime);
  value = positive;
}

// NIST P-384 field reduction. The prime is 2^384 - 2^128 - 2^96 + 2^32 - 1,
// a generalized Mersenne prime whose reduction recombines the 32-bit limbs of
// the product into a small signed sum of ten field-width terms
// (FIPS 186-4 Appendix D.2.4)
inline auto field_reduce_p384(CurveBignum &value,
                              const CurveBignum &prime) noexcept -> void {
  std::array<std::uint64_t, 24> limbs{};
  auto *c{limbs.data()};
  for (std::size_t index = 0; index < 24; ++index) {
    c[index] = field_word(value, index);
  }

  std::array<std::uint64_t, 13> positive_columns{};
  std::array<std::uint64_t, 13> negative_columns{};
  field_accumulate<12>(positive_columns,
                       {{c[11], c[10], c[9], c[8], c[7], c[6], c[5], c[4], c[3],
                         c[2], c[1], c[0]}},
                       1);
  field_accumulate<12>(positive_columns,
                       {{0, 0, 0, 0, 0, c[23], c[22], c[21], 0, 0, 0, 0}}, 2);
  field_accumulate<12>(positive_columns,
                       {{c[23], c[22], c[21], c[20], c[19], c[18], c[17], c[16],
                         c[15], c[14], c[13], c[12]}},
                       1);
  field_accumulate<12>(positive_columns,
                       {{c[20], c[19], c[18], c[17], c[16], c[15], c[14], c[13],
                         c[12], c[23], c[22], c[21]}},
                       1);
  field_accumulate<12>(positive_columns,
                       {{c[19], c[18], c[17], c[16], c[15], c[14], c[13], c[12],
                         c[20], 0, c[23], 0}},
                       1);
  field_accumulate<12>(positive_columns,
                       {{0, 0, 0, 0, c[23], c[22], c[21], c[20], 0, 0, 0, 0}},
                       1);
  field_accumulate<12>(positive_columns,
                       {{0, 0, 0, 0, 0, 0, c[23], c[22], c[21], 0, 0, c[20]}},
                       1);
  field_accumulate<12>(negative_columns,
                       {{c[22], c[21], c[20], c[19], c[18], c[17], c[16], c[15],
                         c[14], c[13], c[12], c[23]}},
                       1);
  field_accumulate<12>(negative_columns,
                       {{0, 0, 0, 0, 0, 0, 0, c[23], c[22], c[21], c[20], 0}},
                       1);
  field_accumulate<12>(negative_columns,
                       {{0, 0, 0, 0, 0, 0, 0, c[23], c[23], 0, 0, 0}}, 1);

  auto positive{field_from_columns<12>(positive_columns)};
  const auto negative{field_from_columns<12>(negative_columns)};
  field_combine(positive, negative, prime);
  value = positive;
}

// Reduce a product below the square of the field prime, taking the fast
// generalized Mersenne path for the curve rather than long division
inline auto field_reduce(CurveBignum &value,
                         const EllipticCurveParameters &curve) noexcept
    -> void {
  switch (curve.reduction) {
    case NISTPrime::P521:
      field_reduce_p521(value, curve.prime);
      return;
    case NISTPrime::P256:
      field_reduce_p256(value, curve.prime);
      return;
    case NISTPrime::P384:
      field_reduce_p384(value, curve.prime);
      return;
  }
}

inline auto field_mod_multiply(const CurveBignum &left,
                               const CurveBignum &right,
                               const EllipticCurveParameters &curve) noexcept
    -> CurveBignum {
  auto result{bignum_multiply(left, right)};
  field_reduce(result, curve);
  return result;
}

inline auto field_square(const CurveBignum &value,
                         const EllipticCurveParameters &curve) noexcept
    -> CurveBignum {
  auto result{bignum_square_fixed(value, value.size)};
  bignum_normalize(result);
  field_reduce(result, curve);
  return result;
}

inline auto point_is_infinity(const JacobianPoint &point) noexcept -> bool {
  return bignum_is_zero(point.z);
}

// Point doubling in Jacobian coordinates (the general short Weierstrass
// formulas, which hold for the NIST curves where the coefficient is -3)
inline auto point_double(const JacobianPoint &point,
                         const EllipticCurveParameters &curve)
    -> JacobianPoint {
  if (point_is_infinity(point) || bignum_is_zero(point.y)) {
    return {};
  }

  // The doubling formula for curves with coefficient -3 (EFD dbl-2001-b),
  // which trades the coefficient multiplication and a squaring for one more
  // subtraction, and computes every small multiple as a chain of modular
  // additions so that no division-based reduction is spent on a constant
  const auto &prime{curve.prime};
  const auto delta{field_square(point.z, curve)};
  const auto gamma{field_square(point.y, curve)};
  const auto beta{field_mod_multiply(point.x, gamma, curve)};
  const auto difference{
      field_mod_multiply(bignum_mod_subtract(point.x, delta, prime),
                         bignum_mod_add(point.x, delta, prime), curve)};
  const auto alpha{bignum_mod_add(bignum_mod_add(difference, difference, prime),
                                  difference, prime)};
  const auto two_beta{bignum_mod_add(beta, beta, prime)};
  const auto four_beta{bignum_mod_add(two_beta, two_beta, prime)};
  const auto eight_beta{bignum_mod_add(four_beta, four_beta, prime)};
  const auto result_x{
      bignum_mod_subtract(field_square(alpha, curve), eight_beta, prime)};
  const auto y_plus_z{bignum_mod_add(point.y, point.z, prime)};
  const auto result_z{bignum_mod_subtract(
      bignum_mod_subtract(field_square(y_plus_z, curve), gamma, prime), delta,
      prime)};
  const auto gamma_squared{field_square(gamma, curve)};
  const auto two_gamma_squared{
      bignum_mod_add(gamma_squared, gamma_squared, prime)};
  const auto four_gamma_squared{
      bignum_mod_add(two_gamma_squared, two_gamma_squared, prime)};
  const auto eight_gamma_squared{
      bignum_mod_add(four_gamma_squared, four_gamma_squared, prime)};
  const auto result_y{bignum_mod_subtract(
      field_mod_multiply(alpha, bignum_mod_subtract(four_beta, result_x, prime),
                         curve),
      eight_gamma_squared, prime)};
  return {.x = result_x, .y = result_y, .z = result_z};
}

// Point addition in Jacobian coordinates
inline auto point_add(const JacobianPoint &left, const JacobianPoint &right,
                      const EllipticCurveParameters &curve) -> JacobianPoint {
  if (point_is_infinity(left)) {
    return right;
  }

  if (point_is_infinity(right)) {
    return left;
  }

  const auto &prime{curve.prime};
  const auto left_z_squared{field_mod_multiply(left.z, left.z, curve)};
  const auto right_z_squared{field_mod_multiply(right.z, right.z, curve)};
  const auto u1{field_mod_multiply(left.x, right_z_squared, curve)};
  const auto u2{field_mod_multiply(right.x, left_z_squared, curve)};
  const auto left_z_cubed{field_mod_multiply(left_z_squared, left.z, curve)};
  const auto right_z_cubed{field_mod_multiply(right_z_squared, right.z, curve)};
  const auto s1{field_mod_multiply(left.y, right_z_cubed, curve)};
  const auto s2{field_mod_multiply(right.y, left_z_cubed, curve)};

  if (bignum_compare(u1, u2) == 0) {
    if (bignum_compare(s1, s2) != 0) {
      return {};
    }

    return point_double(left, curve);
  }

  const auto h{bignum_mod_subtract(u2, u1, prime)};
  const auto r{bignum_mod_subtract(s2, s1, prime)};
  const auto h_squared{field_square(h, curve)};
  const auto h_cubed{field_mod_multiply(h_squared, h, curve)};
  const auto u1_h_squared{field_mod_multiply(u1, h_squared, curve)};
  const auto result_x{bignum_mod_subtract(
      bignum_mod_subtract(field_square(r, curve), h_cubed, prime),
      field_mod_multiply(bignum_from_u64<CURVE_BIGNUM_CAPACITY>(2),
                         u1_h_squared, curve),
      prime)};
  const auto result_y{bignum_mod_subtract(
      field_mod_multiply(r, bignum_mod_subtract(u1_h_squared, result_x, prime),
                         curve),
      field_mod_multiply(s1, h_cubed, curve), prime)};
  const auto result_z{
      field_mod_multiply(field_mod_multiply(h, left.z, curve), right.z, curve)};
  return {.x = result_x, .y = result_y, .z = result_z};
}

// Add a Jacobian point and an affine point whose Z coordinate is one, the case
// that arises when accumulating the fixed input points in the combined ladder.
// Skipping the second point's Z powers saves several multiplications over the
// general addition (EFD madd-2007-bl)
inline auto point_add_mixed(const JacobianPoint &left,
                            const JacobianPoint &right,
                            const EllipticCurveParameters &curve)
    -> JacobianPoint {
  if (point_is_infinity(left)) {
    return right;
  }

  if (point_is_infinity(right)) {
    return left;
  }

  const auto &prime{curve.prime};
  const auto z_squared{field_square(left.z, curve)};
  const auto u2{field_mod_multiply(right.x, z_squared, curve)};
  const auto s2{field_mod_multiply(
      right.y, field_mod_multiply(left.z, z_squared, curve), curve)};
  const auto h{bignum_mod_subtract(u2, left.x, prime)};

  if (bignum_is_zero(h)) {
    if (bignum_compare(s2, left.y) == 0) {
      return point_double(left, curve);
    }

    return {};
  }

  const auto h_squared{field_square(h, curve)};
  const auto two_h_squared{bignum_mod_add(h_squared, h_squared, prime)};
  const auto scaled_h_squared{
      bignum_mod_add(two_h_squared, two_h_squared, prime)};
  const auto j{field_mod_multiply(h, scaled_h_squared, curve)};
  const auto s_difference{bignum_mod_subtract(s2, left.y, prime)};
  const auto r{bignum_mod_add(s_difference, s_difference, prime)};
  const auto v{field_mod_multiply(left.x, scaled_h_squared, curve)};
  const auto two_v{bignum_mod_add(v, v, prime)};
  const auto result_x{bignum_mod_subtract(
      bignum_mod_subtract(field_mod_multiply(r, r, curve), j, prime), two_v,
      prime)};
  const auto y_j{field_mod_multiply(left.y, j, curve)};
  const auto two_y_j{bignum_mod_add(y_j, y_j, prime)};
  const auto result_y{bignum_mod_subtract(
      field_mod_multiply(r, bignum_mod_subtract(v, result_x, prime), curve),
      two_y_j, prime)};
  const auto z_plus_h{bignum_mod_add(left.z, h, prime)};
  const auto result_z{bignum_mod_subtract(
      bignum_mod_subtract(field_square(z_plus_h, curve), z_squared, prime),
      h_squared, prime)};
  return {.x = result_x, .y = result_y, .z = result_z};
}

// Normalize a Jacobian point to the affine representation with Z coordinate
// one, so that later additions can take the cheaper mixed path
inline auto point_to_affine(const JacobianPoint &point,
                            const EllipticCurveParameters &curve)
    -> JacobianPoint {
  if (point_is_infinity(point)) {
    return {};
  }

  const auto z_inverse{bignum_mod_inverse(point.z, curve.prime)};
  const auto z_inverse_squared{field_square(z_inverse, curve)};
  const auto z_inverse_cubed{
      field_mod_multiply(z_inverse_squared, z_inverse, curve)};
  return {.x = field_mod_multiply(point.x, z_inverse_squared, curve),
          .y = field_mod_multiply(point.y, z_inverse_cubed, curve),
          .z = bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1)};
}

// Compute scalar_one * point_one + scalar_two * point_two with Shamir's trick,
// a single double-and-add over the longer scalar that adds a precomputed sum
// whenever both scalars have a set bit, halving the doublings of two separate
// scalar multiplications. The three addable points are kept affine so every
// step takes the mixed addition
inline auto point_double_scalar_multiply(const CurveBignum &scalar_one,
                                         const JacobianPoint &point_one,
                                         const CurveBignum &scalar_two,
                                         const JacobianPoint &point_two,
                                         const EllipticCurveParameters &curve)
    -> JacobianPoint {
  const auto combined{
      point_to_affine(point_add(point_one, point_two, curve), curve)};
  JacobianPoint result{};
  const auto bits_one{bignum_bit_length(scalar_one)};
  const auto bits_two{bignum_bit_length(scalar_two)};
  const auto bits{bits_one > bits_two ? bits_one : bits_two};
  for (std::size_t index = bits; index > 0; --index) {
    result = point_double(result, curve);
    const auto bit_one{bignum_get_bit(scalar_one, index - 1)};
    const auto bit_two{bignum_get_bit(scalar_two, index - 1)};
    if (bit_one && bit_two) {
      result = point_add_mixed(result, combined, curve);
    } else if (bit_one) {
      result = point_add_mixed(result, point_one, curve);
    } else if (bit_two) {
      result = point_add_mixed(result, point_two, curve);
    }
  }

  return result;
}

// Recover the affine x coordinate (X / Z^2) of a Jacobian point
inline auto point_affine_x(const JacobianPoint &point,
                           const EllipticCurveParameters &curve)
    -> CurveBignum {
  const auto z_inverse{bignum_mod_inverse(point.z, curve.prime)};
  const auto z_inverse_squared{field_square(z_inverse, curve)};
  return field_mod_multiply(point.x, z_inverse_squared, curve);
}

inline auto point_conditional_select(const bool condition,
                                     const JacobianPoint &when_true,
                                     const JacobianPoint &when_false,
                                     const std::size_t words) noexcept
    -> JacobianPoint {
  return {.x = bignum_conditional_select(condition, when_true.x, when_false.x,
                                         words),
          .y = bignum_conditional_select(condition, when_true.y, when_false.y,
                                         words),
          .z = bignum_conditional_select(condition, when_true.z, when_false.z,
                                         words)};
}

// Complete projective point addition for the prime-order NIST curves, whose
// coefficient a is negative three (Renes, Costello, and Batina 2016, Algorithm
// 4). It has no exceptional cases, so the signing ladder needs no
// secret-dependent branch, and it also doubles, so one routine serves both
// ladder steps. Coordinates here are projective, so the affine point is X / Z
inline auto point_complete_add(const JacobianPoint &left,
                               const JacobianPoint &right,
                               const CurveBignum &coefficient_b,
                               const CurveBarrettContext &field) noexcept
    -> JacobianPoint {
  auto t0{field_mod_multiply_ct(left.x, right.x, field)};
  auto t1{field_mod_multiply_ct(left.y, right.y, field)};
  auto t2{field_mod_multiply_ct(left.z, right.z, field)};
  auto t3{field_add_ct(left.x, left.y, field)};
  auto t4{field_add_ct(right.x, right.y, field)};
  t3 = field_mod_multiply_ct(t3, t4, field);
  t4 = field_add_ct(t0, t1, field);
  t3 = field_subtract_ct(t3, t4, field);
  t4 = field_add_ct(left.y, left.z, field);
  auto x3{field_add_ct(right.y, right.z, field)};
  t4 = field_mod_multiply_ct(t4, x3, field);
  x3 = field_add_ct(t1, t2, field);
  t4 = field_subtract_ct(t4, x3, field);
  x3 = field_add_ct(left.x, left.z, field);
  auto y3{field_add_ct(right.x, right.z, field)};
  x3 = field_mod_multiply_ct(x3, y3, field);
  y3 = field_add_ct(t0, t2, field);
  y3 = field_subtract_ct(x3, y3, field);
  auto z3{field_mod_multiply_ct(coefficient_b, t2, field)};
  x3 = field_subtract_ct(y3, z3, field);
  z3 = field_add_ct(x3, x3, field);
  x3 = field_add_ct(x3, z3, field);
  z3 = field_subtract_ct(t1, x3, field);
  x3 = field_add_ct(t1, x3, field);
  y3 = field_mod_multiply_ct(coefficient_b, y3, field);
  t1 = field_add_ct(t2, t2, field);
  t2 = field_add_ct(t1, t2, field);
  y3 = field_subtract_ct(y3, t2, field);
  y3 = field_subtract_ct(y3, t0, field);
  t1 = field_add_ct(y3, y3, field);
  y3 = field_add_ct(t1, y3, field);
  t1 = field_add_ct(t0, t0, field);
  t0 = field_add_ct(t1, t0, field);
  t0 = field_subtract_ct(t0, t2, field);
  t1 = field_mod_multiply_ct(t4, y3, field);
  t2 = field_mod_multiply_ct(t0, y3, field);
  y3 = field_mod_multiply_ct(x3, z3, field);
  y3 = field_add_ct(y3, t2, field);
  x3 = field_mod_multiply_ct(t3, x3, field);
  x3 = field_subtract_ct(x3, t1, field);
  z3 = field_mod_multiply_ct(t4, z3, field);
  t1 = field_mod_multiply_ct(t3, t0, field);
  z3 = field_add_ct(z3, t1, field);
  return {.x = x3, .y = y3, .z = z3};
}

// Exception-free projective point doubling for the same curves (Renes,
// Costello, and Batina 2016, Algorithm 6). It agrees with the complete addition
// of a point to itself, the identity included, for four fewer multiplications,
// three of the rest being squarings
inline auto point_complete_double(const JacobianPoint &point,
                                  const CurveBignum &coefficient_b,
                                  const CurveBarrettContext &field) noexcept
    -> JacobianPoint {
  auto t0{field_square_ct(point.x, field)};
  const auto t1{field_square_ct(point.y, field)};
  auto t2{field_square_ct(point.z, field)};
  auto t3{field_mod_multiply_ct(point.x, point.y, field)};
  t3 = field_add_ct(t3, t3, field);
  auto z3{field_mod_multiply_ct(point.x, point.z, field)};
  z3 = field_add_ct(z3, z3, field);
  auto y3{field_mod_multiply_ct(coefficient_b, t2, field)};
  y3 = field_subtract_ct(y3, z3, field);
  auto x3{field_add_ct(y3, y3, field)};
  y3 = field_add_ct(x3, y3, field);
  x3 = field_subtract_ct(t1, y3, field);
  y3 = field_add_ct(t1, y3, field);
  y3 = field_mod_multiply_ct(x3, y3, field);
  x3 = field_mod_multiply_ct(x3, t3, field);
  t3 = field_add_ct(t2, t2, field);
  t2 = field_add_ct(t2, t3, field);
  z3 = field_mod_multiply_ct(coefficient_b, z3, field);
  z3 = field_subtract_ct(z3, t2, field);
  z3 = field_subtract_ct(z3, t0, field);
  t3 = field_add_ct(z3, z3, field);
  z3 = field_add_ct(z3, t3, field);
  t3 = field_add_ct(t0, t0, field);
  t0 = field_add_ct(t3, t0, field);
  t0 = field_subtract_ct(t0, t2, field);
  t0 = field_mod_multiply_ct(t0, z3, field);
  y3 = field_add_ct(y3, t0, field);
  t0 = field_mod_multiply_ct(point.y, point.z, field);
  t0 = field_add_ct(t0, t0, field);
  z3 = field_mod_multiply_ct(t0, z3, field);
  x3 = field_subtract_ct(x3, z3, field);
  z3 = field_mod_multiply_ct(t0, t1, field);
  z3 = field_add_ct(z3, z3, field);
  z3 = field_add_ct(z3, z3, field);
  return {.x = x3, .y = y3, .z = z3};
}

// NIST P-521 field reduction in constant time, for the signing ladder. The
// prime is 2^521 - 1, so the bits of a product above position 521 fold back
// onto the low 521 bits with one fixed-width addition, and folding the bit 521
// of that sum once more leaves a value no greater than 2^521. Such a value is
// at least the prime exactly when adding one to it reaches bit 521, and that
// sum without bit 521 is then the reduced value, so a single masked selection
// finishes the reduction
inline auto field_reduce_p521_ct(
    const CurveBignum &value,
    [[maybe_unused]] const CurveBarrettContext &context) noexcept
    -> CurveBignum {
  const auto *value_data{value.words.data()};
  CurveBignum sum;
  auto *sum_data{sum.words.data()};
  std::uint64_t carry{0};
  for (std::size_t index = 0; index < 8; ++index) {
    const auto high{(value_data[index + 8] >> 9U) |
                    (value_data[index + 9] << 55U)};
    const auto total{static_cast<BignumDoubleWord>(value_data[index]) + high +
                     carry};
    sum_data[index] = static_cast<std::uint64_t>(total);
    carry = static_cast<std::uint64_t>(total >> 64U);
  }

  // The top word keeps only the nine bits below position 521, and the product
  // bits folded onto it stay below 2^9, so its sum cannot overflow
  sum_data[8] = (value_data[8] & 0x1ffULL) +
                ((value_data[16] >> 9U) | (value_data[17] << 55U)) + carry;

  std::uint64_t addend{sum_data[8] >> 9U};
  sum_data[8] &= 0x1ffULL;
  for (std::size_t index = 0; index < 9; ++index) {
    const auto total{static_cast<BignumDoubleWord>(sum_data[index]) + addend};
    sum_data[index] = static_cast<std::uint64_t>(total);
    addend = static_cast<std::uint64_t>(total >> 64U);
  }

  CurveBignum reduced;
  auto *reduced_data{reduced.words.data()};
  addend = 1;
  for (std::size_t index = 0; index < 9; ++index) {
    const auto total{static_cast<BignumDoubleWord>(sum_data[index]) + addend};
    reduced_data[index] = static_cast<std::uint64_t>(total);
    addend = static_cast<std::uint64_t>(total >> 64U);
  }

  const auto at_least_prime{(reduced_data[8] >> 9U) != 0};
  reduced_data[8] &= 0x1ffULL;
  sum.size = 9;
  reduced.size = 9;
  return bignum_conditional_select(at_least_prime, reduced, sum, 9);
}

// The constant-time field arithmetic context of a curve, taking the Mersenne
// reduction for P-521 over the generic Barrett one
inline auto curve_field_context(const EllipticCurveParameters &curve)
    -> CurveBarrettContext {
  auto field{barrett_context(curve.prime)};
  if (curve.reduction == NISTPrime::P521) {
    field.reduce = &field_reduce_p521_ct;
  }

  return field;
}

// The field arithmetic context of the signing ladder, taking the Mersenne
// reduction for P-521 and Montgomery form for the other curves, whose
// reduction is cheaper than the Barrett one
inline auto curve_ladder_context(const EllipticCurveParameters &curve)
    -> CurveBarrettContext {
  const auto field{curve_field_context(curve)};
  if (curve.reduction == NISTPrime::P521) {
    return field;
  }

  return montgomery_context(field);
}

// Move the coordinates of a projective point into the representation of a
// context, and back out of it
inline auto point_to_montgomery(const JacobianPoint &point,
                                const CurveBarrettContext &field) noexcept
    -> JacobianPoint {
  return {.x = field_to_montgomery_ct(point.x, field),
          .y = field_to_montgomery_ct(point.y, field),
          .z = field_to_montgomery_ct(point.z, field)};
}

inline auto point_from_montgomery(const JacobianPoint &point,
                                  const CurveBarrettContext &field) noexcept
    -> JacobianPoint {
  return {.x = field_from_montgomery_ct(point.x, field),
          .y = field_from_montgomery_ct(point.y, field),
          .z = field_from_montgomery_ct(point.z, field)};
}

// For the signing path, where the scalar is the secret nonce: a fixed four-bit
// window ladder over the complete formula. The window count is fixed by the
// public order length, every window doubles four times and adds one table entry
// taken through a masked scan over the whole table, and the complete formula
// absorbs the identity entry of a zero window, so neither the control flow nor
// the field arithmetic underneath depends on the scalar. The arithmetic runs in
// the representation of the ladder context, and the input point and the result
// are projective outside of it
inline auto point_scalar_multiply_constant_time(
    const CurveBignum &scalar, const JacobianPoint &point,
    const EllipticCurveParameters &curve) -> JacobianPoint {
  const auto field{curve_ladder_context(curve)};
  const auto coefficient_b{field_to_montgomery_ct(curve.coefficient_b, field)};
  const auto base_point{point_to_montgomery(point, field)};
  std::array<JacobianPoint, 16> multiples{};
  multiples[0] =
      JacobianPoint{.x = CurveBignum{},
                    .y = field_to_montgomery_ct(
                        bignum_from_u64<CURVE_BIGNUM_CAPACITY>(1), field),
                    .z = CurveBignum{}};
  multiples[1] = base_point;
  for (std::size_t index = 2; index < multiples.size(); ++index) {
    multiples[index] = point_complete_add(multiples[index - 1], base_point,
                                          coefficient_b, field);
  }

  auto result{multiples[0]};
  const auto windows{(bignum_bit_length(curve.order) + 3) / 4};
  for (std::size_t window = windows; window > 0; --window) {
    for (std::size_t step = 0; step < 4; ++step) {
      result = point_complete_double(result, coefficient_b, field);
    }

    std::size_t digit{0};
    for (std::size_t bit = 0; bit < 4; ++bit) {
      digit |= static_cast<std::size_t>(
                   bignum_get_bit_fixed(scalar, ((window - 1) * 4) + bit))
               << bit;
    }

    JacobianPoint selected{};
    for (std::size_t index = 0; index < multiples.size(); ++index) {
      selected = point_conditional_select(digit == index, multiples[index],
                                          selected, field.words);
    }

    result = point_complete_add(result, selected, coefficient_b, field);
  }

  return point_from_montgomery(result, field);
}

inline auto point_affine_x_constant_time(const JacobianPoint &point,
                                         const EllipticCurveParameters &curve)
    -> CurveBignum {
  const auto field{curve_field_context(curve)};
  const auto z_inverse{field_inverse_ct(point.z, field)};
  auto result{field_mod_multiply_ct(point.x, z_inverse, field)};
  bignum_normalize(result);
  return result;
}

// Whether the affine point satisfies y^2 = x^3 + a*x + b (mod p)
inline auto point_on_curve(const CurveBignum &x, const CurveBignum &y,
                           const EllipticCurveParameters &curve) -> bool {
  const auto &prime{curve.prime};
  const auto left{field_square(y, curve)};
  const auto x_cubed{field_mod_multiply(field_square(x, curve), x, curve)};
  const auto right{bignum_mod_add(
      bignum_mod_add(x_cubed, field_mod_multiply(curve.coefficient_a, x, curve),
                     prime),
      curve.coefficient_b, prime)};
  return bignum_compare(left, right) == 0;
}

} // namespace sourcemeta::core

#endif
