#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"

#include <openssl/bn.h>          // BN_*
#include <openssl/core_names.h>  // OSSL_PKEY_PARAM_*
#include <openssl/ec.h>          // ECDSA_SIG_*, i2d_ECDSA_SIG
#include <openssl/evp.h>         // EVP_*
#include <openssl/param_build.h> // OSSL_PARAM_*

#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto to_message_digest(
    const sourcemeta::core::SignatureHashFunction hash) noexcept
    -> const EVP_MD * {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return EVP_sha256();
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return EVP_sha384();
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return EVP_sha512();
  }

  std::unreachable();
}

auto to_group_name(const sourcemeta::core::EllipticCurve curve) noexcept
    -> const char * {
  switch (curve) {
    case sourcemeta::core::EllipticCurve::P256:
      return "P-256";
    case sourcemeta::core::EllipticCurve::P384:
      return "P-384";
    case sourcemeta::core::EllipticCurve::P521:
      return "P-521";
  }

  std::unreachable();
}

auto make_ec_public_key(const sourcemeta::core::EllipticCurve curve,
                        const std::string_view coordinate_x,
                        const std::string_view coordinate_y) -> EVP_PKEY * {
  const auto width{sourcemeta::core::curve_field_bytes(curve)};
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  if (stripped_x.size() > width || stripped_y.size() > width) {
    return nullptr;
  }

  std::string point;
  point.push_back('\x04');
  point.append(sourcemeta::core::pad_left(stripped_x, width, '\x00'));
  point.append(sourcemeta::core::pad_left(stripped_y, width, '\x00'));

  EVP_PKEY *result{nullptr};
  auto *builder{OSSL_PARAM_BLD_new()};
  if (builder != nullptr &&
      OSSL_PARAM_BLD_push_utf8_string(builder, OSSL_PKEY_PARAM_GROUP_NAME,
                                      to_group_name(curve), 0) == 1 &&
      OSSL_PARAM_BLD_push_octet_string(
          builder, OSSL_PKEY_PARAM_PUB_KEY,
          reinterpret_cast<const unsigned char *>(point.data()),
          point.size()) == 1) {
    auto *parameters{OSSL_PARAM_BLD_to_param(builder)};
    if (parameters != nullptr) {
      auto *context{EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr)};
      if (context != nullptr) {
        if (EVP_PKEY_fromdata_init(context) == 1) {
          EVP_PKEY_fromdata(context, &result, EVP_PKEY_PUBLIC_KEY, parameters);
        }

        EVP_PKEY_CTX_free(context);
      }

      OSSL_PARAM_free(parameters);
    }
  }

  OSSL_PARAM_BLD_free(builder);
  return result;
}

// Convert the raw fixed-width R || S concatenation into the DER signature
// that the verification interface expects
auto encode_ecdsa_signature(const std::string_view raw_signature,
                            unsigned char **output) -> int {
  const auto half{raw_signature.size() / 2};
  auto *signature{ECDSA_SIG_new()};
  if (signature == nullptr) {
    return -1;
  }

  auto *r{
      BN_bin2bn(reinterpret_cast<const unsigned char *>(raw_signature.data()),
                static_cast<int>(half), nullptr)};
  auto *s{BN_bin2bn(
      reinterpret_cast<const unsigned char *>(raw_signature.data() + half),
      static_cast<int>(half), nullptr)};
  if (r == nullptr || s == nullptr || ECDSA_SIG_set0(signature, r, s) != 1) {
    BN_free(r);
    BN_free(s);
    ECDSA_SIG_free(signature);
    return -1;
  }

  const auto length{i2d_ECDSA_SIG(signature, output)};
  ECDSA_SIG_free(signature);
  return length;
}

} // namespace

namespace sourcemeta::core {

auto ecdsa_verify(const EllipticCurve curve, const SignatureHashFunction hash,
                  const std::string_view coordinate_x,
                  const std::string_view coordinate_y,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  if (signature.size() != sourcemeta::core::curve_field_bytes(curve) * 2) {
    return false;
  }

  auto *key{make_ec_public_key(curve, coordinate_x, coordinate_y)};
  if (key == nullptr) {
    return false;
  }

  unsigned char *der_signature{nullptr};
  const auto der_length{encode_ecdsa_signature(signature, &der_signature)};
  auto result{false};
  if (der_length > 0) {
    auto *context{EVP_MD_CTX_new()};
    if (context != nullptr) {
      if (EVP_DigestVerifyInit(context, nullptr, to_message_digest(hash),
                               nullptr, key) == 1) {
        result =
            EVP_DigestVerify(
                context, der_signature, static_cast<std::size_t>(der_length),
                reinterpret_cast<const unsigned char *>(message.data()),
                message.size()) == 1;
      }

      EVP_MD_CTX_free(context);
    }
  }

  OPENSSL_free(der_signature);
  EVP_PKEY_free(key);
  return result;
}

} // namespace sourcemeta::core
