#include <sourcemeta/core/jose_verify.h>

namespace sourcemeta::core {

auto jwt_verify_signature(const JWT &token, const JWK &key) -> bool {
  return jws_verify_signature(token.algorithm(), token.signing_input(),
                              token.signature(), key);
}

} // namespace sourcemeta::core
