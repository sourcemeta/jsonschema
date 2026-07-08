#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/text.h>

#include "crypto_bignum.h"
#include "crypto_ecc.h"
#include "crypto_eddsa.h"
#include "crypto_helpers.h"
#include "crypto_pkcs8.h"

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {
namespace {

// The DigestInfo prefixes for the EMSA-PKCS1-v1_5 encoding, taken verbatim
// from RFC 8017 Section 9.2 Note 1
constexpr std::array<std::uint8_t, 19> DIGEST_INFO_SHA256{
    {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
     0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20}};
constexpr std::array<std::uint8_t, 19> DIGEST_INFO_SHA384{
    {0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
     0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30}};
constexpr std::array<std::uint8_t, 19> DIGEST_INFO_SHA512{
    {0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
     0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40}};

auto digest_info_prefix(const SignatureHashFunction hash) -> std::string_view {
  switch (hash) {
    case SignatureHashFunction::SHA256:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA256.data()),
              DIGEST_INFO_SHA256.size()};
    case SignatureHashFunction::SHA384:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA384.data()),
              DIGEST_INFO_SHA384.size()};
    case SignatureHashFunction::SHA512:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA512.data()),
              DIGEST_INFO_SHA512.size()};
  }

  std::unreachable();
}

// EMSA-PKCS1-v1_5 encoding (RFC 8017 Section 9.2)
auto build_encoded_message(const SignatureHashFunction hash,
                           const std::string_view message,
                           const std::size_t key_length)
    -> std::optional<std::string> {
  const auto prefix{digest_info_prefix(hash)};
  const auto digest{digest_message(hash, message)};
  const auto encoded_length{prefix.size() + digest.size()};
  if (key_length < encoded_length + 11) {
    return std::nullopt;
  }

  std::string result;
  result.reserve(key_length);
  result.push_back('\x00');
  result.push_back('\x01');
  result.append(key_length - encoded_length - 3, '\xff');
  result.push_back('\x00');
  result.append(prefix);
  result.append(digest);
  return result;
}

// MGF1 mask generation (RFC 8017 Appendix B.2.1)
auto mask_generation(const SignatureHashFunction hash,
                     const std::string_view seed, const std::size_t length)
    -> std::string {
  std::string result;
  result.reserve(length + 64);
  std::uint32_t counter{0};
  while (result.size() < length) {
    std::string block{seed};
    block.push_back(static_cast<char>((counter >> 24u) & 0xffu));
    block.push_back(static_cast<char>((counter >> 16u) & 0xffu));
    block.push_back(static_cast<char>((counter >> 8u) & 0xffu));
    block.push_back(static_cast<char>(counter & 0xffu));
    result.append(digest_message(hash, block));
    counter += 1;
  }

  result.resize(length);
  return result;
}

// EMSA-PSS encoding (RFC 8017 Section 9.1.1), with the salt length fixed to the
// hash function output as RFC 7518 Section 3.5 requires
auto emsa_pss_encode(const SignatureHashFunction hash,
                     const std::string_view message,
                     const std::size_t encoded_bits)
    -> std::optional<std::string> {
  const auto message_digest{digest_message(hash, message)};
  const auto hash_length{message_digest.size()};
  const auto salt_length{hash_length};
  const auto encoded_length{(encoded_bits + 7) / 8};
  if (encoded_length < hash_length + salt_length + 2) {
    return std::nullopt;
  }

  const auto salt{random_bytes(salt_length)};
  std::string primed(8, '\x00');
  primed.append(message_digest);
  primed.append(salt);
  const auto hash_value{digest_message(hash, primed)};

  const auto database_length{encoded_length - hash_length - 1};
  std::string database(database_length - salt_length - 1, '\x00');
  database.push_back('\x01');
  database.append(salt);
  const auto mask{mask_generation(hash, hash_value, database_length)};
  for (std::size_t index = 0; index < database_length; ++index) {
    database[index] = static_cast<char>(database[index] ^ mask[index]);
  }

  // RFC 8017 Section 9.1.1 step 11: clear the leftmost 8emLen - emBits bits
  const auto unused_bits{(8 * encoded_length) - encoded_bits};
  const auto unused_mask{
      static_cast<std::uint8_t>((0xff00u >> unused_bits) & 0xffu)};
  database[0] = static_cast<char>(static_cast<std::uint8_t>(database[0]) &
                                  static_cast<std::uint8_t>(~unused_mask));

  std::string result{std::move(database)};
  result.append(hash_value);
  result.push_back('\xbc');
  return result;
}

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

