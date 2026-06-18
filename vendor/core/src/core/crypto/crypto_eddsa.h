#ifndef SOURCEMETA_CORE_CRYPTO_EDDSA_H_
#define SOURCEMETA_CORE_CRYPTO_EDDSA_H_

// Edwards-curve signature verification (Ed25519 and Ed448, RFC 8032 Section 5,
// the pure variants) for the backends without a native EdDSA primitive. Points
// are kept in extended Edwards coordinates, so that the group law is a single
// set of complete formulas shared by both curves. Constant time execution is
// not required, since verification consumes only public inputs

#include <sourcemeta/core/crypto_sha512.h>

#include "crypto_bignum.h"
#include "crypto_shake256.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// A point in extended Edwards coordinates (X : Y : Z : T), where the affine
// point is (X / Z, Y / Z) and T = X * Y / Z (RFC 8032 Section 5.1.4)
struct EdwardsPoint {
  Bignum x;
  Bignum y;
  Bignum z;
  Bignum t;
};

struct EdwardsParameters {
  Bignum prime;
  Bignum order;
  Bignum coefficient_a;
  Bignum coefficient_d;
  EdwardsPoint base;
};

// Interpret the bytes as a little-endian unsigned integer, the encoding EdDSA
// uses throughout (RFC 8032 Section 5.1.2), by reversing into the big-endian
// conversion
inline auto bignum_from_bytes_little_endian(const std::string_view input)
    -> Bignum {
  const std::string reversed{input.rbegin(), input.rend()};
  return bignum_from_bytes(reversed);
}

// The complete unified Edwards addition formulas in extended coordinates
// (Hisil, Wong, Carter, and Dawson 2008), which hold for any two points,
// including equal points and the identity, since the curve coefficient is a
// square and d is a non-square modulo p
inline auto edwards_point_add(const EdwardsPoint &left,
                              const EdwardsPoint &right,
                              const EdwardsParameters &parameters)
    -> EdwardsPoint {
  const auto &prime{parameters.prime};
  const auto a{bignum_mod_multiply(left.x, right.x, prime)};
  const auto b{bignum_mod_multiply(left.y, right.y, prime)};
  const auto c{bignum_mod_multiply(
      bignum_mod_multiply(parameters.coefficient_d, left.t, prime), right.t,
      prime)};
  const auto d{bignum_mod_multiply(left.z, right.z, prime)};
  const auto e{bignum_mod_subtract(
      bignum_mod_multiply(bignum_mod_add(left.x, left.y, prime),
                          bignum_mod_add(right.x, right.y, prime), prime),
      bignum_mod_add(a, b, prime), prime)};
  const auto f{bignum_mod_subtract(d, c, prime)};
  const auto g{bignum_mod_add(d, c, prime)};
  const auto h{bignum_mod_subtract(
      b, bignum_mod_multiply(parameters.coefficient_a, a, prime), prime)};
  return EdwardsPoint{.x = bignum_mod_multiply(e, f, prime),
                      .y = bignum_mod_multiply(g, h, prime),
                      .z = bignum_mod_multiply(f, g, prime),
                      .t = bignum_mod_multiply(e, h, prime)};
}

inline auto edwards_point_scalar_multiply(const Bignum &scalar,
                                          const EdwardsPoint &point,
                                          const EdwardsParameters &parameters)
    -> EdwardsPoint {
  // The identity element is (0 : 1 : 1 : 0)
  EdwardsPoint result{.x = Bignum{},
                      .y = bignum_from_u64(1),
                      .z = bignum_from_u64(1),
                      .t = Bignum{}};
  const auto bits{bignum_bit_length(scalar)};
  for (std::size_t index = bits; index > 0; --index) {
    result = edwards_point_add(result, result, parameters);
    if (bignum_get_bit(scalar, index - 1)) {
      result = edwards_point_add(result, point, parameters);
    }
  }

  return result;
}

// Whether two points are equal, compared without leaving projective space by
// cross-multiplying through the Z factors
inline auto edwards_point_equal(const EdwardsPoint &left,
                                const EdwardsPoint &right, const Bignum &prime)
    -> bool {
  return bignum_compare(bignum_mod_multiply(left.x, right.z, prime),
                        bignum_mod_multiply(right.x, left.z, prime)) == 0 &&
         bignum_compare(bignum_mod_multiply(left.y, right.z, prime),
                        bignum_mod_multiply(right.y, left.z, prime)) == 0;
}

