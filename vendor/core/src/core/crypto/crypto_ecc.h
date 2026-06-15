#ifndef SOURCEMETA_CORE_CRYPTO_ECC_H_
#define SOURCEMETA_CORE_CRYPTO_ECC_H_

// Short Weierstrass elliptic curve arithmetic over the NIST prime curves
// for the reference signature verification backend. Points are kept in
// Jacobian coordinates so that scalar multiplication needs a single modular
// inversion at the end rather than one per step. Performance and constant
// time execution are non-goals, verification consumes only public inputs

#include "crypto_bignum.h"

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

struct EllipticCurveParameters {
  Bignum prime;
  Bignum coefficient_a;
  Bignum coefficient_b;
  Bignum generator_x;
  Bignum generator_y;
  Bignum order;
  std::size_t field_bytes;
};

// A point in Jacobian coordinates, where the affine point is
// (X / Z^2, Y / Z^3). A zero Z marks the point at infinity
struct JacobianPoint {
  Bignum x;
  Bignum y;
  Bignum z;
};

// FIPS 186-4 Appendix D.1.2 curve domain parameters
inline auto curve_p256() -> EllipticCurveParameters {
  return {bignum_from_hex("ffffffff00000001000000000000000000000000ffffffff"
                          "ffffffffffffffff"),
          bignum_from_hex("ffffffff00000001000000000000000000000000ffffffff"
                          "fffffffffffffffc"),
          bignum_from_hex("5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f6"
                          "3bce3c3e27d2604b"),
          bignum_from_hex("6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0"
                          "f4a13945d898c296"),
          bignum_from_hex("4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ece"
                          "cbb6406837bf51f5"),
          bignum_from_hex("ffffffff00000000ffffffffffffffffbce6faada7179e84"
                          "f3b9cac2fc632551"),
          32};
}

inline auto curve_p384() -> EllipticCurveParameters {
  // The hexadecimal constants are single string literals so that no digit
  // is ever lost across a line break
  // clang-format off
  return {
      bignum_from_hex("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff"),
      bignum_from_hex("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000fffffffc"),
      bignum_from_hex("b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef"),
      bignum_from_hex("aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7"),
      bignum_from_hex("3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f"),
      bignum_from_hex("ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973"),
      48};
  // clang-format on
}

