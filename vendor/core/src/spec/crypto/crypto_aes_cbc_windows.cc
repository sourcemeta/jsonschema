#include "crypto_aes.h"

#include <windows.h> // ULONG, PUCHAR
// clang-format off
#include <bcrypt.h> // BCrypt*, BCRYPT_*
// clang-format on

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
auto as_buffer(const std::string_view value) -> PUCHAR {
  return reinterpret_cast<PUCHAR>(const_cast<char *>(
      value.data())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

auto as_buffer(std::string &value) -> PUCHAR {
  return reinterpret_cast<PUCHAR>(value.data());
}

// The chaining mode is a wide string constant rather than caller-owned bytes
auto as_buffer(const wchar_t *const value) -> PUCHAR {
  return reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(
      value)); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

// Both directions share the setup, differing only in the transform, since
// BCryptEncrypt and BCryptDecrypt take the same arguments over whole blocks
auto apply_cbc(const bool encrypt, const std::string_view key,
               const std::string_view iv, const std::string_view input)
    -> std::optional<std::string> {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0))) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  BCRYPT_KEY_HANDLE key_handle{nullptr};
  if (BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                                       as_buffer(BCRYPT_CHAIN_MODE_CBC),
                                       sizeof(BCRYPT_CHAIN_MODE_CBC), 0)) &&
      BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(
          algorithm, &key_handle, nullptr, 0, as_buffer(key),
          static_cast<ULONG>(key.size()), 0))) {
    // The chaining vector is updated in place, so it is copied from the caller
    std::string chaining{iv};
    std::string output(input.size(), '\x00');
    ULONG written{0};
    const auto status{
        encrypt ? BCryptEncrypt(key_handle, as_buffer(input),
                                static_cast<ULONG>(input.size()), nullptr,
                                as_buffer(chaining),
                                static_cast<ULONG>(chaining.size()),
                                as_buffer(output),
                                static_cast<ULONG>(output.size()), &written, 0)
                : BCryptDecrypt(
                      key_handle, as_buffer(input),
                      static_cast<ULONG>(input.size()), nullptr,
                      as_buffer(chaining), static_cast<ULONG>(chaining.size()),
                      as_buffer(output), static_cast<ULONG>(output.size()),
                      &written, 0)};
    if (BCRYPT_SUCCESS(status)) {
      result = std::move(output);
    }

    BCryptDestroyKey(key_handle);
  }

  BCryptCloseAlgorithmProvider(algorithm, 0);
  return result;
}
} // namespace

namespace sourcemeta::core {

auto aes_cbc_encrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view plaintext)
    -> std::optional<std::string> {
  return apply_cbc(true, key, iv, plaintext);
}

auto aes_cbc_decrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view ciphertext)
    -> std::optional<std::string> {
  return apply_cbc(false, key, iv, ciphertext);
}

} // namespace sourcemeta::core
