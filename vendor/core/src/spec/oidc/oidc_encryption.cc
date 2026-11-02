#include <sourcemeta/core/oidc_encryption.h>

#include <sourcemeta/core/jose.h>

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto oidc_decrypt_nested_jwt(const std::string_view compact,
                             const JWKPrivate &key)
    -> std::optional<std::string> {
  // OpenID Connect Core 1.0 Section 10.2: an encrypted token is a signed JWT
  // wrapped in a JWE, so decryption yields the inner compact JWS
  const auto object{JWE::from(compact)};
  if (!object.has_value()) {
    return std::nullopt;
  }

  return jwe_decrypt(object.value(), key);
}

} // namespace sourcemeta::core
