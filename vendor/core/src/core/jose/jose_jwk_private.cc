#include <sourcemeta/core/jose_jwk_private.h>

#include <sourcemeta/core/crypto.h>

#include "jose_key.h"

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
using namespace std::string_view_literals;

const auto HASH_KTY{sourcemeta::core::JSON::Object::hash("kty"sv)};
const auto HASH_N{sourcemeta::core::JSON::Object::hash("n"sv)};
const auto HASH_E{sourcemeta::core::JSON::Object::hash("e"sv)};
const auto HASH_D{sourcemeta::core::JSON::Object::hash("d"sv)};
const auto HASH_P{sourcemeta::core::JSON::Object::hash("p"sv)};
const auto HASH_Q{sourcemeta::core::JSON::Object::hash("q"sv)};
const auto HASH_DP{sourcemeta::core::JSON::Object::hash("dp"sv)};
const auto HASH_DQ{sourcemeta::core::JSON::Object::hash("dq"sv)};
const auto HASH_QI{sourcemeta::core::JSON::Object::hash("qi"sv)};
const auto HASH_OTH{sourcemeta::core::JSON::Object::hash("oth"sv)};
const auto HASH_K{sourcemeta::core::JSON::Object::hash("k"sv)};
const auto HASH_CRV{sourcemeta::core::JSON::Object::hash("crv"sv)};
const auto HASH_X{sourcemeta::core::JSON::Object::hash("x"sv)};
const auto HASH_Y{sourcemeta::core::JSON::Object::hash("y"sv)};
const auto HASH_KID{sourcemeta::core::JSON::Object::hash("kid"sv)};
const auto HASH_ALG{sourcemeta::core::JSON::Object::hash("alg"sv)};

// A required string member decoded from base64url, empty when the member is
// missing, of the wrong type, malformed, or decodes to nothing
template <typename Hash>
auto required_base64url(const sourcemeta::core::JSON &value,
                        const std::string_view name, const Hash &hash)
    -> std::optional<std::string> {
  const auto *member{value.try_at(name, hash)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  auto decoded{sourcemeta::core::base64url_decode(member->to_string())};
  if (!decoded.has_value() || decoded.value().empty()) {
    return std::nullopt;
  }

  return decoded;
}

auto to_jwk_kind(const sourcemeta::core::JWKPrivate::Type type) noexcept
    -> sourcemeta::core::JWKKind {
  switch (type) {
    case sourcemeta::core::JWKPrivate::Type::RSA:
      return sourcemeta::core::JWKKind::RSA;
    case sourcemeta::core::JWKPrivate::Type::EllipticCurve:
      return sourcemeta::core::JWKKind::EllipticCurve;
    case sourcemeta::core::JWKPrivate::Type::OctetKeyPair:
      return sourcemeta::core::JWKKind::OctetKeyPair;
    case sourcemeta::core::JWKPrivate::Type::Octet:
      return sourcemeta::core::JWKKind::Octet;
  }

  std::unreachable();
}

} // namespace

