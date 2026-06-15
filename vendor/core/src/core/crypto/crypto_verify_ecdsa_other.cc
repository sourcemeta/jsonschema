#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_bignum.h"
#include "crypto_ecc.h"
#include "crypto_helpers.h"

#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto to_curve_parameters(const sourcemeta::core::EllipticCurve curve)
    -> sourcemeta::core::EllipticCurveParameters {
  switch (curve) {
    case sourcemeta::core::EllipticCurve::P256:
      return sourcemeta::core::curve_p256();
    case sourcemeta::core::EllipticCurve::P384:
      return sourcemeta::core::curve_p384();
    case sourcemeta::core::EllipticCurve::P521:
      return sourcemeta::core::curve_p521();
  }

  std::unreachable();
}

// FIPS 186-4 Section 6.4 step 2, deriving the integer e from the leftmost
// bits of the message digest, truncated to the bit length of the order
auto digest_to_integer(const sourcemeta::core::SignatureHashFunction hash,
                       const std::string_view message,
                       const std::size_t order_bits)
    -> sourcemeta::core::Bignum {
  const auto digest{sourcemeta::core::digest_message(hash, message)};
  auto value{sourcemeta::core::bignum_from_bytes(digest)};
  const auto digest_bits{digest.size() * 8};
  if (digest_bits > order_bits) {
    value =
        sourcemeta::core::bignum_shift_right(value, digest_bits - order_bits);
  }

  return value;
}

} // namespace

namespace sourcemeta::core {

auto ecdsa_verify(const EllipticCurve curve, const SignatureHashFunction hash,
                  const std::string_view coordinate_x,
                  const std::string_view coordinate_y,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto parameters{to_curve_parameters(curve)};
  const auto field_bytes{parameters.field_bytes};

  // RFC 7518 Section 3.4: the signature is the fixed-width concatenation of
  // the two integers, each as long as the curve field
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

  // Reject coordinates wider than the field, matching the platform backends
  // and preventing an oversized input from being truncated into a valid key
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  if (stripped_x.size() > field_bytes || stripped_y.size() > field_bytes) {
    return false;
  }

  const auto public_x{bignum_from_bytes(stripped_x)};
  const auto public_y{bignum_from_bytes(stripped_y)};

  // The public key must be a valid point: coordinates below the field prime
  // and satisfying the curve equation
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

  const JacobianPoint generator{parameters.generator_x, parameters.generator_y,
                                bignum_from_u64(1)};
  const JacobianPoint public_point{public_x, public_y, bignum_from_u64(1)};
  const auto point{point_add(
      point_scalar_multiply(u1, generator, parameters),
      point_scalar_multiply(u2, public_point, parameters), parameters)};

  // FIPS 186-4 Section 6.4.2 step 6: reject when the combination is the
  // point at infinity
  if (point_is_infinity(point)) {
    return false;
  }

  // FIPS 186-4 Section 6.4.2 step 7: the signature is valid when the affine
  // x coordinate, reduced modulo the order, equals r
  auto candidate{point_affine_x(point, parameters)};
  bignum_reduce(candidate, parameters.order);
  return bignum_compare(candidate, r) == 0;
}

} // namespace sourcemeta::core