inline auto curve_p521() -> EllipticCurveParameters {
  // clang-format off
  return {
      bignum_from_hex("01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"),
      bignum_from_hex("01fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffc"),
      bignum_from_hex("0051953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00"),
      bignum_from_hex("00c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66"),
      bignum_from_hex("011839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650"),
      bignum_from_hex("01fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409"),
      66};
  // clang-format on
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
    return {Bignum{}, Bignum{}, Bignum{}};
  }

  const auto &prime{curve.prime};
  const auto two{bignum_from_u64(2)};
  const auto three{bignum_from_u64(3)};
  const auto y_squared{bignum_mod_multiply(point.y, point.y, prime)};
  auto subterm{bignum_mod_multiply(point.x, y_squared, prime)};
  subterm = bignum_mod_multiply(bignum_from_u64(4), subterm, prime);
  const auto x_squared{bignum_mod_multiply(point.x, point.x, prime)};
  const auto z_squared{bignum_mod_multiply(point.z, point.z, prime)};
  const auto z_fourth{bignum_mod_multiply(z_squared, z_squared, prime)};
  const auto slope{bignum_mod_add(
      bignum_mod_multiply(three, x_squared, prime),
      bignum_mod_multiply(curve.coefficient_a, z_fourth, prime), prime)};
  const auto result_x{
      bignum_mod_subtract(bignum_mod_multiply(slope, slope, prime),
                          bignum_mod_multiply(two, subterm, prime), prime)};
  const auto y_fourth{bignum_mod_multiply(y_squared, y_squared, prime)};
  const auto result_y{bignum_mod_subtract(
      bignum_mod_multiply(slope, bignum_mod_subtract(subterm, result_x, prime),
                          prime),
      bignum_mod_multiply(bignum_from_u64(8), y_fourth, prime), prime)};
  const auto result_z{bignum_mod_multiply(
      bignum_mod_multiply(two, point.y, prime), point.z, prime)};
  return {result_x, result_y, result_z};
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
  const auto left_z_squared{bignum_mod_multiply(left.z, left.z, prime)};
  const auto right_z_squared{bignum_mod_multiply(right.z, right.z, prime)};
  const auto u1{bignum_mod_multiply(left.x, right_z_squared, prime)};
  const auto u2{bignum_mod_multiply(right.x, left_z_squared, prime)};
  const auto left_z_cubed{bignum_mod_multiply(left_z_squared, left.z, prime)};
  const auto right_z_cubed{
      bignum_mod_multiply(right_z_squared, right.z, prime)};
  const auto s1{bignum_mod_multiply(left.y, right_z_cubed, prime)};
  const auto s2{bignum_mod_multiply(right.y, left_z_cubed, prime)};

  if (bignum_compare(u1, u2) == 0) {
    if (bignum_compare(s1, s2) != 0) {
      return {Bignum{}, Bignum{}, Bignum{}};
    }

    return point_double(left, curve);
  }

  const auto h{bignum_mod_subtract(u2, u1, prime)};
  const auto r{bignum_mod_subtract(s2, s1, prime)};
  const auto h_squared{bignum_mod_multiply(h, h, prime)};
  const auto h_cubed{bignum_mod_multiply(h_squared, h, prime)};
  const auto u1_h_squared{bignum_mod_multiply(u1, h_squared, prime)};
  const auto result_x{bignum_mod_subtract(
      bignum_mod_subtract(bignum_mod_multiply(r, r, prime), h_cubed, prime),
      bignum_mod_multiply(bignum_from_u64(2), u1_h_squared, prime), prime)};
  const auto result_y{bignum_mod_subtract(
      bignum_mod_multiply(r, bignum_mod_subtract(u1_h_squared, result_x, prime),
                          prime),
      bignum_mod_multiply(s1, h_cubed, prime), prime)};
  const auto result_z{bignum_mod_multiply(bignum_mod_multiply(h, left.z, prime),
                                          right.z, prime)};
  return {result_x, result_y, result_z};
}

inline auto point_scalar_multiply(const Bignum &scalar,
                                  const JacobianPoint &point,
                                  const EllipticCurveParameters &curve)
    -> JacobianPoint {
  JacobianPoint result{Bignum{}, Bignum{}, Bignum{}};
  const auto bits{bignum_bit_length(scalar)};
  for (std::size_t index = bits; index > 0; --index) {
    result = point_double(result, curve);
    if (bignum_get_bit(scalar, index - 1)) {
      result = point_add(result, point, curve);
    }
  }

  return result;
}

// Recover the affine x coordinate (X / Z^2) of a Jacobian point
inline auto point_affine_x(const JacobianPoint &point,
                           const EllipticCurveParameters &curve) -> Bignum {
  const auto z_inverse{bignum_mod_inverse(point.z, curve.prime)};
  const auto z_inverse_squared{
      bignum_mod_multiply(z_inverse, z_inverse, curve.prime)};
  return bignum_mod_multiply(point.x, z_inverse_squared, curve.prime);
}

// Whether the affine point satisfies y^2 = x^3 + a*x + b (mod p)
inline auto point_on_curve(const Bignum &x, const Bignum &y,
                           const EllipticCurveParameters &curve) -> bool {
  const auto &prime{curve.prime};
  const auto left{bignum_mod_multiply(y, y, prime)};
  const auto x_cubed{
      bignum_mod_multiply(bignum_mod_multiply(x, x, prime), x, prime)};
  const auto right{bignum_mod_add(
      bignum_mod_add(x_cubed,
                     bignum_mod_multiply(curve.coefficient_a, x, prime), prime),
      curve.coefficient_b, prime)};
  return bignum_compare(left, right) == 0;
}

} // namespace sourcemeta::core

#endif
