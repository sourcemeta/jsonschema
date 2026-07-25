#ifndef SOURCEMETA_CORE_CRYPTO_OTHER_H_
#define SOURCEMETA_CORE_CRYPTO_OTHER_H_

// The reference backend key internals, so the parsed key layout stays
// single-sourced across the reference backends

#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/crypto_verify.h>

#include <string> // std::string

namespace sourcemeta::core {

struct PublicKey::Internal {
  PublicKey::Type kind;
  std::string modulus;
  std::string exponent;
  std::string coordinate_x;
  std::string coordinate_y;
  EllipticCurve elliptic_curve;
  EdwardsCurve edwards_curve;
};

struct PrivateKey::Internal {
  PrivateKey::Type kind;
  std::string modulus;
  std::string public_exponent;
  std::string private_exponent;
  std::string scalar;
  EllipticCurve elliptic_curve;
  std::string edwards_seed;
  EdwardsCurve edwards_curve;
  bool rsa_pss_restricted{false};
  // The public coordinates, kept so the public key can be recovered without
  // recomputing the point from the scalar. A key parsed from a PEM document,
  // which carries only the scalar on this backend, recomputes them at parse
  // time, so they are populated for every elliptic curve private key
  std::string coordinate_x;
  std::string coordinate_y;
};

} // namespace sourcemeta::core

#endif
