#ifndef SOURCEMETA_CORE_OIDC_VERIFY_H_
#define SOURCEMETA_CORE_OIDC_VERIFY_H_

#include <sourcemeta/core/jose.h>

#include <algorithm> // std::ranges::find
#include <span>      // std::span

namespace sourcemeta::core {

// The signature-verification step shared by the OpenID Connect token paths (the
// UserInfo response, the request object, and the logout token). The algorithm
// is pinned to the allow-list and never taken from the header alone, defeating
// algorithm confusion, and a present key identifier selects exactly one key,
// matching jwt_verify so key rotation and revocation stay consistent across the
// paths rather than each maintaining its own selection
inline auto oidc_verify_selected_signature(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms) -> bool {
  const auto algorithm{token.algorithm()};
  if (!algorithm.has_value() ||
      std::ranges::find(allowed_algorithms, algorithm.value()) ==
          allowed_algorithms.end()) {
    return false;
  }

  const auto key_id{token.key_id()};
  if (key_id.has_value()) {
    const auto *key{keys.find(key_id.value())};
    return key != nullptr && jwt_verify_signature(token, *key);
  }

  for (const auto &key : keys) {
    if (jwt_verify_signature(token, key)) {
      return true;
    }
  }

  return false;
}

} // namespace sourcemeta::core

#endif
