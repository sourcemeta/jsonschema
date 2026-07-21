#include <sourcemeta/core/jose_jwk.h>

#include <sourcemeta/core/crypto.h>

#include "jose_key.h"

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace {
using namespace std::string_view_literals;

const auto HASH_KTY{sourcemeta::core::JSON::Object::hash("kty"sv)};
const auto HASH_N{sourcemeta::core::JSON::Object::hash("n"sv)};
const auto HASH_E{sourcemeta::core::JSON::Object::hash("e"sv)};
const auto HASH_CRV{sourcemeta::core::JSON::Object::hash("crv"sv)};
const auto HASH_X{sourcemeta::core::JSON::Object::hash("x"sv)};
const auto HASH_Y{sourcemeta::core::JSON::Object::hash("y"sv)};
const auto HASH_KID{sourcemeta::core::JSON::Object::hash("kid"sv)};
const auto HASH_ALG{sourcemeta::core::JSON::Object::hash("alg"sv)};
const auto HASH_D{sourcemeta::core::JSON::Object::hash("d"sv)};
const auto HASH_P{sourcemeta::core::JSON::Object::hash("p"sv)};
const auto HASH_Q{sourcemeta::core::JSON::Object::hash("q"sv)};
const auto HASH_DP{sourcemeta::core::JSON::Object::hash("dp"sv)};
const auto HASH_DQ{sourcemeta::core::JSON::Object::hash("dq"sv)};
const auto HASH_QI{sourcemeta::core::JSON::Object::hash("qi"sv)};
const auto HASH_OTH{sourcemeta::core::JSON::Object::hash("oth"sv)};
const auto HASH_K{sourcemeta::core::JSON::Object::hash("k"sv)};

auto to_jwk_kind(const sourcemeta::core::JWK::Type type) noexcept
    -> sourcemeta::core::JWKKind {
  switch (type) {
    case sourcemeta::core::JWK::Type::RSA:
      return sourcemeta::core::JWKKind::RSA;
    case sourcemeta::core::JWK::Type::EllipticCurve:
      return sourcemeta::core::JWKKind::EllipticCurve;
    case sourcemeta::core::JWK::Type::OctetKeyPair:
      return sourcemeta::core::JWKKind::OctetKeyPair;
    case sourcemeta::core::JWK::Type::Octet:
      return sourcemeta::core::JWKKind::Octet;
  }

  std::unreachable();
}

} // namespace