struct HashSizes {
  std::size_t block_bytes;
  std::size_t output_bytes;
};

auto hash_sizes(const SignatureHashFunction hash) -> HashSizes {
  switch (hash) {
    case SignatureHashFunction::SHA256:
      return {.block_bytes = 64, .output_bytes = 32};
    case SignatureHashFunction::SHA384:
      return {.block_bytes = 128, .output_bytes = 48};
    case SignatureHashFunction::SHA512:
      return {.block_bytes = 128, .output_bytes = 64};
  }

  std::unreachable();
}

// HMAC (RFC 2104) keyed on the signature hash function, the primitive that the
// deterministic nonce generator is built on
auto hmac(const SignatureHashFunction hash, const std::string_view key,
          const std::string_view message) -> std::string {
  const auto block_bytes{hash_sizes(hash).block_bytes};
  std::string block_key{key.size() > block_bytes ? digest_message(hash, key)
                                                 : std::string{key}};
  block_key.resize(block_bytes, '\x00');

  std::string inner_input(block_bytes, '\x00');
  std::string outer_input(block_bytes, '\x00');
  for (std::size_t index{0}; index < block_bytes; ++index) {
    const auto byte{static_cast<unsigned char>(block_key[index])};
    inner_input[index] = static_cast<char>(byte ^ 0x36u);
    outer_input[index] = static_cast<char>(byte ^ 0x5cu);
  }

  inner_input.append(message);
  outer_input.append(digest_message(hash, inner_input));
  return digest_message(hash, outer_input);
}

// RFC 6979 Section 2.3.2 bits2int, which is also the FIPS 186-4 Section 6.4
// truncation of a bit string to the leftmost order-length bits
auto bits2int(const std::string_view bits, const std::size_t order_bits)
    -> Bignum {
  auto value{bignum_from_bytes(bits)};
  const auto bit_length{bits.size() * 8};
  if (bit_length > order_bits) {
    value = bignum_shift_right(value, bit_length - order_bits);
  }

  return value;
}

// RFC 6979 Section 2.3.4 bits2octets, reducing the truncated hash modulo the
// order and encoding it to the fixed octet width
auto bits2octets(const std::string_view bits, const Bignum &order,
                 const std::size_t order_bits, const std::size_t order_bytes)
    -> std::string {
  auto value{bits2int(bits, order_bits)};
  bignum_reduce(value, order);
  return bignum_to_bytes(value, order_bytes);
}

auto sign_rsa(const std::string_view modulus,
              const std::string_view private_exponent,
              const std::string_view encoded_message) -> std::string {
  // The exponent is the secret private key, so the exponentiation runs in
  // constant time; the exponent copy it consumes is wiped before returning
  const auto context{barrett_context(bignum_from_bytes(modulus))};
  auto exponent{bignum_from_bytes(private_exponent)};
  const SecureBignumScope exponent_scope{exponent};
  auto representative{
      bignum_mod_exp_ct(bignum_from_bytes(encoded_message), exponent, context)};
  bignum_normalize(representative);
  return bignum_to_bytes(representative, modulus.size());
}

