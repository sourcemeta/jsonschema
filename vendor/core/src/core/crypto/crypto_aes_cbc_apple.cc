#include "crypto_aes.h"

#include <CommonCrypto/CommonCryptor.h> // CCCrypt, kCC*

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
// CommonCrypto runs AES in Cipher Block Chaining mode with no padding when no
// option is set, over whole blocks, with the key length selecting the variant
auto apply_cbc(const CCOperation operation, const std::string_view key,
               const std::string_view iv, const std::string_view input)
    -> std::optional<std::string> {
  std::string output(input.size(), '\x00');
  std::size_t moved{0};
  const auto status{CCCrypt(operation, kCCAlgorithmAES, 0, key.data(),
                            key.size(), iv.data(), input.data(), input.size(),
                            output.data(), output.size(), &moved)};
  if (status != kCCSuccess || moved != input.size()) {
    return std::nullopt;
  }

  return output;
}
} // namespace

namespace sourcemeta::core {

auto aes_cbc_encrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view plaintext)
    -> std::optional<std::string> {
  return apply_cbc(kCCEncrypt, key, iv, plaintext);
}

auto aes_cbc_decrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view ciphertext)
    -> std::optional<std::string> {
  return apply_cbc(kCCDecrypt, key, iv, ciphertext);
}

} // namespace sourcemeta::core