namespace sourcemeta::core {

auto JWKPrivate::parse(const JSON &value, JWKPrivate &result) -> bool {
  if (!value.is_object()) {
    return false;
  }

  const auto *key_type{value.try_at("kty", HASH_KTY)};
  if (key_type == nullptr || !key_type->is_string()) {
    return false;
  }

  const auto &key_type_value{key_type->to_string()};
  std::optional<PrivateKey> parsed_key;
  if (key_type_value == "RSA") {
    // Only the two-prime form is supported, so a multi-prime key is rejected
    // (RFC 7518 Section 6.3.2.7)
    if (value.try_at("oth", HASH_OTH) != nullptr) {
      return false;
    }

    // Every private component is required (RFC 7518 Section 6.3.2), since the
    // key is built from its full private structure
    const auto modulus{required_base64url(value, "n", HASH_N)};
    const auto public_exponent{required_base64url(value, "e", HASH_E)};
    const auto private_exponent{required_base64url(value, "d", HASH_D)};
    const auto prime1{required_base64url(value, "p", HASH_P)};
    const auto prime2{required_base64url(value, "q", HASH_Q)};
    const auto exponent1{required_base64url(value, "dp", HASH_DP)};
    const auto exponent2{required_base64url(value, "dq", HASH_DQ)};
    const auto coefficient{required_base64url(value, "qi", HASH_QI)};
    if (!modulus.has_value() || !public_exponent.has_value() ||
        !private_exponent.has_value() || !prime1.has_value() ||
        !prime2.has_value() || !exponent1.has_value() ||
        !exponent2.has_value() || !coefficient.has_value()) {
      return false;
    }

    if (!jwk_rsa_modulus_is_allowed(modulus.value())) {
      return false;
    }

    result.type_ = Type::RSA;
    result.modulus_ = modulus.value();
    result.exponent_ = public_exponent.value();
    parsed_key = make_rsa_private_key(modulus.value(), public_exponent.value(),
                                      private_exponent.value(), prime1.value(),
                                      prime2.value(), exponent1.value(),
                                      exponent2.value(), coefficient.value());
  } else if (key_type_value == "EC") {
    const auto *curve{value.try_at("crv", HASH_CRV)};
    if (curve == nullptr || !curve->is_string()) {
      return false;
    }

    const auto coordinate_bytes{jwk_ec_coordinate_bytes(curve->to_string())};
    if (!coordinate_bytes.has_value()) {
      return false;
    }

    // The public coordinates and the private scalar are all required and share
    // the curve field width (RFC 7518 Sections 6.2.1 and 6.2.2)
    const auto coordinate_x{required_base64url(value, "x", HASH_X)};
    const auto coordinate_y{required_base64url(value, "y", HASH_Y)};
    const auto scalar{required_base64url(value, "d", HASH_D)};
    if (!coordinate_x.has_value() ||
        coordinate_x.value().size() != coordinate_bytes.value() ||
        !coordinate_y.has_value() ||
        coordinate_y.value().size() != coordinate_bytes.value() ||
        !scalar.has_value() ||
        scalar.value().size() != coordinate_bytes.value()) {
      return false;
    }

    result.type_ = Type::EllipticCurve;
    result.curve_ = curve->to_string();
    result.coordinate_x_ = coordinate_x.value();
    result.coordinate_y_ = coordinate_y.value();
    parsed_key = make_ec_private_key(jwk_to_elliptic_curve(result.curve_),
                                     scalar.value(), coordinate_x.value(),
                                     coordinate_y.value());
  } else if (key_type_value == "OKP") {
    const auto *curve{value.try_at("crv", HASH_CRV)};
    if (curve == nullptr || !curve->is_string()) {
      return false;
    }

    const auto key_bytes{jwk_okp_key_bytes(curve->to_string())};
    if (!key_bytes.has_value()) {
      return false;
    }

    // The public key and the private seed are both required and share the same
    // length (RFC 8037 Section 2)
    const auto public_key{required_base64url(value, "x", HASH_X)};
    const auto seed{required_base64url(value, "d", HASH_D)};
    if (!public_key.has_value() ||
        public_key.value().size() != key_bytes.value() || !seed.has_value() ||
        seed.value().size() != key_bytes.value()) {
      return false;
    }

    result.type_ = Type::OctetKeyPair;
    result.curve_ = curve->to_string();
    result.public_point_ = public_key.value();
    parsed_key = make_edwards_private_key(jwk_to_edwards_curve(result.curve_),
                                          seed.value());
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

  // The platform key is built once when the material is decoded, so signing
  // reuses it. A key that cannot be turned into one stays null and simply fails
  // to sign
  result.private_key_ = std::move(parsed_key);

  // A private JWK carries the public key beside the private key (RFC 7518
  // Section 6.2.2, RFC 8037 Section 2). For the curve types, reject a document
  // whose stored public part does not correspond to its private part, verified
  // through a signature the private key produces and the public key checks. For
  // elliptic curve keys the platform key construction additionally binds the
  // coordinates to the scalar, closing the gap where a crafted point could
  // satisfy one precomputed signature on a deterministic backend. RSA
  // correspondence is not checked here, so an inconsistent RSA key instead
  // stays null and fails to sign
  if (result.type_ == Type::EllipticCurve ||
      result.type_ == Type::OctetKeyPair) {
    if (!result.private_key_.has_value()) {
      return false;
    }

    using namespace std::string_view_literals;
    const auto probe{"sourcemeta core JWK keypair consistency probe"sv};
    bool consistent{false};
    if (result.type_ == Type::EllipticCurve) {
      const auto public_key{
          make_ec_public_key(jwk_to_elliptic_curve(result.curve_),
                             result.coordinate_x_, result.coordinate_y_)};
      const auto signature{ecdsa_sign(result.private_key_.value(),
                                      SignatureHashFunction::SHA256, probe)};
      consistent =
          public_key.has_value() && signature.has_value() &&
          ecdsa_verify(public_key.value(), SignatureHashFunction::SHA256, probe,
                       signature.value());
    } else {
      const auto public_key{make_eddsa_public_key(
          jwk_to_edwards_curve(result.curve_), result.public_point_)};
      const auto signature{eddsa_sign(result.private_key_.value(), probe)};
      consistent = public_key.has_value() && signature.has_value() &&
                   eddsa_verify(public_key.value(), probe, signature.value());
    }

    if (!consistent) {
      return false;
    }
  }

  return true;
}

auto JWKPrivate::from(const JSON &value) -> std::optional<JWKPrivate> {
  JWKPrivate result;
  if (parse(value, result)) {
    return result;
  }

  return std::nullopt;
}

auto JWKPrivate::from(JSON &&value) -> std::optional<JWKPrivate> {
  return from(value);
}

auto JWKPrivate::from_pem(const std::string_view pem)
    -> std::optional<JWKPrivate> {
  auto parsed_key{make_private_key(pem)};
  if (!parsed_key.has_value()) {
    return std::nullopt;
  }

  JWKPrivate result;
  switch (parsed_key.value().type()) {
    case PrivateKey::Type::RSA:
      result.type_ = Type::RSA;
      break;
    case PrivateKey::Type::EllipticCurve:
      result.type_ = Type::EllipticCurve;
      break;
    case PrivateKey::Type::Edwards:
      result.type_ = Type::OctetKeyPair;
      break;
  }

  result.private_key_ = std::move(parsed_key);

  // A PEM document carries no encoded public members, so recover them from the
  // parsed key to support public serialization and thumbprints
  const auto public_key{derive_public_key(result.private_key_.value())};
  if (public_key.has_value()) {
    switch (public_key.value().type()) {
      case PublicKey::Type::RSA: {
        const auto components{rsa_public_components(public_key.value())};
        if (components.has_value()) {
          result.modulus_ = components.value().modulus;
          result.exponent_ = components.value().exponent;
        }

        break;
      }
      case PublicKey::Type::EllipticCurve: {
        const auto components{ec_public_components(public_key.value())};
        if (components.has_value()) {
          result.curve_ = elliptic_curve_to_jwk(components.value().curve);
          result.coordinate_x_ = components.value().x;
          result.coordinate_y_ = components.value().y;
        }

        break;
      }
      case PublicKey::Type::Edwards: {
        const auto components{edwards_public_components(public_key.value())};
        if (components.has_value()) {
          result.curve_ = edwards_curve_to_jwk(components.value().curve);
          result.public_point_ = components.value().point;
        }

        break;
      }
    }
  }

  return result;
}

} // namespace sourcemeta::core
