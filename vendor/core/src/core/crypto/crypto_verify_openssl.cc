#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"

#include <openssl/bn.h>          // BN_*
#include <openssl/core_names.h>  // OSSL_PKEY_PARAM_*
#include <openssl/ec.h>          // ECDSA_SIG_*, i2d_ECDSA_SIG
#include <openssl/evp.h>         // EVP_*
#include <openssl/param_build.h> // OSSL_PARAM_*
#include <openssl/rsa.h> // RSA_PKCS1_PSS_PADDING, RSA_PSS_SALTLEN_DIGEST

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

// The parsed key keeps the native handle alive so that many signatures verify
// without rebuilding it
struct PublicKey::Internal {
  PublicKey::Type kind;
  EVP_PKEY *key;
  // The stripped modulus, kept for the RSA signature range check
  std::string modulus;
  // The field width for the elliptic curve signature size check
  std::size_t field_bytes;
  // The expected signature length for the Edwards curve
  std::size_t signature_bytes;
};

} // namespace sourcemeta::core

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

auto to_pkey_id(const sourcemeta::core::EdwardsCurve curve) noexcept -> int {
  switch (curve) {
    case sourcemeta::core::EdwardsCurve::Ed25519:
      return EVP_PKEY_ED25519;
    case sourcemeta::core::EdwardsCurve::Ed448:
      return EVP_PKEY_ED448;
  }

  std::unreachable();
}

auto native_rsa_key(const std::string_view modulus,
                    const std::string_view exponent) -> EVP_PKEY * {
  EVP_PKEY *result{nullptr};
  auto *modulus_number{
      BN_bin2bn(reinterpret_cast<const unsigned char *>(modulus.data()),
                static_cast<int>(modulus.size()), nullptr)};
  auto *exponent_number{
      BN_bin2bn(reinterpret_cast<const unsigned char *>(exponent.data()),
                static_cast<int>(exponent.size()), nullptr)};
  auto *builder{OSSL_PARAM_BLD_new()};

  if (modulus_number != nullptr && exponent_number != nullptr &&
      builder != nullptr &&
      OSSL_PARAM_BLD_push_BN(builder, OSSL_PKEY_PARAM_RSA_N, modulus_number) ==
          1 &&
      OSSL_PARAM_BLD_push_BN(builder, OSSL_PKEY_PARAM_RSA_E, exponent_number) ==
          1) {
    auto *parameters{OSSL_PARAM_BLD_to_param(builder)};
    if (parameters != nullptr) {
      auto *context{EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr)};
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
  BN_free(exponent_number);
  BN_free(modulus_number);
  return result;
}

auto native_ec_key(const sourcemeta::core::EllipticCurve curve,
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

// Convert the raw fixed-width R || S concatenation into the DER signature that
// the verification interface expects
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

auto verify_rsa(EVP_PKEY *key,
                const sourcemeta::core::SignatureHashFunction hash,
                const std::string_view message,
                const std::string_view signature, const bool probabilistic)
    -> bool {
  auto result{false};
  auto *context{EVP_MD_CTX_new()};
  if (context != nullptr) {
    EVP_PKEY_CTX *key_context{nullptr};
    auto ready{EVP_DigestVerifyInit(context, &key_context,
                                    to_message_digest(hash), nullptr,
                                    key) == 1};
    if (ready && probabilistic) {
      ready = EVP_PKEY_CTX_set_rsa_padding(key_context,
                                           RSA_PKCS1_PSS_PADDING) == 1 &&
              EVP_PKEY_CTX_set_rsa_pss_saltlen(key_context,
                                               RSA_PSS_SALTLEN_DIGEST) == 1;
    }

    if (ready) {
      result = EVP_DigestVerify(
                   context,
                   reinterpret_cast<const unsigned char *>(signature.data()),
                   signature.size(),
                   reinterpret_cast<const unsigned char *>(message.data()),
                   message.size()) == 1;
    }

    EVP_MD_CTX_free(context);
  }

  return result;
}

} // namespace

namespace sourcemeta::core {

PublicKey::PublicKey(Internal *internal) noexcept : internal_{internal} {}

PublicKey::~PublicKey() {
  if (internal_ != nullptr) {
    EVP_PKEY_free(internal_->key);
    delete internal_;
  }
}

PublicKey::PublicKey(PublicKey &&other) noexcept : internal_{other.internal_} {
  other.internal_ = nullptr;
}

auto PublicKey::operator=(PublicKey &&other) noexcept -> PublicKey & {
  if (this != &other) {
    if (internal_ != nullptr) {
      EVP_PKEY_free(internal_->key);
      delete internal_;
    }

    internal_ = other.internal_;
    other.internal_ = nullptr;
  }

  return *this;
}

auto PublicKey::type() const noexcept -> Type { return internal_->kind; }

auto make_rsa_public_key(const std::string_view modulus,
                         const std::string_view exponent)
    -> std::optional<PublicKey> {
  auto stripped_modulus{std::string{strip_left(modulus, '\x00')}};
  const auto stripped_exponent{strip_left(exponent, '\x00')};
  if (stripped_modulus.empty() || stripped_exponent.empty() ||
      stripped_modulus.size() > MAXIMUM_KEY_BYTES ||
      stripped_exponent.size() > MAXIMUM_KEY_BYTES) {
    return std::nullopt;
  }

  auto *key{native_rsa_key(stripped_modulus, stripped_exponent)};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::RSA,
                              .key = key,
                              .modulus = std::move(stripped_modulus),
                              .field_bytes = 0,
                              .signature_bytes = 0}};
}