// The signature for one nonce candidate (FIPS 186-4 Section 6.4.1), returning
// no value when the candidate must be rejected and a fresh one drawn
auto ecdsa_signature_for_nonce(const Bignum &nonce,
                               const Bignum &digest_integer,
                               const Bignum &private_scalar,
                               const JacobianPoint &generator,
                               const EllipticCurveParameters &parameters,
                               const std::size_t field_bytes)
    -> std::optional<std::string> {
  if (bignum_is_zero(nonce) || bignum_compare(nonce, parameters.order) >= 0) {
    return std::nullopt;
  }

  const auto point{
      point_scalar_multiply_constant_time(nonce, generator, parameters)};

  // The ladder leaves its projective output fixed-width rather than normalized,
  // so it does not leak the secret nonce through a value-dependent loop. The
  // nonce lies in [1, n), so the result is never the point at infinity, and the
  // r == 0 rejection below is the FIPS 186-4 restart condition regardless
  auto r{point_affine_x_constant_time(point, parameters)};
  bignum_reduce(r, parameters.order);
  if (bignum_is_zero(r)) {
    return std::nullopt;
  }

  // s = k^-1 (z + r * d) mod n, evaluated over the constant-time field
  // arithmetic modulo the order. The nonce inverse and every partial sum that
  // mixes in the private scalar d carry secret material, so each is wiped
  // before returning; the resulting r and s are the public signature
  const auto order_field{barrett_context(parameters.order)};
  auto nonce_inverse{field_inverse_ct(nonce, order_field)};
  const SecureBignumScope nonce_inverse_scope{nonce_inverse};
  auto private_reduced{barrett_reduce(private_scalar, order_field)};
  const SecureBignumScope private_reduced_scope{private_reduced};
  auto private_product{field_mod_multiply_ct(r, private_reduced, order_field)};
  const SecureBignumScope private_product_scope{private_product};
  const auto digest_reduced{barrett_reduce(digest_integer, order_field)};
  auto shifted_digest{
      field_add_ct(digest_reduced, private_product, order_field)};
  const SecureBignumScope shifted_digest_scope{shifted_digest};
  auto s{field_mod_multiply_ct(nonce_inverse, shifted_digest, order_field)};
  bignum_normalize(s);
  if (bignum_is_zero(s)) {
    return std::nullopt;
  }

  std::string signature{bignum_to_bytes(r, field_bytes)};
  signature.append(bignum_to_bytes(s, field_bytes));
  return signature;
}

// ECDSA signature generation (FIPS 186-4 Section 6.4.1) with the per-signature
// nonce derived deterministically from the private key and the message digest,
// so that the signature never depends on the quality of the random generator
// (RFC 6979 Section 3.2)
auto sign_ecdsa(const EllipticCurve curve, const SignatureHashFunction hash,
                const std::string_view scalar, const std::string_view message)
    -> std::optional<std::string> {
  const auto parameters{to_curve_parameters(curve)};
  const auto field_bytes{parameters.field_bytes};
  const auto order_bits{bignum_bit_length(parameters.order)};
  const auto order_bytes{(order_bits + 7) / 8};
  const auto digest{digest_message(hash, message)};
  const auto digest_integer{bits2int(digest, order_bits)};
  auto private_scalar{bignum_from_bytes(scalar)};
  const SecureBignumScope private_scalar_scope{private_scalar};
  const JacobianPoint generator{.x = parameters.generator_x,
                                .y = parameters.generator_y,
                                .z = bignum_from_u64(1)};

  auto private_octets{bignum_to_bytes(private_scalar, order_bytes)};
  const SecureScope private_octets_scope{private_octets};
  const auto hashed_octets{
      bits2octets(digest, parameters.order, order_bits, order_bytes)};

  // RFC 6979 Section 3.2 steps b to g: seed the HMAC generator. The generator
  // state and the private octets are derived from the private key, so each is
  // wiped when leaving this function
  const auto output_bytes{hash_sizes(hash).output_bytes};
  std::string hmac_value(output_bytes, '\x01');
  const SecureScope hmac_value_scope{hmac_value};
  std::string hmac_key(output_bytes, '\x00');
  const SecureScope hmac_key_scope{hmac_key};
  std::string seed{hmac_value};
  const SecureScope seed_scope{seed};
  seed.push_back('\x00');
  seed.append(private_octets);
  seed.append(hashed_octets);
  hmac_key = hmac(hash, hmac_key, seed);
  hmac_value = hmac(hash, hmac_key, hmac_value);
  seed = hmac_value;
  seed.push_back('\x01');
  seed.append(private_octets);
  seed.append(hashed_octets);
  hmac_key = hmac(hash, hmac_key, seed);
  hmac_value = hmac(hash, hmac_key, hmac_value);

  for (std::size_t attempt = 0; attempt < 256; ++attempt) {
    // RFC 6979 Section 3.2 step h.2: draw enough output to cover the order. The
    // candidate is the secret nonce, so it is wiped when the attempt ends
    std::string candidate;
    const SecureScope candidate_scope{candidate};
    while (candidate.size() * 8 < order_bits) {
      hmac_value = hmac(hash, hmac_key, hmac_value);
      candidate.append(hmac_value);
    }

    auto nonce{bits2int(candidate, order_bits)};
    const SecureBignumScope nonce_scope{nonce};
    const auto signature{ecdsa_signature_for_nonce(nonce, digest_integer,
                                                   private_scalar, generator,
                                                   parameters, field_bytes)};
    if (signature.has_value()) {
      return signature;
    }

    // RFC 6979 Section 3.2 step h: reseed before the next candidate
    std::string reseed{hmac_value};
    const SecureScope reseed_scope{reseed};
    reseed.push_back('\x00');
    hmac_key = hmac(hash, hmac_key, reseed);
    hmac_value = hmac(hash, hmac_key, hmac_value);
  }

  return std::nullopt;
}

} // namespace

