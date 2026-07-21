#ifndef SOURCEMETA_CORE_CRYPTO_OPENSSL_H_
#define SOURCEMETA_CORE_CRYPTO_OPENSSL_H_

// OpenSSL key export helpers shared by the signing and verification backends,
// so the provider parameter handling and the curve mapping stay single-sourced

#include <sourcemeta/core/crypto_verify.h>

#include <openssl/bn.h>         // BIGNUM, BN_*
#include <openssl/core_names.h> // OSSL_PKEY_PARAM_*
#include <openssl/evp.h>        // EVP_PKEY, EVP_PKEY_get_*

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// The platform reports the curve under its canonical name rather than the JWK
// alias the key was built with, so both are accepted
inline auto ec_curve_from_group_name(const std::string_view name) noexcept
    -> std::optional<EllipticCurve> {
  if (name == "P-256" || name == "prime256v1") {
    return EllipticCurve::P256;
  } else if (name == "P-384" || name == "secp384r1") {
    return EllipticCurve::P384;
  } else if (name == "P-521" || name == "secp521r1") {
    return EllipticCurve::P521;
  } else {
    return std::nullopt;
  }
}

// A big-endian minimal-length byte string of a bignum, as the JWK members and
// the signature range check expect
inline auto bignum_to_bytes(const BIGNUM *number) -> std::string {
  std::string result(static_cast<std::size_t>(BN_num_bytes(number)), '\x00');
  BN_bn2bin(number, reinterpret_cast<unsigned char *>(result.data()));
  return result;
}

// Read the fixed-width uncompressed public point of an elliptic curve or
// Edwards curve key from the native handle
inline auto read_public_point(EVP_PKEY *key) -> std::optional<std::string> {
  std::size_t length{0};
  if (EVP_PKEY_get_octet_string_param(key, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0,
                                      &length) != 1) {
    return std::nullopt;
  }

  std::string point(length, '\x00');
  if (EVP_PKEY_get_octet_string_param(
          key, OSSL_PKEY_PARAM_PUB_KEY,
          reinterpret_cast<unsigned char *>(point.data()), point.size(),
          &length) != 1) {
    return std::nullopt;
  }

  // The second call reports how many octets it actually wrote, which can be
  // fewer than the buffer, so the string is trimmed to avoid trailing zeros
  point.resize(length);
  return point;
}

inline auto read_ec_curve(EVP_PKEY *key) -> std::optional<EllipticCurve> {
  std::array<char, 64> group{};
  std::size_t length{0};
  if (EVP_PKEY_get_utf8_string_param(key, OSSL_PKEY_PARAM_GROUP_NAME,
                                     group.data(), group.size(),
                                     &length) != 1) {
    return std::nullopt;
  }

  return ec_curve_from_group_name(std::string_view{group.data(), length});
}

} // namespace sourcemeta::core

#endif
