#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"
#include "crypto_pkcs8.h"

#include <openssl/bn.h>          // BN_bn2binpad, BN_bin2bn
#include <openssl/core_names.h>  // OSSL_PKEY_PARAM_*
#include <openssl/ec.h>          // ECDSA_SIG, d2i_ECDSA_SIG, ECDSA_SIG_get0_*
#include <openssl/evp.h>         // EVP_*
#include <openssl/param_build.h> // OSSL_PARAM_*
#include <openssl/pem.h>         // PEM_read_bio_PrivateKey, BIO_*
#include <openssl/rsa.h> // RSA_PKCS1_PSS_PADDING, RSA_PSS_SALTLEN_DIGEST

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

// The parsed key keeps the native handle alive so that many signatures are
// produced without rebuilding it
struct PrivateKey::Internal {
  PrivateKey::Type kind;
  EVP_PKEY *key;
  // The field width for the elliptic curve raw signature encoding
  std::size_t field_bytes;
  // Set for an id-RSASSA-PSS key, which is refused for PKCS1v15 signing
  bool rsa_pss_restricted{false};
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

// Refuse encrypted documents by offering no password, rather than letting the
// default callback prompt on the terminal
auto no_password(char *, int, int, void *) -> int { return 0; }

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

// The signing functions only cover the three NIST prime curves, so a parsed key
// on any other group is rejected here rather than producing a signature that no
// matching public key can be built for
auto supported_ec_field_bytes(EVP_PKEY *key) -> std::optional<std::size_t> {
  std::array<char, 64> group{};
  std::size_t length{0};
  if (EVP_PKEY_get_utf8_string_param(key, OSSL_PKEY_PARAM_GROUP_NAME,
                                     group.data(), group.size(),
                                     &length) != 1) {
    return std::nullopt;
  }

  const std::string_view name{group.data(), length};
  if (name == "prime256v1" || name == "P-256") {
    return 32;
  }
  if (name == "secp384r1" || name == "P-384") {
    return 48;
  }
  if (name == "secp521r1" || name == "P-521") {
    return 66;
  }

  return std::nullopt;
}

auto native_ec_private_key(const sourcemeta::core::EllipticCurve curve,
                           const std::string_view scalar,
                           const std::string_view coordinate_x,
                           const std::string_view coordinate_y) -> EVP_PKEY * {
  const auto width{sourcemeta::core::curve_field_bytes(curve)};
  const auto stripped_x{sourcemeta::core::strip_left(coordinate_x, '\x00')};
  const auto stripped_y{sourcemeta::core::strip_left(coordinate_y, '\x00')};
  const auto stripped_scalar{sourcemeta::core::strip_left(scalar, '\x00')};
  if (stripped_scalar.empty() || stripped_x.size() > width ||
      stripped_y.size() > width || stripped_scalar.size() > width) {
    return nullptr;
  }

  std::string point;
  point.push_back('\x04');
  point.append(sourcemeta::core::pad_left(stripped_x, width, '\x00'));
  point.append(sourcemeta::core::pad_left(stripped_y, width, '\x00'));
  const auto padded_scalar{
      sourcemeta::core::pad_left(stripped_scalar, width, '\x00')};

  EVP_PKEY *result{nullptr};
  auto *scalar_number{
      BN_bin2bn(reinterpret_cast<const unsigned char *>(padded_scalar.data()),
                static_cast<int>(padded_scalar.size()), nullptr)};
  auto *builder{OSSL_PARAM_BLD_new()};
  if (scalar_number != nullptr && builder != nullptr &&
      OSSL_PARAM_BLD_push_utf8_string(builder, OSSL_PKEY_PARAM_GROUP_NAME,
                                      to_group_name(curve), 0) == 1 &&
      OSSL_PARAM_BLD_push_octet_string(
          builder, OSSL_PKEY_PARAM_PUB_KEY,
          reinterpret_cast<const unsigned char *>(point.data()),
          point.size()) == 1 &&
      OSSL_PARAM_BLD_push_BN(builder, OSSL_PKEY_PARAM_PRIV_KEY,
                             scalar_number) == 1) {
    auto *parameters{OSSL_PARAM_BLD_to_param(builder)};
    if (parameters != nullptr) {
      auto *context{EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr)};
      if (context != nullptr) {
        if (EVP_PKEY_fromdata_init(context) == 1) {
          EVP_PKEY_fromdata(context, &result, EVP_PKEY_KEYPAIR, parameters);
        }

        EVP_PKEY_CTX_free(context);
      }

      OSSL_PARAM_free(parameters);
    }
  }

