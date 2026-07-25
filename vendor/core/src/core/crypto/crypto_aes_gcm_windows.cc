#include "crypto_aes.h"

#include <windows.h> // ULONG, PUCHAR
// clang-format off
#include <bcrypt.h> // BCrypt*, BCRYPT_*
// clang-format on

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
constexpr std::size_t TAG_BYTES{16};

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
} // namespace

namespace sourcemeta::core {

auto aes_gcm_seal(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext> {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0))) {
    return std::nullopt;
  }

  std::optional<AESGCMCiphertext> result;
  BCRYPT_KEY_HANDLE key_handle{nullptr};
  if (BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                                       as_buffer(BCRYPT_CHAIN_MODE_GCM),
                                       sizeof(BCRYPT_CHAIN_MODE_GCM), 0)) &&
      BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(
          algorithm, &key_handle, nullptr, 0, as_buffer(key),
          static_cast<ULONG>(key.size()), 0))) {
    // A single buffer holds the ciphertext followed by its 16-byte tag, which
    // the mode info writes into at the offset past the ciphertext
    std::string data(plaintext.size() + TAG_BYTES, '\x00');
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
    BCRYPT_INIT_AUTH_MODE_INFO(info);
    info.pbNonce = as_buffer(iv);
    info.cbNonce = static_cast<ULONG>(iv.size());
    info.pbAuthData = as_buffer(associated_data);
    info.cbAuthData = static_cast<ULONG>(associated_data.size());
    info.pbTag = as_buffer(data) + plaintext.size();
    info.cbTag = static_cast<ULONG>(TAG_BYTES);

    ULONG written{0};
    if (BCRYPT_SUCCESS(BCryptEncrypt(key_handle, as_buffer(plaintext),
                                     static_cast<ULONG>(plaintext.size()),
                                     &info, nullptr, 0, as_buffer(data),
                                     static_cast<ULONG>(plaintext.size()),
                                     &written, 0))) {
      result = AESGCMCiphertext{.data = std::move(data)};
    }

    BCryptDestroyKey(key_handle);
  }

  BCryptCloseAlgorithmProvider(algorithm, 0);
  return result;
}

auto aes_gcm_open(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view ciphertext, const std::string_view tag)
    -> std::optional<std::string> {
  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0))) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  BCRYPT_KEY_HANDLE key_handle{nullptr};
  if (BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                                       as_buffer(BCRYPT_CHAIN_MODE_GCM),
                                       sizeof(BCRYPT_CHAIN_MODE_GCM), 0)) &&
      BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(
          algorithm, &key_handle, nullptr, 0, as_buffer(key),
          static_cast<ULONG>(key.size()), 0))) {
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
    BCRYPT_INIT_AUTH_MODE_INFO(info);
    info.pbNonce = as_buffer(iv);
    info.cbNonce = static_cast<ULONG>(iv.size());
    info.pbAuthData = as_buffer(associated_data);
    info.cbAuthData = static_cast<ULONG>(associated_data.size());
    info.pbTag = as_buffer(tag);
    info.cbTag = static_cast<ULONG>(tag.size());

    std::string plaintext(ciphertext.size(), '\x00');
    ULONG written{0};
    // Reports a tag mismatch through the status, so a tampered message fails
    if (BCRYPT_SUCCESS(BCryptDecrypt(key_handle, as_buffer(ciphertext),
                                     static_cast<ULONG>(ciphertext.size()),
                                     &info, nullptr, 0, as_buffer(plaintext),
                                     static_cast<ULONG>(plaintext.size()),
                                     &written, 0))) {
      result = std::move(plaintext);
    }

    BCryptDestroyKey(key_handle);
  }

  BCryptCloseAlgorithmProvider(algorithm, 0);
  return result;
}

} // namespace sourcemeta::core
