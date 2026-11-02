#include <sourcemeta/core/crypto_ecdh.h>

#include "crypto_openssl.h"

#include <openssl/evp.h> // EVP_*

#include <cstddef>  // std::size_t
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string
#include <utility>  // std::move

namespace sourcemeta::core {

auto ecdh_derive(const PrivateKey &private_key, const PublicKey &public_key)
    -> std::optional<std::string> {
  const auto *const private_internal{private_key.internal()};
  const auto *const public_internal{public_key.internal()};
  if (private_internal == nullptr || public_internal == nullptr ||
      private_internal->kind != PrivateKey::Type::EllipticCurve ||
      public_internal->kind != PublicKey::Type::EllipticCurve ||
      private_internal->field_bytes != public_internal->field_bytes) {
    return std::nullopt;
  }

  auto *context{EVP_PKEY_CTX_new(private_internal->key, nullptr)};
  if (context == nullptr) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  std::size_t length{0};
  // The peer key was validated on the curve when it was parsed, and setting it
  // as the derivation peer checks it again
  if (EVP_PKEY_derive_init(context) == 1 &&
      EVP_PKEY_derive_set_peer(context, public_internal->key) == 1 &&
      EVP_PKEY_derive(context, nullptr, &length) == 1) {
    std::string secret(length, '\x00');
    if (EVP_PKEY_derive(context,
                        reinterpret_cast<unsigned char *>(secret.data()),
                        &length) == 1 &&
        length <= private_internal->field_bytes) {
      secret.resize(length);
      // Guarantee the fixed-length x coordinate the contract promises, since a
      // leading zero could otherwise shorten the platform output
      secret.insert(secret.begin(),
                    private_internal->field_bytes - secret.size(), '\x00');
      result = std::move(secret);
    }
  }

  EVP_PKEY_CTX_free(context);
  return result;
}

} // namespace sourcemeta::core