  OSSL_PARAM_BLD_free(builder);
  BN_free(scalar_number);
  return result;
}

auto sign_digest(EVP_MD_CTX *context, const std::string_view message)
    -> std::optional<std::string> {
  std::size_t length{0};
  if (EVP_DigestSign(context, nullptr, &length,
                     reinterpret_cast<const unsigned char *>(message.data()),
                     message.size()) != 1) {
    return std::nullopt;
  }

  std::string signature(length, '\x00');
  if (EVP_DigestSign(
          context, reinterpret_cast<unsigned char *>(signature.data()), &length,
          reinterpret_cast<const unsigned char *>(message.data()),
          message.size()) != 1) {
    return std::nullopt;
  }

  signature.resize(length);
  return signature;
}

auto sign_rsa(EVP_PKEY *key, const sourcemeta::core::SignatureHashFunction hash,
              const std::string_view message, const bool probabilistic)
    -> std::optional<std::string> {
  std::optional<std::string> result;
  auto *context{EVP_MD_CTX_new()};
  if (context != nullptr) {
    EVP_PKEY_CTX *key_context{nullptr};
    auto ready{EVP_DigestSignInit(context, &key_context,
                                  to_message_digest(hash), nullptr, key) == 1};
    // The padding is set explicitly rather than left to the provider default,
    // so a key restricted to one scheme cleanly rejects the other
    if (ready && probabilistic) {
      ready = EVP_PKEY_CTX_set_rsa_padding(key_context,
                                           RSA_PKCS1_PSS_PADDING) == 1 &&
              EVP_PKEY_CTX_set_rsa_pss_saltlen(key_context,
                                               RSA_PSS_SALTLEN_DIGEST) == 1;
    } else if (ready) {
      ready = EVP_PKEY_CTX_set_rsa_padding(key_context, RSA_PKCS1_PADDING) == 1;
    }

    if (ready) {
      result = sign_digest(context, message);
    }

    EVP_MD_CTX_free(context);
  }

  return result;
}

// Convert the DER ECDSA signature that the platform produces into the raw
// fixed-width R || S concatenation that JWS mandates (RFC 7518 Section 3.4)
auto encode_ecdsa_signature(const std::string_view der,
                            const std::size_t field_bytes)
    -> std::optional<std::string> {
  const auto *pointer{reinterpret_cast<const unsigned char *>(der.data())};
  auto *signature{
      d2i_ECDSA_SIG(nullptr, &pointer, static_cast<long>(der.size()))};
  if (signature == nullptr) {
    return std::nullopt;
  }

  const auto width{static_cast<int>(field_bytes)};
  std::string raw(field_bytes * 2, '\x00');
  auto *raw_data{reinterpret_cast<unsigned char *>(raw.data())};
  const auto ok{
      BN_bn2binpad(ECDSA_SIG_get0_r(signature), raw_data, width) == width &&
      BN_bn2binpad(ECDSA_SIG_get0_s(signature), raw_data + width, width) ==
          width};
  ECDSA_SIG_free(signature);
  if (!ok) {
    return std::nullopt;
  }

  return raw;
}

} // namespace

namespace sourcemeta::core {

PrivateKey::PrivateKey(Internal *internal) noexcept : internal_{internal} {}

PrivateKey::~PrivateKey() {
  if (internal_ != nullptr) {
    EVP_PKEY_free(internal_->key);
    delete internal_;
  }
}

PrivateKey::PrivateKey(PrivateKey &&other) noexcept
    : internal_{other.internal_} {
  other.internal_ = nullptr;
}

auto PrivateKey::operator=(PrivateKey &&other) noexcept -> PrivateKey & {
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

auto PrivateKey::type() const noexcept -> Type {
  // A moved-from key holds no state, so reading its kind is a use-after-move
  assert(internal_ != nullptr);
  return internal_->kind;
}

auto make_private_key(const std::string_view pem) -> std::optional<PrivateKey> {
  // The OpenSSL PEM reader accepts non-canonical PKCS#8 such as trailing bytes
  // past the outer SEQUENCE, so the structure is validated up front to match
  // the other backends (X.690 Section 10.1)
  auto der{pem_to_der(pem)};
  if (!der.has_value()) {
    return std::nullopt;
  }

  // The decoded PKCS#8 holds the whole private key, so it is wiped on return
  const SecureScope der_scope{der.value()};
  if (!parse_pkcs8(der.value()).has_value()) {
    return std::nullopt;
  }

  auto *bio{BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()))};
  if (bio == nullptr) {
    return std::nullopt;
  }

