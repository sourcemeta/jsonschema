#include <sourcemeta/core/jose_jwks.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {

const auto HASH_KEYS{sourcemeta::core::JSON::Object::hash("keys")};

} // namespace

namespace sourcemeta::core {

auto JWKS::parse(const JSON &value, JWKS &result) -> bool {
  if (!value.is_object()) {
    return false;
  }

  const auto *keys{value.try_at("keys", HASH_KEYS)};
  if (keys == nullptr || !keys->is_array()) {
    return false;
  }

  // Individual keys that fail to parse are skipped so that one exotic key
  // cannot break verification of tokens signed by the others (RFC 7517
  // Section 5)
  for (const auto &entry : keys->as_array()) {
    auto key{JWK::from(entry)};
    if (key.has_value()) {
      result.keys_.push_back(std::move(key).value());
    }
  }

  // An empty set, or one whose keys all failed to parse, is not usable
  return !result.keys_.empty();
}

JWKS::JWKS(const JSON &value) {
  if (!parse(value, *this)) {
    throw JWKSParseError{};
  }
}

// The keys are decoded into fresh storage, so there is nothing to move out of
// the source value. The rvalue overloads exist for call-site symmetry and
// delegate to the lvalue path
JWKS::JWKS(JSON &&value) : JWKS{value} {}

auto JWKS::from(const JSON &value) -> std::optional<JWKS> {
  JWKS result;
  if (parse(value, result)) {
    return result;
  }

  return std::nullopt;
}

auto JWKS::from(JSON &&value) -> std::optional<JWKS> { return from(value); }

auto JWKS::find(const std::string_view key_id) const noexcept -> const JWK * {
  for (const auto &key : this->keys_) {
    const auto candidate{key.key_id()};
    if (candidate.has_value() && candidate.value() == key_id) {
      return &key;
    }
  }

  return nullptr;
}

} // namespace sourcemeta::core