// Recover an Ed25519 point from its 32-byte encoding (RFC 8032 Section 5.1.3),
// returning no value when the encoding does not name a point on the curve
inline auto edwards25519_decode_point(const std::string_view encoding,
                                      const Bignum &prime,
                                      const Bignum &coefficient_d,
                                      const Bignum &square_root_of_minus_one)
    -> std::optional<EdwardsPoint> {
  if (encoding.size() != 32) {
    return std::nullopt;
  }

  // The final bit holds the sign of x, the remaining bits the little-endian y
  std::string bytes{encoding};
  const auto sign_bit{
      static_cast<unsigned>(static_cast<std::uint8_t>(bytes.back()) >> 7) & 1u};
  bytes.back() =
      static_cast<char>(static_cast<std::uint8_t>(bytes.back()) & 0x7fu);
  const auto y{bignum_from_bytes_little_endian(bytes)};

  // A y coordinate at or beyond the field prime is not a canonical encoding
  if (bignum_compare(y, prime) >= 0) {
    return std::nullopt;
  }

  const auto one{bignum_from_u64(1)};
  const auto y_squared{bignum_mod_multiply(y, y, prime)};

  // Solve x^2 = (y^2 - 1) / (d * y^2 + 1) (mod p)
  const auto numerator{bignum_mod_subtract(y_squared, one, prime)};
  const auto denominator{bignum_mod_add(
      bignum_mod_multiply(coefficient_d, y_squared, prime), one, prime)};

  // The candidate root is x = numerator * denominator^3 *
  // (numerator * denominator^7)^((p - 5) / 8) (mod p), a single powering that
  // folds in the inversion of the denominator
  const auto denominator_squared{
      bignum_mod_multiply(denominator, denominator, prime)};
  const auto denominator_cubed{
      bignum_mod_multiply(denominator_squared, denominator, prime)};
  const auto denominator_seventh{bignum_mod_multiply(
      bignum_mod_multiply(denominator_cubed, denominator_cubed, prime),
      denominator, prime)};
  auto exponent{prime};
  bignum_subtract_in_place(exponent, bignum_from_u64(5));
  exponent = bignum_shift_right(exponent, 3);
  const auto root{
      bignum_mod_exp(bignum_mod_multiply(numerator, denominator_seventh, prime),
                     exponent, prime)};
  auto candidate{bignum_mod_multiply(
      bignum_mod_multiply(numerator, denominator_cubed, prime), root, prime)};

  // The candidate is correct when denominator * x^2 equals the numerator, off
  // by sqrt(-1) when it equals its negation, and otherwise no root exists
  const auto check{bignum_mod_multiply(
      denominator, bignum_mod_multiply(candidate, candidate, prime), prime)};
  if (bignum_compare(check, numerator) != 0) {
    const auto negated_numerator{
        bignum_mod_subtract(Bignum{}, numerator, prime)};
    if (bignum_compare(check, negated_numerator) != 0) {
      return std::nullopt;
    }

    candidate = bignum_mod_multiply(candidate, square_root_of_minus_one, prime);
  }

  // Reject the non-canonical zero root with a set sign bit, then select the
  // root whose low bit matches the encoded sign
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
                      .t = bignum_mod_multiply(candidate, y, prime)};
}

