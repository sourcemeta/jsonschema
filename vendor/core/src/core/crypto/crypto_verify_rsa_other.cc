#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_bignum.h"
#include "crypto_helpers.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

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

auto digest_info_prefix(const sourcemeta::core::SignatureHashFunction hash)
    -> std::string_view {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA256.data()),
              DIGEST_INFO_SHA256.size()};
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA384.data()),
              DIGEST_INFO_SHA384.size()};
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return {reinterpret_cast<const char *>(DIGEST_INFO_SHA512.data()),
              DIGEST_INFO_SHA512.size()};
  }

  std::unreachable();
}

// EMSA-PKCS1-v1_5 encoding (RFC 8017 Section 9.2)
auto build_encoded_message(const sourcemeta::core::SignatureHashFunction hash,
                           const std::string_view message,
                           const std::size_t key_length)
    -> std::optional<std::string> {
  const auto prefix{digest_info_prefix(hash)};
  const auto digest{sourcemeta::core::digest_message(hash, message)};
  const auto encoded_length{prefix.size() + digest.size()};

  // RFC 8017 Section 9.2 step 3: "If emLen < tLen + 11, output 'intended
  // encoded message length too short'"
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
auto mask_generation(const sourcemeta::core::SignatureHashFunction hash,
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
    result.append(sourcemeta::core::digest_message(hash, block));
    counter += 1;
  }

  result.resize(length);
  return result;
}

// EMSA-PSS verification (RFC 8017 Section 9.1.2), with the salt length
// fixed to the hash function output as RFC 7518 Section 3.5 requires
auto emsa_pss_verify(const sourcemeta::core::SignatureHashFunction hash,
                     const std::string_view message,
                     const std::string_view encoded_message,
                     const std::size_t encoded_bits) -> bool {
  const auto digest{sourcemeta::core::digest_message(hash, message)};
  const auto hash_length{digest.size()};
  const auto salt_length{hash_length};
  const auto encoded_length{encoded_message.size()};

  // RFC 8017 Section 9.1.2 step 3: "If emLen < hLen + sLen + 2, output
  // 'inconsistent'"
  if (encoded_length < hash_length + salt_length + 2) {
    return false;
  }

  // RFC 8017 Section 9.1.2 step 4: "If the rightmost octet of EM does not
  // have hexadecimal value 0xbc, output 'inconsistent'"
  if (static_cast<std::uint8_t>(encoded_message.back()) != 0xbc) {
    return false;
  }

  const auto database_length{encoded_length - hash_length - 1};
  const auto masked_database{encoded_message.substr(0, database_length)};
  const auto hash_value{encoded_message.substr(database_length, hash_length)};

  // RFC 8017 Section 9.1.2 step 6: "If the leftmost 8emLen - emBits bits of
  // the leftmost octet in maskedDB are not all equal to zero, output
  // 'inconsistent'"
  const auto unused_bits{(8 * encoded_length) - encoded_bits};
  const auto unused_mask{
      static_cast<std::uint8_t>((0xff00u >> unused_bits) & 0xffu)};
  if ((static_cast<std::uint8_t>(masked_database.front()) & unused_mask) != 0) {
    return false;
  }

  auto database{mask_generation(hash, hash_value, database_length)};
  for (std::size_t index = 0; index < database_length; ++index) {
    database[index] =
        static_cast<char>(database[index] ^ masked_database[index]);
  }

  database[0] = static_cast<char>(static_cast<std::uint8_t>(database[0]) &
                                  static_cast<std::uint8_t>(~unused_mask));

  // RFC 8017 Section 9.1.2 step 10: "If the emLen - hLen - sLen - 2
  // leftmost octets of DB are not zero or if the octet at position
  // emLen - hLen - sLen - 1 does not have hexadecimal value 0x01, output
  // 'inconsistent'"
  const auto padding_length{encoded_length - hash_length - salt_length - 2};
  for (std::size_t index = 0; index < padding_length; ++index) {
    if (database[index] != '\x00') {
      return false;
    }
  }

  if (static_cast<std::uint8_t>(database[padding_length]) != 0x01) {
    return false;
  }

  // RFC 8017 Section 9.1.2 steps 12 and 13: hash the concatenation of eight
  // zero octets, the message digest, and the recovered salt
  std::string verification_input(8, '\x00');
  verification_input.append(digest);
  verification_input.append(database.substr(database_length - salt_length));
  const auto expected{
      sourcemeta::core::digest_message(hash, verification_input)};
  return expected == hash_value;
}

} // namespace

