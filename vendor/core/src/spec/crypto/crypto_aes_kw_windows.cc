#include "crypto_aes.h"
#include "crypto_aes_kw_loop.h"

#include <windows.h> // ULONG, PUCHAR
// clang-format off
#include <bcrypt.h> // BCrypt*, BCRYPT_*
// clang-format on

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

// Windows CNG has no RFC 3394 primitive, so AES Key Wrap runs the shared loop
// over the AES block cipher in Electronic Codebook mode

namespace {
auto as_buffer(const std::string_view value) -> PUCHAR {
  return reinterpret_cast<PUCHAR>(const_cast<char *>(
      value.data())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

auto as_buffer(const wchar_t *const value) -> PUCHAR {
  return reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(
      value)); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

// One Electronic Codebook block, reporting a CNG failure or a short transform
// as no value so the shared loop stops rather than wrapping corrupt output
auto cipher_block(BCRYPT_KEY_HANDLE key, const bool encrypt,
                  const sourcemeta::core::AesKeyWrapBlock &input)
    -> std::optional<sourcemeta::core::AesKeyWrapBlock> {
  sourcemeta::core::AesKeyWrapBlock output{};
  auto *const input_buffer{reinterpret_cast<PUCHAR>(
      const_cast<
          std::uint8_t *>( // NOLINT(cppcoreguidelines-pro-type-const-cast)
          input.data()))};
  auto *const output_buffer{reinterpret_cast<PUCHAR>(output.data())};
  ULONG written{0};
  const auto status{
      encrypt
          ? BCryptEncrypt(key, input_buffer, static_cast<ULONG>(input.size()),
                          nullptr, nullptr, 0, output_buffer,
                          static_cast<ULONG>(output.size()), &written, 0)
          : BCryptDecrypt(key, input_buffer, static_cast<ULONG>(input.size()),
                          nullptr, nullptr, 0, output_buffer,
                          static_cast<ULONG>(output.size()), &written, 0)};
  if (!BCRYPT_SUCCESS(status) || written != output.size()) {
    return std::nullopt;
  }

  return output;
}
} // namespace

namespace sourcemeta::core {

auto aes_wrap(const std::string_view key_encryption_key,
              const std::string_view key) -> std::optional<std::string> {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0))) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  BCRYPT_KEY_HANDLE key_handle{nullptr};
  if (BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                                       as_buffer(BCRYPT_CHAIN_MODE_ECB),
                                       sizeof(BCRYPT_CHAIN_MODE_ECB), 0)) &&
      BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(
          algorithm, &key_handle, nullptr, 0, as_buffer(key_encryption_key),
          static_cast<ULONG>(key_encryption_key.size()), 0))) {
    result = aes_kw_wrap_loop(
        [key_handle](const AesKeyWrapBlock &block) {
          return cipher_block(key_handle, true, block);
        },
        key);
    BCryptDestroyKey(key_handle);
  }

  BCryptCloseAlgorithmProvider(algorithm, 0);
  return result;
}

auto aes_unwrap(const std::string_view key_encryption_key,
                const std::string_view wrapped_key)
    -> std::optional<std::string> {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0))) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  BCRYPT_KEY_HANDLE key_handle{nullptr};
  if (BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                                       as_buffer(BCRYPT_CHAIN_MODE_ECB),
                                       sizeof(BCRYPT_CHAIN_MODE_ECB), 0)) &&
      BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(
          algorithm, &key_handle, nullptr, 0, as_buffer(key_encryption_key),
          static_cast<ULONG>(key_encryption_key.size()), 0))) {
    result = aes_kw_unwrap_loop(
        [key_handle](const AesKeyWrapBlock &block) {
          return cipher_block(key_handle, false, block);
        },
        wrapped_key);
    BCryptDestroyKey(key_handle);
  }

  BCryptCloseAlgorithmProvider(algorithm, 0);
  return result;
}

} // namespace sourcemeta::core