// The reference backend parses the key material into big integers inside each
// signature, so the parsed key simply holds the raw material
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
};

PrivateKey::PrivateKey(Internal *internal) noexcept : internal_{internal} {}

PrivateKey::~PrivateKey() {
  if (internal_ != nullptr) {
    secure_zero(internal_->private_exponent);
    secure_zero(internal_->scalar);
    secure_zero(internal_->edwards_seed);
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
      secure_zero(internal_->private_exponent);
      secure_zero(internal_->scalar);
      secure_zero(internal_->edwards_seed);
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
  auto der{pem_to_der(pem)};
  if (!der.has_value()) {
    return std::nullopt;
  }

  // The decoded PKCS#8 holds the whole private key, so it is wiped on return
  const SecureScope der_scope{der.value()};
  const auto parsed{parse_pkcs8(der.value())};
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (parsed->kind) {
    case PKCS8KeyKind::RSA: {
      // RFC 8017 Appendix A.1.2: RSAPrivateKey is a SEQUENCE of version,
      // modulus, publicExponent, privateExponent, and the further primes
      const auto sequence{der_read(parsed->key)};
      if (!sequence.has_value() || sequence->tag != 0x30) {
        return std::nullopt;
      }

      const auto version{der_read(sequence->content)};
      if (!version.has_value() || version->tag != 0x02) {
        return std::nullopt;
      }

      const auto modulus{der_read(version->rest)};
      if (!modulus.has_value() || modulus->tag != 0x02) {
        return std::nullopt;
      }

      const auto public_exponent{der_read(modulus->rest)};
      if (!public_exponent.has_value() || public_exponent->tag != 0x02) {
        return std::nullopt;
      }

      const auto private_exponent{der_read(public_exponent->rest)};
      if (!private_exponent.has_value() || private_exponent->tag != 0x02) {
        return std::nullopt;
      }

      const auto stripped_modulus{strip_left(modulus->content, '\x00')};
      const auto stripped_private_exponent{
          strip_left(private_exponent->content, '\x00')};
      if (stripped_modulus.empty() ||
          stripped_modulus.size() > MAXIMUM_KEY_BYTES ||
          stripped_private_exponent.size() > MAXIMUM_KEY_BYTES) {
        return std::nullopt;
      }

      return PrivateKey{new PrivateKey::Internal{
          .kind = PrivateKey::Type::RSA,
          .modulus = std::string{stripped_modulus},
          .public_exponent =
              std::string{strip_left(public_exponent->content, '\x00')},
          .private_exponent = std::string{stripped_private_exponent},
          .scalar = {},
          .elliptic_curve = {},
          .edwards_seed = {},
          .edwards_curve = {},
          .rsa_pss_restricted = parsed->rsa_pss_restricted}};
    }
    case PKCS8KeyKind::EllipticCurve: {
      // RFC 5915 Section 3: ECPrivateKey is a SEQUENCE whose second field is
      // the private scalar as an OCTET STRING
      const auto sequence{der_read(parsed->key)};
      if (!sequence.has_value() || sequence->tag != 0x30) {
        return std::nullopt;
      }

      const auto version{der_read(sequence->content)};
      if (!version.has_value() || version->tag != 0x02) {
        return std::nullopt;
      }

      const auto scalar{der_read(version->rest)};
      if (!scalar.has_value() || scalar->tag != 0x04) {
        return std::nullopt;
      }

      const auto stripped_scalar{strip_left(scalar->content, '\x00')};
      const auto scalar_width{curve_field_bytes(parsed->curve)};
      // A zero or oversized scalar is not a valid private key
      if (stripped_scalar.empty() || stripped_scalar.size() > scalar_width) {
        return std::nullopt;
      }

      return PrivateKey{new PrivateKey::Internal{
          .kind = PrivateKey::Type::EllipticCurve,
          .modulus = {},
          .public_exponent = {},
          .private_exponent = {},
          .scalar =
              std::string{pad_left(stripped_scalar, scalar_width, '\x00')},
          .elliptic_curve = parsed->curve,
          .edwards_seed = {},
          .edwards_curve = {}}};
    }
    case PKCS8KeyKind::Edwards: {
      const auto seed{der_read(parsed->key)};
      if (!seed.has_value() || seed->tag != 0x04 ||
          seed->content.size() !=
              eddsa_public_key_bytes(parsed->edwards_curve)) {
        return std::nullopt;
      }

      return PrivateKey{
          new PrivateKey::Internal{.kind = PrivateKey::Type::Edwards,
                                   .modulus = {},
                                   .public_exponent = {},
                                   .private_exponent = {},
                                   .scalar = {},
                                   .elliptic_curve = {},
                                   .edwards_seed = std::string{seed->content},
                                   .edwards_curve = parsed->edwards_curve}};
    }
  }

  std::unreachable();
}

auto make_ec_private_key(const EllipticCurve curve,
                         const std::string_view scalar,
                         const std::string_view coordinate_x,
                         const std::string_view coordinate_y)
    -> std::optional<PrivateKey> {
  const auto stripped{strip_left(scalar, '\x00')};
  const auto width{curve_field_bytes(curve)};
  // The scalar alone drives signing here, but the public coordinates are still
  // range-checked so that malformed input is rejected as on the other backends
  if (stripped.empty() || stripped.size() > width ||
      strip_left(coordinate_x, '\x00').size() > width ||
      strip_left(coordinate_y, '\x00').size() > width ||
      !ec_private_scalar_in_range(scalar, curve)) {
    return std::nullopt;
  }

  return PrivateKey{new PrivateKey::Internal{
      .kind = PrivateKey::Type::EllipticCurve,
      .modulus = {},
      .public_exponent = {},
      .private_exponent = {},
      .scalar = std::string{pad_left(stripped, width, '\x00')},
      .elliptic_curve = curve,
      .edwards_seed = {},
      .edwards_curve = {}}};
}

auto make_edwards_private_key(const EdwardsCurve curve,
                              const std::string_view seed)
    -> std::optional<PrivateKey> {
  if (seed.size() != eddsa_public_key_bytes(curve)) {
    return std::nullopt;
  }

  return PrivateKey{new PrivateKey::Internal{.kind = PrivateKey::Type::Edwards,
                                             .modulus = {},
                                             .public_exponent = {},
                                             .private_exponent = {},
                                             .scalar = {},
                                             .elliptic_curve = {},
                                             .edwards_seed = std::string{seed},
                                             .edwards_curve = curve}};
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

  const auto encoded{
      build_encoded_message(hash, message, internal->modulus.size())};
  if (!encoded.has_value()) {
    return std::nullopt;
  }

  return sign_rsa(internal->modulus, internal->private_exponent,
                  encoded.value());
}

auto rsassa_pss_sign(const PrivateKey &key, const SignatureHashFunction hash,
                     const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA) {
    return std::nullopt;
  }

  const auto encoded_bits{
      bignum_bit_length(bignum_from_bytes(internal->modulus)) - 1};
  const auto encoded{emsa_pss_encode(hash, message, encoded_bits)};
  if (!encoded.has_value()) {
    return std::nullopt;
  }

  return sign_rsa(internal->modulus, internal->private_exponent,
                  encoded.value());
}

auto ecdsa_sign(const PrivateKey &key, const SignatureHashFunction hash,
                const std::string_view message) -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr ||
      internal->kind != PrivateKey::Type::EllipticCurve) {
    return std::nullopt;
  }

  return sign_ecdsa(internal->elliptic_curve, hash, internal->scalar, message);
}

auto eddsa_sign(const PrivateKey &key, const std::string_view message)
    -> std::optional<std::string> {
  const auto *internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::Edwards) {
    return std::nullopt;
  }

  switch (internal->edwards_curve) {
    case EdwardsCurve::Ed25519:
      return edwards25519_sign(internal->edwards_seed, message);
    case EdwardsCurve::Ed448:
      return edwards448_sign(internal->edwards_seed, message);
  }

  std::unreachable();
}

} // namespace sourcemeta::core
