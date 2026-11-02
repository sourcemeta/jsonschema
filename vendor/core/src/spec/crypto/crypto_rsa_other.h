#ifndef SOURCEMETA_CORE_CRYPTO_RSA_OTHER_H_
#define SOURCEMETA_CORE_CRYPTO_RSA_OTHER_H_

// The RSA private key operation for the reference backend, shared by signing
// (RSASP1, RFC 8017 Section 5.2.1) and decryption (RSADP, RFC 8017 Section
// 5.1.2), which compute the same function of the private key

#include "crypto_bignum.h"
#include "crypto_other.h"

#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint64_t
#include <optional> // std::optional, std::nullopt

namespace sourcemeta::core {

// RFC 8017 Section 5.1.2 step 2.b for the two-prime form: exponentiate modulo
// each prime with its CRT exponent and recombine, which costs about a quarter
// of one exponentiation modulo the modulus. Every value derives from the secret
// primes, so their Barrett constants are built in constant time and every
// intermediate is wiped. Returns no value when a prime is too short for the
// Barrett reduction of the input, which the word lengths fixed by the public
// key size decide
inline auto rsa_private_operation_crt(const PrivateKey::Internal &key,
                                      const Bignum &input,
                                      const std::size_t modulus_words)
    -> std::optional<Bignum> {
  auto prime1{bignum_from_bytes(key.prime1)};
  const SecureBignumScope prime1_scope{prime1};
  auto prime2{bignum_from_bytes(key.prime2)};
  const SecureBignumScope prime2_scope{prime2};
  if (bignum_is_zero(prime1) || bignum_is_zero(prime2) ||
      modulus_words > 2 * prime1.size || modulus_words > 2 * prime2.size) {
    return std::nullopt;
  }

  auto context1{barrett_context_ct(prime1)};
  const SecureBignumScope context1_modulus_scope{context1.modulus};
  const SecureBignumScope context1_factor_scope{context1.factor};
  auto context2{barrett_context_ct(prime2)};
  const SecureBignumScope context2_modulus_scope{context2.modulus};
  const SecureBignumScope context2_factor_scope{context2.factor};

  // RFC 8017 Section 5.1.2 step 2.b.i: "Let m_1 = c^dP mod p and m_2 = c^dQ
  // mod q"
  auto exponent1{bignum_from_bytes(key.exponent1)};
  const SecureBignumScope exponent1_scope{exponent1};
  auto message1{bignum_mod_exp_ct(input, exponent1, context1)};
  const SecureBignumScope message1_scope{message1};
  auto exponent2{bignum_from_bytes(key.exponent2)};
  const SecureBignumScope exponent2_scope{exponent2};
  auto message2{bignum_mod_exp_ct(input, exponent2, context2)};
  const SecureBignumScope message2_scope{message2};

  // RFC 8017 Section 5.1.2 step 2.b.iii: "Let h = (m_1 - m_2) * qInv mod p"
  auto message2_reduced{barrett_reduce(message2, context1)};
  const SecureBignumScope message2_reduced_scope{message2_reduced};
  auto difference{field_subtract_ct(message1, message2_reduced, context1)};
  const SecureBignumScope difference_scope{difference};
  auto coefficient{bignum_from_bytes(key.coefficient)};
  const SecureBignumScope coefficient_scope{coefficient};
  auto coefficient_reduced{barrett_reduce(coefficient, context1)};
  const SecureBignumScope coefficient_reduced_scope{coefficient_reduced};
  auto lift{field_mod_multiply_ct(difference, coefficient_reduced, context1)};
  const SecureBignumScope lift_scope{lift};

  // RFC 8017 Section 5.1.2 step 2.b.iv: "Let m = m_2 + q * h"
  auto product{
      bignum_multiply_fixed(prime2, lift, context2.words, context1.words)};
  const SecureBignumScope product_scope{product};
  const auto width{context1.words + context2.words};
  const auto *product_data{product.words.data()};
  const auto *message2_data{message2.words.data()};
  Bignum result;
  const SecureBignumScope result_scope{result};
  auto *result_data{result.words.data()};
  std::uint64_t carry{0};
  for (std::size_t index = 0; index < width; ++index) {
    const auto total{static_cast<BignumDoubleWord>(product_data[index]) +
                     message2_data[index] + carry};
    result_data[index] = static_cast<std::uint64_t>(total);
    carry = static_cast<std::uint64_t>(total >> 64U);
  }

  result.size = width;
  return result;
}

// Whether raising the candidate to the public exponent modulo the modulus gives
// back the input, which catches a CRT result corrupted by a fault or by
// inconsistent key components before it is released. The exponent is public,
// so its bits may steer the loop, while the multiplications stay constant time
// as the candidate may be a decrypted secret
inline auto rsa_private_result_matches(const Bignum &candidate,
                                       const Bignum &input,
                                       const Bignum &public_exponent,
                                       const BarrettContext &context) noexcept
    -> bool {
  const auto width{context.words};
  Bignum power;
  const SecureBignumScope power_scope{power};
  power.words[0] = 1;
  power.size = width;
  for (std::size_t index = bignum_bit_length(public_exponent); index > 0;
       --index) {
    power = field_square_ct(power, context);
    if (bignum_get_bit(public_exponent, index - 1)) {
      power = field_mod_multiply_ct(power, candidate, context);
    }
  }

  return field_equal_ct(power, input, context);
}

// RSASP1 and RSADP (RFC 8017 Sections 5.2.1 and 5.1.2) over an input already
// known to lie below the modulus. The two-prime form takes the CRT path, whose
// result is released only once raising it back to the public exponent
// reproduces the input, so a fault or inconsistent CRT components fall back to
// the private exponent rather than leak a corrupted value. A multi-prime key
// carries no CRT components here and uses the private exponent directly
inline auto rsa_private_operation(const PrivateKey::Internal &key,
                                  const Bignum &input) -> Bignum {
  const auto modulus{bignum_from_bytes(key.modulus)};
  const auto context{barrett_context(modulus)};
  if (!key.prime1.empty()) {
    auto candidate{rsa_private_operation_crt(key, input, modulus.size)};
    if (candidate.has_value()) {
      const SecureBignumScope candidate_scope{candidate.value()};
      if (rsa_private_result_matches(candidate.value(), input,
                                     bignum_from_bytes(key.public_exponent),
                                     context)) {
        return candidate.value();
      }
    }
  }

  auto exponent{bignum_from_bytes(key.private_exponent)};
  const SecureBignumScope exponent_scope{exponent};
  return bignum_mod_exp_ct(input, exponent, context);
}

} // namespace sourcemeta::core

#endif