auto make_ec_public_key(const EllipticCurve curve,
                        const std::string_view coordinate_x,
                        const std::string_view coordinate_y)
    -> std::optional<PublicKey> {
  auto *key{native_ec_key(curve, coordinate_x, coordinate_y)};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::EllipticCurve,
                              .key = key,
                              .modulus = {},
                              .field_bytes = curve_field_bytes(curve),
                              .signature_bytes = 0}};
}

auto make_eddsa_public_key(const EdwardsCurve curve,
                           const std::string_view public_key)
    -> std::optional<PublicKey> {
  if (public_key.size() != eddsa_public_key_bytes(curve)) {
    return std::nullopt;
  }

  auto *key{EVP_PKEY_new_raw_public_key(
      to_pkey_id(curve), nullptr,
      reinterpret_cast<const unsigned char *>(public_key.data()),
      public_key.size())};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PublicKey{
      new PublicKey::Internal{.kind = PublicKey::Type::Edwards,
                              .key = key,
                              .modulus = {},
                              .field_bytes = 0,
                              .signature_bytes = eddsa_signature_bytes(curve)}};
}

auto rsassa_pkcs1_v15_verify(const PublicKey &key,
                             const SignatureHashFunction hash,
                             const std::string_view message,
                             const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA ||
      !rsa_signature_in_range(signature, internal->modulus)) {
    return false;
  }

  return verify_rsa(internal->key, hash, message, signature, false);
}

auto rsassa_pss_verify(const PublicKey &key, const SignatureHashFunction hash,
                       const std::string_view message,
                       const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA ||
      !rsa_signature_in_range(signature, internal->modulus)) {
    return false;
  }

  return verify_rsa(internal->key, hash, message, signature, true);
}

auto ecdsa_verify(const PublicKey &key, const SignatureHashFunction hash,
                  const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::EllipticCurve ||
      signature.size() != internal->field_bytes * 2) {
    return false;
  }

  unsigned char *der_signature{nullptr};
  const auto der_length{encode_ecdsa_signature(signature, &der_signature)};
  auto result{false};
  if (der_length > 0) {
    auto *context{EVP_MD_CTX_new()};
    if (context != nullptr) {
      if (EVP_DigestVerifyInit(context, nullptr, to_message_digest(hash),
                               nullptr, internal->key) == 1) {
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
  return result;
}

auto eddsa_verify(const PublicKey &key, const std::string_view message,
                  const std::string_view signature) -> bool {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::Edwards ||
      signature.size() != internal->signature_bytes) {
    return false;
  }

  auto result{false};
  auto *context{EVP_MD_CTX_new()};
  if (context != nullptr) {
    // EdDSA is a one-shot verification with a null digest, since the curve
    // fixes the hash function internally
    if (EVP_DigestVerifyInit(context, nullptr, nullptr, nullptr,
                             internal->key) == 1) {
      result = EVP_DigestVerify(
                   context,
                   reinterpret_cast<const unsigned char *>(signature.data()),
                   signature.size(),
                   reinterpret_cast<const unsigned char *>(message.data()),
                   message.size()) == 1;
    }

    EVP_MD_CTX_free(context);
  }

  return result;
}

} // namespace sourcemeta::core
