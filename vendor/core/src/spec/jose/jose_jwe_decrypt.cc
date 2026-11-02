#include <sourcemeta/core/jose_decrypt.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose_jwk.h>
#include <sourcemeta/core/json.h>

#include "jose_jwe_internal.h"

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
using namespace std::string_view_literals;

constexpr auto HASH_EPK{sourcemeta::core::JSON::Object::hash("epk"sv)};
constexpr auto HASH_APU{sourcemeta::core::JSON::Object::hash("apu"sv)};
constexpr auto HASH_APV{sourcemeta::core::JSON::Object::hash("apv"sv)};

// Recover the content encryption key from an ECDH-ES agreement (RFC 7518
// Section 4.6). Returns no value when the ephemeral key, the agreement, or the
// key derivation cannot be completed
auto ecdh_content_key(const sourcemeta::core::JWE &jwe,
                      const sourcemeta::core::JWKPrivate &key,
                      const sourcemeta::core::JWEAlgorithm algorithm,
                      const sourcemeta::core::JWEEncryption encryption,
                      const std::size_t content_key_bytes)
    -> std::optional<std::string> {
  // The ephemeral public key travels in the protected header (RFC 7518 Section
  // 4.6.1), parsed and validated on the curve through the JWK machinery
  const auto *ephemeral{jwe.header().try_at("epk", HASH_EPK)};
  if (ephemeral == nullptr || !ephemeral->is_object()) {
    return std::nullopt;
  }

  const auto ephemeral_key{sourcemeta::core::JWK::from(*ephemeral)};
  if (!ephemeral_key.has_value() ||
      ephemeral_key.value().type() !=
          sourcemeta::core::JWK::Type::EllipticCurve ||
      ephemeral_key.value().public_key() == nullptr) {
    return std::nullopt;
  }

  // The agreement pairs the recipient private key with the ephemeral public
  // key; a curve mismatch is rejected here
  const auto shared{sourcemeta::core::ecdh_derive(
      *key.private_key(), *ephemeral_key.value().public_key())};
  if (!shared.has_value()) {
    return std::nullopt;
  }

  std::string party_u;
  std::string party_v;
  if (!sourcemeta::core::jwe_read_party_info(jwe.header(), "apu", HASH_APU,
                                             party_u) ||
      !sourcemeta::core::jwe_read_party_info(jwe.header(), "apv", HASH_APV,
                                             party_v)) {
    return std::nullopt;
  }

  if (algorithm == sourcemeta::core::JWEAlgorithm::ECDH_ES) {
    // Direct Key Agreement carries no wrapped key, so the Encrypted Key must be
    // empty (RFC 7516 Section 5.2 step 10)
    if (!jwe.encrypted_key().empty()) {
      return std::nullopt;
    }

    // The derived key is the CEK, over the "enc" value (RFC 7518 Section 4.6.2)
    return sourcemeta::core::kdf_concat(
        shared.value(), sourcemeta::core::jwe_encryption_name(encryption),
        party_u, party_v, content_key_bytes);
  }

  // Key Agreement with Key Wrapping: the derived key unwraps the CEK, over the
  // "alg" value (RFC 7518 Section 4.6.2)
  const auto key_wrap_bytes{
      sourcemeta::core::jwe_key_wrap_bytes(algorithm).value()};
  const auto key_encryption_key{sourcemeta::core::kdf_concat(
      shared.value(), sourcemeta::core::jwe_algorithm_name(algorithm), party_u,
      party_v, key_wrap_bytes)};
  if (!key_encryption_key.has_value()) {
    return std::nullopt;
  }

  return sourcemeta::core::aes_key_unwrap(key_encryption_key.value(),
                                          jwe.encrypted_key());
}

} // namespace

namespace sourcemeta::core {

auto jwe_decrypt(const JWE &jwe, const JWKPrivate &key)
    -> std::optional<std::string> {
  const auto algorithm{jwe.algorithm()};
  const auto encryption{jwe.encryption()};
  if (!algorithm.has_value() || !encryption.has_value()) {
    return std::nullopt;
  }

  const auto content_key_bytes{jwe_encryption_key_bytes(encryption.value())};

  // Recover the content encryption key. The key-type binding is checked first,
  // since it depends on the key rather than the ciphertext and blocks algorithm
  // confusion (RFC 7516 Section 11.4)
  std::optional<std::string> content_key;
  switch (algorithm.value()) {
    case JWEAlgorithm::RSA_OAEP:
    case JWEAlgorithm::RSA_OAEP_256:
      if (key.type() != JWKPrivate::Type::RSA || key.private_key() == nullptr) {
        return std::nullopt;
      }

      content_key = rsa_oaep_decrypt(*key.private_key(),
                                     jwe_rsa_oaep_hash(algorithm.value()),
                                     jwe.encrypted_key());
      break;
    case JWEAlgorithm::A128KW:
    case JWEAlgorithm::A192KW:
    case JWEAlgorithm::A256KW: {
      const auto key_wrap_bytes{jwe_key_wrap_bytes(algorithm.value()).value()};
      if (key.type() != JWKPrivate::Type::Octet ||
          key.secret().size() != key_wrap_bytes) {
        return std::nullopt;
      }

      content_key = aes_key_unwrap(key.secret(), jwe.encrypted_key());
      break;
    }
    case JWEAlgorithm::DIR:
      // The Encrypted Key must be empty and the shared secret is the CEK (RFC
      // 7518 Section 4.5)
      if (key.type() != JWKPrivate::Type::Octet ||
          key.secret().size() != content_key_bytes ||
          !jwe.encrypted_key().empty()) {
        return std::nullopt;
      }

      content_key = std::string{key.secret()};
      break;
    case JWEAlgorithm::ECDH_ES:
    case JWEAlgorithm::ECDH_ES_A128KW:
    case JWEAlgorithm::ECDH_ES_A192KW:
    case JWEAlgorithm::ECDH_ES_A256KW:
      if (key.type() != JWKPrivate::Type::EllipticCurve ||
          key.private_key() == nullptr) {
        return std::nullopt;
      }

      content_key = ecdh_content_key(jwe, key, algorithm.value(),
                                     encryption.value(), content_key_bytes);
      break;
  }

  // RFC 7516 Section 11.5: the recipient MUST NOT distinguish format, padding,
  // and length errors of the encrypted key. Substituting a random CEK, which
  // the section strongly recommends, for a missing or wrong-length one makes
  // decryption always proceed to the authentication tag check and fail there,
  // denying a key-unwrap oracle
  std::string effective_key{
      content_key.has_value() && content_key.value().size() == content_key_bytes
          ? std::move(content_key).value()
          : random_bytes(content_key_bytes)};

  return jwe_content_decrypt(
      encryption.value(), effective_key, jwe.initialization_vector(),
      jwe.protected_header(), jwe.ciphertext(), jwe.tag());
}

} // namespace sourcemeta::core
