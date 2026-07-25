#include <sourcemeta/core/crypto_rsa_oaep.h>

#include "crypto_windows.h"

#include <windows.h> // ULONG, PUCHAR, LPCWSTR, DWORD
// clang-format off
#include <bcrypt.h> // BCrypt*, BCRYPT_*
// clang-format on

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

namespace {
auto to_cng_algorithm(const RSAOAEPHash hash) -> LPCWSTR {
  switch (hash) {
    case RSAOAEPHash::SHA1:
      return BCRYPT_SHA1_ALGORITHM;
    case RSAOAEPHash::SHA256:
      return BCRYPT_SHA256_ALGORITHM;
  }

  std::unreachable();
}

auto as_buffer(const std::string_view value) -> PUCHAR {
  return reinterpret_cast<PUCHAR>(const_cast<char *>(
      value.data())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}
} // namespace

auto rsa_oaep_encrypt(const PublicKey &key, const RSAOAEPHash hash,
                      const std::string_view plaintext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA) {
    return std::nullopt;
  }

  BCRYPT_OAEP_PADDING_INFO padding{};
  padding.pszAlgId = to_cng_algorithm(hash);
  padding.pbLabel = nullptr;
  padding.cbLabel = 0;

  ULONG length{0};
  if (!BCRYPT_SUCCESS(BCryptEncrypt(internal->key, as_buffer(plaintext),
                                    static_cast<ULONG>(plaintext.size()),
                                    &padding, nullptr, 0, nullptr, 0, &length,
                                    BCRYPT_PAD_OAEP))) {
    return std::nullopt;
  }

  std::string ciphertext(length, '\x00');
  ULONG written{0};
  if (!BCRYPT_SUCCESS(BCryptEncrypt(internal->key, as_buffer(plaintext),
                                    static_cast<ULONG>(plaintext.size()),
                                    &padding, nullptr, 0, as_buffer(ciphertext),
                                    static_cast<ULONG>(ciphertext.size()),
                                    &written, BCRYPT_PAD_OAEP))) {
    return std::nullopt;
  }

  ciphertext.resize(written);
  return ciphertext;
}

auto rsa_oaep_decrypt(const PrivateKey &key, const RSAOAEPHash hash,
                      const std::string_view ciphertext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA ||
      internal->rsa_pss_restricted) {
    return std::nullopt;
  }

  // Reject any ciphertext that is not exactly the modulus size, so that an
  // input with appended or missing bytes cannot decrypt as the same integer
  DWORD key_bits{0};
  ULONG property_size{0};
  if (!BCRYPT_SUCCESS(BCryptGetProperty(internal->key, BCRYPT_KEY_LENGTH,
                                        reinterpret_cast<PUCHAR>(&key_bits),
                                        sizeof(key_bits), &property_size, 0)) ||
      ciphertext.size() != (key_bits + 7u) / 8u) {
    return std::nullopt;
  }

  BCRYPT_OAEP_PADDING_INFO padding{};
  padding.pszAlgId = to_cng_algorithm(hash);
  padding.pbLabel = nullptr;
  padding.cbLabel = 0;

  ULONG length{0};
  // A failed OAEP check surfaces as an unsuccessful status
  if (!BCRYPT_SUCCESS(BCryptDecrypt(internal->key, as_buffer(ciphertext),
                                    static_cast<ULONG>(ciphertext.size()),
                                    &padding, nullptr, 0, nullptr, 0, &length,
                                    BCRYPT_PAD_OAEP))) {
    return std::nullopt;
  }

  std::string plaintext(length, '\x00');
  ULONG written{0};
  if (!BCRYPT_SUCCESS(BCryptDecrypt(internal->key, as_buffer(ciphertext),
                                    static_cast<ULONG>(ciphertext.size()),
                                    &padding, nullptr, 0, as_buffer(plaintext),
                                    static_cast<ULONG>(plaintext.size()),
                                    &written, BCRYPT_PAD_OAEP))) {
    return std::nullopt;
  }

  plaintext.resize(written);
  return plaintext;
}

} // namespace sourcemeta::core