  auto *key{PEM_read_bio_PrivateKey(bio, nullptr, no_password, nullptr)};
  BIO_free(bio);
  if (key == nullptr) {
    return std::nullopt;
  }

  const auto base_id{EVP_PKEY_get_base_id(key)};
  PrivateKey::Type kind{};
  std::size_t field_bytes{0};
  switch (base_id) {
    case EVP_PKEY_RSA:
    case EVP_PKEY_RSA_PSS:
      kind = PrivateKey::Type::RSA;
      break;
    case EVP_PKEY_EC: {
      const auto supported{supported_ec_field_bytes(key)};
      if (!supported.has_value()) {
        EVP_PKEY_free(key);
        return std::nullopt;
      }

      kind = PrivateKey::Type::EllipticCurve;
      field_bytes = supported.value();
      break;
    }
    case EVP_PKEY_ED25519:
    case EVP_PKEY_ED448:
      kind = PrivateKey::Type::Edwards;
      break;
    default:
      EVP_PKEY_free(key);
      return std::nullopt;
  }

  return PrivateKey{new PrivateKey::Internal{.kind = kind,
                                             .key = key,
                                             .field_bytes = field_bytes,
                                             .rsa_pss_restricted =
                                                 base_id == EVP_PKEY_RSA_PSS}};
}

auto make_ec_private_key(const EllipticCurve curve,
                         const std::string_view scalar,
                         const std::string_view coordinate_x,
                         const std::string_view coordinate_y)
    -> std::optional<PrivateKey> {
  if (!ec_private_scalar_in_range(scalar, curve)) {
    return std::nullopt;
  }

  auto *key{native_ec_private_key(curve, scalar, coordinate_x, coordinate_y)};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PrivateKey{
      new PrivateKey::Internal{.kind = PrivateKey::Type::EllipticCurve,
                               .key = key,
                               .field_bytes = curve_field_bytes(curve)}};
}

auto make_edwards_private_key(const EdwardsCurve curve,
                              const std::string_view seed)
    -> std::optional<PrivateKey> {
  if (seed.size() != eddsa_public_key_bytes(curve)) {
    return std::nullopt;
  }

  auto *key{EVP_PKEY_new_raw_private_key(
      to_pkey_id(curve), nullptr,
      reinterpret_cast<const unsigned char *>(seed.data()), seed.size())};
  if (key == nullptr) {
    return std::nullopt;
  }

  return PrivateKey{new PrivateKey::Internal{
      .kind = PrivateKey::Type::Edwards, .key = key, .field_bytes = 0}};
}

auto rsassa_pkcs1_v15_sign(const PrivateKey &key,
                           const SignatureHashFunction hash,
                           const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA) {
    return std::nullopt;
  }

  // An id-RSASSA-PSS key is restricted to PSS and must not sign PKCS1v15
  if (internal->rsa_pss_restricted) {
    return std::nullopt;
  }

  return sign_rsa(internal->key, hash, message, false);
}

auto rsassa_pss_sign(const PrivateKey &key, const SignatureHashFunction hash,
                     const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA) {
    return std::nullopt;
  }

  return sign_rsa(internal->key, hash, message, true);
}

auto ecdsa_sign(const PrivateKey &key, const SignatureHashFunction hash,
                const std::string_view message) -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr ||
      internal->kind != PrivateKey::Type::EllipticCurve) {
    return std::nullopt;
  }

  std::optional<std::string> der;
  auto *context{EVP_MD_CTX_new()};
  if (context != nullptr) {
    if (EVP_DigestSignInit(context, nullptr, to_message_digest(hash), nullptr,
                           internal->key) == 1) {
      der = sign_digest(context, message);
    }

    EVP_MD_CTX_free(context);
  }

  if (!der.has_value()) {
    return std::nullopt;
  }

  return encode_ecdsa_signature(der.value(), internal->field_bytes);
}

auto eddsa_sign(const PrivateKey &key, const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::Edwards) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  auto *context{EVP_MD_CTX_new()};
  if (context != nullptr) {
    // EdDSA is a one-shot signature with a null digest, since the curve fixes
    // the hash function internally
    if (EVP_DigestSignInit(context, nullptr, nullptr, nullptr, internal->key) ==
        1) {
      result = sign_digest(context, message);
    }

    EVP_MD_CTX_free(context);
  }

  return result;
}

} // namespace sourcemeta::core