// The Edwards25519 domain parameters (RFC 8032 Section 5.1)
inline auto edwards25519() -> EdwardsParameters {
  EdwardsParameters parameters;
  parameters.prime = bignum_from_hex(
      "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed");
  parameters.order = bignum_from_hex(
      "1000000000000000000000000000000014def9dea2f79cd65812631a5cf5d3ed");

  // The curve coefficient a is -1 (mod p)
  parameters.coefficient_a = parameters.prime;
  bignum_subtract_in_place(parameters.coefficient_a, bignum_from_u64(1));

  // d = -121665 / 121666 (mod p)
  auto negated_numerator{parameters.prime};
  bignum_subtract_in_place(negated_numerator, bignum_from_u64(121665));
  parameters.coefficient_d = bignum_mod_multiply(
      negated_numerator,
      bignum_mod_inverse(bignum_from_u64(121666), parameters.prime),
      parameters.prime);

  // sqrt(-1) = 2^((p - 1) / 4) (mod p), used to recover the second root
  auto root_exponent{parameters.prime};
  bignum_subtract_in_place(root_exponent, bignum_from_u64(1));
  root_exponent = bignum_shift_right(root_exponent, 2);
  const auto square_root_of_minus_one{
      bignum_mod_exp(bignum_from_u64(2), root_exponent, parameters.prime)};

  // The base point is recovered from its canonical encoding, y = 4/5 with a
  // clear sign bit (RFC 8032 Section 5.1)
  std::string base_encoding;
  base_encoding.push_back('\x58');
  base_encoding.append(31, '\x66');
  parameters.base = edwards25519_decode_point(base_encoding, parameters.prime,
                                              parameters.coefficient_d,
                                              square_root_of_minus_one)
                        .value();
  return parameters;
}

// Verify an Ed25519 signature over a message (RFC 8032 Section 5.1.7), given
// the 32-byte public key and the 64-byte signature
inline auto edwards25519_verify(const std::string_view public_key,
                                const std::string_view message,
                                const std::string_view signature) -> bool {
  if (public_key.size() != 32 || signature.size() != 64) {
    return false;
  }

  const auto parameters{edwards25519()};
  auto square_root_exponent{parameters.prime};
  bignum_subtract_in_place(square_root_exponent, bignum_from_u64(1));
  square_root_exponent = bignum_shift_right(square_root_exponent, 2);
  const auto square_root_of_minus_one{bignum_mod_exp(
      bignum_from_u64(2), square_root_exponent, parameters.prime)};

  const auto public_point{edwards25519_decode_point(
      public_key, parameters.prime, parameters.coefficient_d,
      square_root_of_minus_one)};
  if (!public_point.has_value()) {
    return false;
  }

  // The signature is the encoded point R followed by the little-endian scalar
  // S, which must lie below the group order
  const auto encoded_r{signature.substr(0, 32)};
  const auto point_r{edwards25519_decode_point(encoded_r, parameters.prime,
                                               parameters.coefficient_d,
                                               square_root_of_minus_one)};
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

  // The signature holds when [S]B = R + [k]A
  const auto left{
      edwards_point_scalar_multiply(scalar_s, parameters.base, parameters)};
  const auto right{edwards_point_add(
      point_r.value(),
      edwards_point_scalar_multiply(scalar_k, public_point.value(), parameters),
      parameters)};
  return edwards_point_equal(left, right, parameters.prime);
}