namespace sourcemeta::core {

auto rsassa_pkcs1_v15_verify(const SignatureHashFunction hash,
                             const std::string_view modulus,
                             const std::string_view exponent,
                             const std::string_view message,
                             const std::string_view signature) -> bool {
  const auto stripped_modulus{sourcemeta::core::strip_left(modulus, '\x00')};
  const auto stripped_exponent{sourcemeta::core::strip_left(exponent, '\x00')};
  if (stripped_modulus.empty() || stripped_exponent.empty() ||
      stripped_modulus.size() > sourcemeta::core::MAXIMUM_KEY_BYTES ||
      stripped_exponent.size() > sourcemeta::core::MAXIMUM_KEY_BYTES) {
    return false;
  }

  // RFC 8017 Section 8.2.2 step 1: "If the length of S is not k octets,
  // output 'invalid signature'"
  const auto key_length{stripped_modulus.size()};
  if (signature.size() != key_length) {
    return false;
  }

  const auto modulus_number{bignum_from_bytes(stripped_modulus)};
  const auto signature_number{bignum_from_bytes(signature)};

  // RFC 8017 Section 5.2.2: "If the signature representative s is not
  // between 0 and n - 1, output 'signature representative out of range'"
  if (bignum_compare(signature_number, modulus_number) >= 0) {
    return false;
  }

  const auto exponent_number{bignum_from_bytes(stripped_exponent)};
  const auto message_representative{
      bignum_mod_exp(signature_number, exponent_number, modulus_number)};
  const auto encoded_message{
      bignum_to_bytes(message_representative, key_length)};
  const auto expected{build_encoded_message(hash, message, key_length)};
  return expected.has_value() && encoded_message == expected.value();
}

auto rsassa_pss_verify(const SignatureHashFunction hash,
                       const std::string_view modulus,
                       const std::string_view exponent,
                       const std::string_view message,
                       const std::string_view signature) -> bool {
  const auto stripped_modulus{sourcemeta::core::strip_left(modulus, '\x00')};
  const auto stripped_exponent{sourcemeta::core::strip_left(exponent, '\x00')};
  if (stripped_modulus.empty() || stripped_exponent.empty() ||
      stripped_modulus.size() > sourcemeta::core::MAXIMUM_KEY_BYTES ||
      stripped_exponent.size() > sourcemeta::core::MAXIMUM_KEY_BYTES) {
    return false;
  }

  // RFC 8017 Section 8.1.2 step 1: "If the length of the signature S is not
  // k octets, output 'invalid signature'"
  const auto key_length{stripped_modulus.size()};
  if (signature.size() != key_length) {
    return false;
  }

  const auto modulus_number{bignum_from_bytes(stripped_modulus)};
  const auto signature_number{bignum_from_bytes(signature)};

  // RFC 8017 Section 5.2.2: "If the signature representative s is not
  // between 0 and n - 1, output 'signature representative out of range'"
  if (bignum_compare(signature_number, modulus_number) >= 0) {
    return false;
  }

  const auto exponent_number{bignum_from_bytes(stripped_exponent)};
  const auto message_representative{
      bignum_mod_exp(signature_number, exponent_number, modulus_number)};

  // RFC 8017 Section 8.1.2 step 2c: the encoded message is emLen octets
  // long, where emLen equals the byte length of emBits = modBits - 1 bits,
  // which is one octet less than k when the modulus bit length is congruent
  // to one modulo eight
  const auto encoded_bits{bignum_bit_length(modulus_number) - 1};
  const auto encoded_length{(encoded_bits + 7) / 8};
  const auto full_representative{
      bignum_to_bytes(message_representative, key_length)};
  for (std::size_t index = 0; index < key_length - encoded_length; ++index) {
    if (full_representative[index] != '\x00') {
      return false;
    }
  }

  const auto encoded_message{std::string_view{full_representative}.substr(
      key_length - encoded_length)};
  return emsa_pss_verify(hash, message, encoded_message, encoded_bits);
}

} // namespace sourcemeta::core
