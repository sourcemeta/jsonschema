#include <sourcemeta/core/crypto_rsa_oaep.h>
#include <sourcemeta/core/crypto_sha256.h>

#include "crypto_bignum.h"
#include "crypto_helpers.h"
#include "crypto_other.h"
#include "crypto_random.h"
#include "crypto_sha1_other.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <exception>   // std::exception
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

// A from-scratch RSA-OAEP (RFC 8017 Section 7.1) for the reference backend,
// over the shared big integer arithmetic. The decode is not constant-time,
// which is acceptable only because this backend is the non-production fallback
// and the production backends run the platform's constant-time implementation.
// A failed check still surfaces as a single uniform failure, never a
// distinguishing one

namespace sourcemeta::core {

namespace {
auto hash_length(const RSAOAEPHash hash) -> std::size_t {
  switch (hash) {
    case RSAOAEPHash::SHA1:
      return 20;
    case RSAOAEPHash::SHA256:
      return 32;
  }

  std::unreachable();
}

auto oaep_hash(const RSAOAEPHash hash, const std::string_view input)
    -> std::string {
  switch (hash) {
    case RSAOAEPHash::SHA1: {
      const auto digest{sha1_digest(input)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         digest.size()};
    }
    case RSAOAEPHash::SHA256: {
      const auto digest{sha256_digest(input)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         digest.size()};
    }
  }

  std::unreachable();
}

// The mask generation function (RFC 8017 Appendix B.2.1)
auto mask_generation(const RSAOAEPHash hash, const std::string_view seed,
                     const std::size_t length) -> std::string {
  std::string result;
  result.reserve(length + hash_length(hash));
  std::uint32_t counter{0};
  while (result.size() < length) {
    std::string block{seed};
    append_big_endian_uint32(block, counter);
    result.append(oaep_hash(hash, block));
    counter += 1;
  }

  result.resize(length);
  return result;
}

auto exclusive_or(const std::string_view left, const std::string_view right)
    -> std::string {
  std::string result(left.size(), '\x00');
  for (std::size_t index = 0; index < left.size(); ++index) {
    result[index] = static_cast<char>(static_cast<std::uint8_t>(left[index]) ^
                                      static_cast<std::uint8_t>(right[index]));
  }

  return result;
}
} // namespace

auto rsa_oaep_encrypt(const PublicKey &key, const RSAOAEPHash hash,
                      const std::string_view plaintext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA) {
    return std::nullopt;
  }

  const auto key_length{internal->modulus.size()};
  const auto digest_length{hash_length(hash)};
  if (key_length < (2 * digest_length) + 2 ||
      plaintext.size() > key_length - (2 * digest_length) - 2) {
    return std::nullopt;
  }

  // DB = lHash || PS || 0x01 || M (RFC 8017 Section 7.1.1)
  const auto label_hash{oaep_hash(hash, "")};
  std::string data_block{label_hash};
  data_block.append(key_length - plaintext.size() - (2 * digest_length) - 2,
                    '\x00');
  data_block.push_back('\x01');
  data_block.append(plaintext);

  std::string seed(digest_length, '\x00');
  try {
    fill_random_bytes(std::span<std::uint8_t>{
        reinterpret_cast<std::uint8_t *>(seed.data()), digest_length});
  } catch (const std::exception &) {
    return std::nullopt;
  }

  const auto masked_data_block{exclusive_or(
      data_block, mask_generation(hash, seed, key_length - digest_length - 1))};
  const auto masked_seed{exclusive_or(
      seed, mask_generation(hash, masked_data_block, digest_length))};

  // EM = 0x00 || maskedSeed || maskedDB
  std::string encoded_message;
  encoded_message.reserve(key_length);
  encoded_message.push_back('\x00');
  encoded_message.append(masked_seed);
  encoded_message.append(masked_data_block);

  const auto ciphertext{bignum_mod_exp(bignum_from_bytes(encoded_message),
                                       bignum_from_bytes(internal->exponent),
                                       bignum_from_bytes(internal->modulus))};
  return bignum_to_bytes(ciphertext, key_length);
}

auto rsa_oaep_decrypt(const PrivateKey &key, const RSAOAEPHash hash,
                      const std::string_view ciphertext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA ||
      internal->rsa_pss_restricted) {
    return std::nullopt;
  }

  const auto key_length{internal->modulus.size()};
  const auto digest_length{hash_length(hash)};
  if (key_length < (2 * digest_length) + 2 || ciphertext.size() != key_length) {
    return std::nullopt;
  }

  const auto modulus{bignum_from_bytes(internal->modulus)};
  auto ciphertext_number{bignum_from_bytes(ciphertext)};
  if (bignum_compare(ciphertext_number, modulus) >= 0) {
    return std::nullopt;
  }

  auto exponent{bignum_from_bytes(internal->private_exponent)};
  const SecureBignumScope exponent_scope{exponent};
  auto message_number{
      bignum_mod_exp_ct(ciphertext_number, exponent, barrett_context(modulus))};
  const SecureBignumScope message_scope{message_number};
  const auto encoded_message{bignum_to_bytes(message_number, key_length)};

  // EM = Y || maskedSeed || maskedDB (RFC 8017 Section 7.1.2)
  const std::string_view masked_seed{encoded_message.data() + 1, digest_length};
  const std::string_view masked_data_block{encoded_message.data() + 1 +
                                               digest_length,
                                           key_length - digest_length - 1};
  const auto seed{exclusive_or(
      masked_seed, mask_generation(hash, masked_data_block, digest_length))};
  const auto data_block{exclusive_or(
      masked_data_block,
      mask_generation(hash, seed, key_length - digest_length - 1))};

  const auto label_hash{oaep_hash(hash, "")};
  // Accumulate every failure into a single value so that a malformed input is
  // rejected uniformly, never revealing which check failed
  std::uint8_t error{static_cast<std::uint8_t>(encoded_message[0])};
  for (std::size_t index = 0; index < digest_length; ++index) {
    error = static_cast<std::uint8_t>(
        error | (static_cast<std::uint8_t>(data_block[index]) ^
                 static_cast<std::uint8_t>(label_hash[index])));
  }

  std::size_t separator{0};
  std::uint8_t found{0};
  for (std::size_t index = digest_length; index < data_block.size(); ++index) {
    const auto byte{static_cast<std::uint8_t>(data_block[index])};
    const std::uint8_t is_one{static_cast<std::uint8_t>(byte == 0x01 ? 1 : 0)};
    const std::uint8_t is_zero{static_cast<std::uint8_t>(byte == 0x00 ? 1 : 0)};
    const std::uint8_t not_found{static_cast<std::uint8_t>(1 - found)};
    separator += static_cast<std::size_t>(not_found & is_one) * index;
    // A byte before the separator that is neither the padding zero nor the
    // separator one is malformed
    error = static_cast<std::uint8_t>(
        error | (not_found & static_cast<std::uint8_t>(1 - is_zero) &
                 static_cast<std::uint8_t>(1 - is_one)));
    found = static_cast<std::uint8_t>(found | is_one);
  }

  error =
      static_cast<std::uint8_t>(error | static_cast<std::uint8_t>(1 - found));
  if (error != 0) {
    return std::nullopt;
  }

  return data_block.substr(separator + 1);
}

} // namespace sourcemeta::core