namespace sourcemeta::core {

auto JWK::parse(const JSON &value, JWK &result) -> bool {
  if (!value.is_object()) {
    return false;
  }

  const auto *key_type{value.try_at("kty", HASH_KTY)};
  if (key_type == nullptr || !key_type->is_string()) {
    return false;
  }

  const auto &key_type_value{key_type->to_string()};
  std::optional<PublicKey> parsed_key;
  if (key_type_value == "RSA") {
    // A public key must not carry the private parameters (RFC 7518 Section
    // 6.3.2), and rejecting them early surfaces dangerous misconfigurations
    if (value.try_at("d", HASH_D) != nullptr ||
        value.try_at("p", HASH_P) != nullptr ||
        value.try_at("q", HASH_Q) != nullptr ||
        value.try_at("dp", HASH_DP) != nullptr ||
        value.try_at("dq", HASH_DQ) != nullptr ||
        value.try_at("qi", HASH_QI) != nullptr ||
        value.try_at("oth", HASH_OTH) != nullptr) {
      return false;
    }

    const auto *modulus{value.try_at("n", HASH_N)};
    const auto *exponent{value.try_at("e", HASH_E)};
    if (modulus == nullptr || !modulus->is_string() || exponent == nullptr ||
        !exponent->is_string()) {
      return false;
    }

    auto decoded_modulus{base64url_decode(modulus->to_string())};
    auto decoded_exponent{base64url_decode(exponent->to_string())};
    if (!decoded_modulus.has_value() || decoded_modulus.value().empty() ||
        !decoded_exponent.has_value() || decoded_exponent.value().empty()) {
      return false;
    }

    if (!jwk_rsa_modulus_is_allowed(decoded_modulus.value())) {
      return false;
    }

    result.type_ = Type::RSA;
    result.modulus_ = decoded_modulus.value();
    result.exponent_ = decoded_exponent.value();
    parsed_key = make_rsa_public_key(result.modulus_, result.exponent_);
  } else if (key_type_value == "EC") {
    // A public key must not carry the private parameter (RFC 7518 Section
    // 6.2.2)
    if (value.try_at("d", HASH_D) != nullptr) {
      return false;
    }

    const auto *curve{value.try_at("crv", HASH_CRV)};
    const auto *coordinate_x{value.try_at("x", HASH_X)};
    const auto *coordinate_y{value.try_at("y", HASH_Y)};
    if (curve == nullptr || !curve->is_string() || coordinate_x == nullptr ||
        !coordinate_x->is_string() || coordinate_y == nullptr ||
        !coordinate_y->is_string()) {
      return false;
    }

    const auto coordinate_bytes{jwk_ec_coordinate_bytes(curve->to_string())};
    if (!coordinate_bytes.has_value()) {
      return false;
    }

    auto decoded_x{base64url_decode(coordinate_x->to_string())};
    auto decoded_y{base64url_decode(coordinate_y->to_string())};
    if (!decoded_x.has_value() ||
        decoded_x.value().size() != coordinate_bytes.value() ||
        !decoded_y.has_value() ||
        decoded_y.value().size() != coordinate_bytes.value()) {
      return false;
    }

    result.type_ = Type::EllipticCurve;
    result.curve_ = curve->to_string();
    result.coordinate_x_ = decoded_x.value();
    result.coordinate_y_ = decoded_y.value();
    parsed_key = make_ec_public_key(jwk_to_elliptic_curve(result.curve_),
                                    result.coordinate_x_, result.coordinate_y_);
  } else if (key_type_value == "OKP") {
    // A public key must not carry the private parameter (RFC 8037 Section 2)
    if (value.try_at("d", HASH_D) != nullptr) {
      return false;
    }

    const auto *curve{value.try_at("crv", HASH_CRV)};
    const auto *public_key{value.try_at("x", HASH_X)};
    if (curve == nullptr || !curve->is_string() || public_key == nullptr ||
        !public_key->is_string()) {
      return false;
    }

    const auto key_bytes{jwk_okp_key_bytes(curve->to_string())};
    if (!key_bytes.has_value()) {
      return false;
    }

    auto decoded_public_key{base64url_decode(public_key->to_string())};
    if (!decoded_public_key.has_value() ||
        decoded_public_key.value().size() != key_bytes.value()) {
      return false;
    }

    result.type_ = Type::OctetKeyPair;
    result.curve_ = curve->to_string();
    result.public_point_ = decoded_public_key.value();
    parsed_key = make_eddsa_public_key(jwk_to_edwards_curve(result.curve_),
                                       result.public_point_);
  } else if (key_type_value == "oct") {
    const auto *key_value{value.try_at("k", HASH_K)};
    if (key_value == nullptr || !key_value->is_string()) {
      return false;
    }

    auto decoded_key{base64url_decode(key_value->to_string())};
    if (!decoded_key.has_value() || decoded_key.value().empty()) {
      return false;
    }

    result.type_ = Type::Octet;
    result.secret_ = std::move(decoded_key).value();
  } else {
    return false;
  }

  const auto *key_id{value.try_at("kid", HASH_KID)};
  if (key_id != nullptr) {
    if (!key_id->is_string()) {
      return false;
    }

    result.key_id_ = key_id->to_string();
  }

  const auto *algorithm{value.try_at("alg", HASH_ALG)};
  if (algorithm != nullptr) {
    if (!algorithm->is_string()) {
      return false;
    }

    // The algorithm is an advisory hint (RFC 7517 Section 4.4), so honor it
    // only when it names a supported algorithm consistent with the key type,
    // and otherwise leave it unset rather than rejecting an otherwise valid key
    const auto parsed{to_jws_algorithm(algorithm->to_string())};
    if (parsed.has_value() &&
        jwk_algorithm_matches_key(parsed.value(), to_jwk_kind(result.type_),
                                  result.curve_)) {
      result.algorithm_ = parsed;
    }
  }

  // The platform key is built once when the material is decoded, so
  // verification reuses it. A key that cannot be turned into one stays null and
  // simply fails to verify
  result.public_key_ = std::move(parsed_key);
  return true;
}

JWK::JWK(const JSON &value) {
  if (!parse(value, *this)) {
    throw JWKParseError{};
  }
}

// The key material is base64url-decoded into fresh storage, so there is nothing
// to move out of the source value. The rvalue overloads exist for call-site
// symmetry and delegate to the lvalue path
JWK::JWK(JSON &&value) : JWK{value} {}

auto JWK::from(const JSON &value) -> std::optional<JWK> {
  JWK result;
  if (parse(value, result)) {
    return result;
  }

  return std::nullopt;
}

auto JWK::from(JSON &&value) -> std::optional<JWK> { return from(value); }

} // namespace sourcemeta::core
