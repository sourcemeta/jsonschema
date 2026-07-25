#include <sourcemeta/core/crypto_ecdh.h>

#include "crypto_bignum.h"
#include "crypto_ecc.h"
#include "crypto_other.h"

#include <optional> // std::optional, std::nullopt
#include <string>   // std::string
#include <utility>  // std::unreachable

// A from-scratch ECDH primitive for the reference backend, over the shared
// constant-time elliptic curve arithmetic. The shared secret is the affine x
// coordinate of the peer point multiplied by the own scalar (SEC1, the form
// RFC 7518 Section 4.6 consumes)

namespace sourcemeta::core {

namespace {
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
} // namespace

auto ecdh_derive(const PrivateKey &private_key, const PublicKey &public_key)
    -> std::optional<std::string> {
  const auto *const private_internal{private_key.internal()};
  const auto *const public_internal{public_key.internal()};
  if (private_internal == nullptr || public_internal == nullptr ||
      private_internal->kind != PrivateKey::Type::EllipticCurve ||
      public_internal->kind != PublicKey::Type::EllipticCurve ||
      private_internal->elliptic_curve != public_internal->elliptic_curve) {
    return std::nullopt;
  }

  const auto parameters{to_curve_parameters(private_internal->elliptic_curve)};
  const auto peer_x{bignum_from_bytes(public_internal->coordinate_x)};
  const auto peer_y{bignum_from_bytes(public_internal->coordinate_y)};
  // Invalid-curve defense: the peer coordinates must be below the field prime
  // and satisfy the curve equation
  if (bignum_compare(peer_x, parameters.prime) >= 0 ||
      bignum_compare(peer_y, parameters.prime) >= 0 ||
      !point_on_curve(peer_x, peer_y, parameters)) {
    return std::nullopt;
  }

  const JacobianPoint peer_point{
      .x = peer_x, .y = peer_y, .z = bignum_from_u64(1)};
  auto scalar{bignum_from_bytes(private_internal->scalar)};
  const SecureBignumScope scalar_scope{scalar};
  const auto product{
      point_scalar_multiply_constant_time(scalar, peer_point, parameters)};
  const auto shared_x{point_affine_x_constant_time(product, parameters)};
  return bignum_to_bytes(shared_x, parameters.field_bytes);
}

} // namespace sourcemeta::core
