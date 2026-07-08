#include <sourcemeta/core/jose_sign.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include <optional>    // std::optional, std::nullopt
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
using namespace std::string_view_literals;

const auto HASH_ALG{sourcemeta::core::JSON::Object::hash("alg"sv)};
const auto HASH_CRIT{sourcemeta::core::JSON::Object::hash("crit"sv)};

auto compact(const sourcemeta::core::JSON &document) -> std::string {
  std::ostringstream stream;
  sourcemeta::core::stringify(document, stream);
  return stream.str();
}

} // namespace

namespace sourcemeta::core {

auto jwt_sign(const JSON &header, const JSON &payload, const JWKPrivate &key)
    -> std::optional<std::string> {
  if (!header.is_object() || !payload.is_object()) {
    return std::nullopt;
  }

  // Critical header extensions are not understood, so a token carrying them is
  // not emitted rather than producing one the parser would reject (RFC 7515
  // Section 4.1.11)
  if (header.try_at("crit", HASH_CRIT) != nullptr) {
    return std::nullopt;
  }

  // The algorithm is taken from the header (RFC 7515 Section 4.1.1)
  const auto *algorithm{header.try_at("alg", HASH_ALG)};
  if (algorithm == nullptr || !algorithm->is_string()) {
    return std::nullopt;
  }

  const auto parsed{to_jws_algorithm(algorithm->to_string())};
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  // The signing input is the base64url header and payload joined by a dot (RFC
  // 7515 Section 5.1)
  std::string signing_input{base64url_encode(compact(header))};
  signing_input.push_back('.');
  signing_input.append(base64url_encode(compact(payload)));

  const auto signature{jws_sign(parsed.value(), signing_input, key)};
  if (!signature.has_value()) {
    return std::nullopt;
  }

  signing_input.push_back('.');
  signing_input.append(base64url_encode(signature.value()));
  return signing_input;
}

} // namespace sourcemeta::core