// Recover an Ed448 point from its 57-byte encoding (RFC 8032 Section 5.2.3),
// returning no value when the encoding does not name a point on the curve
inline auto edwards448_decode_point(const std::string_view encoding,
                                    const Bignum &prime,
                                    const Bignum &coefficient_d)
    -> std::optional<EdwardsPoint> {
  if (encoding.size() != 57) {
    return std::nullopt;
  }

  // The final bit holds the sign of x, the remaining bits the little-endian y
  std::string bytes{encoding};
  const auto sign_bit{
      static_cast<unsigned>(static_cast<std::uint8_t>(bytes.back()) >> 7) & 1u};
  bytes.back() =
      static_cast<char>(static_cast<std::uint8_t>(bytes.back()) & 0x7fu);
  const auto y{bignum_from_bytes_little_endian(bytes)};

  // A y coordinate at or beyond the field prime is not a canonical encoding
  if (bignum_compare(y, prime) >= 0) {
    return std::nullopt;
  }

  const auto one{bignum_from_u64(1)};
  const auto y_squared{bignum_mod_multiply(y, y, prime)};

  // Solve x^2 = (y^2 - 1) / (d * y^2 - 1) (mod p)
  const auto numerator{bignum_mod_subtract(y_squared, one, prime)};
  const auto denominator{bignum_mod_subtract(
      bignum_mod_multiply(coefficient_d, y_squared, prime), one, prime)};

  // The candidate root is x = numerator^3 * denominator *
  // (numerator^5 * denominator^3)^((p - 3) / 4) (mod p), the field having
  // p congruent to 3 modulo 4
  const auto numerator_squared{
      bignum_mod_multiply(numerator, numerator, prime)};
  const auto numerator_cubed{
      bignum_mod_multiply(numerator_squared, numerator, prime)};
  const auto numerator_fifth{
      bignum_mod_multiply(numerator_squared, numerator_cubed, prime)};
  const auto denominator_squared{
      bignum_mod_multiply(denominator, denominator, prime)};
  const auto denominator_cubed{
      bignum_mod_multiply(denominator_squared, denominator, prime)};
  auto exponent{prime};
  bignum_subtract_in_place(exponent, bignum_from_u64(3));
  exponent = bignum_shift_right(exponent, 2);
  const auto root{bignum_mod_exp(
      bignum_mod_multiply(numerator_fifth, denominator_cubed, prime), exponent,
      prime)};
  auto candidate{bignum_mod_multiply(
      bignum_mod_multiply(numerator_cubed, denominator, prime), root, prime)};

  // The candidate is correct when denominator * x^2 equals the numerator, and
  // otherwise no root exists, as the field admits a single square root
  const auto check{bignum_mod_multiply(
      denominator, bignum_mod_multiply(candidate, candidate, prime), prime)};
  if (bignum_compare(check, numerator) != 0) {
    return std::nullopt;
  }

  // Reject the non-canonical zero root with a set sign bit, then select the
  // root whose low bit matches the encoded sign
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
                      .t = bignum_mod_multiply(candidate, y, prime)};
}

// The Edwards448 domain parameters (RFC 8032 Section 5.2)
inline auto edwards448() -> EdwardsParameters {
  EdwardsParameters parameters;
  // clang-format off
  parameters.prime = bignum_from_hex("fffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  parameters.order = bignum_from_hex("3fffffffffffffffffffffffffffffffffffffffffffffffffffffff7cca23e9c44edb49aed63690216cc2728dc58f552378c292ab5844f3");
  // clang-format on

  // The curve coefficient a is 1, and d is -39081 (mod p)
  parameters.coefficient_a = bignum_from_u64(1);
  parameters.coefficient_d = parameters.prime;
  bignum_subtract_in_place(parameters.coefficient_d, bignum_from_u64(39081));

  // The base point is recovered from its canonical 57-octet encoding (RFC 8032
  // Section 5.2)
  // clang-format off
  const auto base_encoding{bignum_to_bytes(bignum_from_hex("14fa30f25b790898adc8d74e2c13bdfdc4397ce61cffd33ad7c2a0051e9c78874098a36c7373ea4b62c7c9563720768824bcb66e71463f6900"), 57)};
  // clang-format on
  parameters.base = edwards448_decode_point(base_encoding, parameters.prime,
                                            parameters.coefficient_d)
                        .value();
  return parameters;
}

// Verify an Ed448 signature over a message (RFC 8032 Section 5.2.7), given the
// 57-byte public key and the 114-byte signature
inline auto edwards448_verify(const std::string_view public_key,
                              const std::string_view message,
                              const std::string_view signature) -> bool {
  if (public_key.size() != 57 || signature.size() != 114) {
    return false;
  }

  const auto parameters{edwards448()};
  const auto public_point{edwards448_decode_point(public_key, parameters.prime,
                                                  parameters.coefficient_d)};
  if (!public_point.has_value()) {
    return false;
  }

  // The signature is the encoded point R followed by the little-endian scalar
  // S, which must lie below the group order
  const auto encoded_r{signature.substr(0, 57)};
  const auto point_r{edwards448_decode_point(encoded_r, parameters.prime,
                                             parameters.coefficient_d)};
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

  // The signature holds when [S]B = R + [k]A
  const auto left{
      edwards_point_scalar_multiply(scalar_s, parameters.base, parameters)};
  const auto right{edwards_point_add(
      point_r.value(),
      edwards_point_scalar_multiply(scalar_k, public_point.value(), parameters),
      parameters)};
  return edwards_point_equal(left, right, parameters.prime);
}

} // namespace sourcemeta::core

#endif
