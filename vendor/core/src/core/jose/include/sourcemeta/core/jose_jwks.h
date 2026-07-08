#ifndef SOURCEMETA_CORE_JOSE_JWKS_H_
#define SOURCEMETA_CORE_JOSE_JWKS_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jose_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/jose_jwk.h>
#include <sourcemeta/core/json.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

/// @ingroup jose
/// A parsed JSON Web Key Set (RFC 7517 Section 5). Keys that individually fail
/// to parse, such as those of an unsupported type, are skipped rather than
/// failing the whole set, so one exotic key cannot break verification of
/// tokens signed by the others. The set owns its keys. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(
///     R"({ "keys": [ { "kty": "RSA", "n": "0vx7ag", "e": "AQAB",
///                      "kid": "2024" } ] })")};
/// const auto keys{sourcemeta::core::JWKS::from(document)};
/// assert(keys.has_value());
/// assert(keys.value().find("2024") != nullptr);
/// ```
class SOURCEMETA_CORE_JOSE_EXPORT JWKS {
public:
  /// Parse a JSON Web Key Set from a JSON value, throwing a `JWKSParseError`
  /// on invalid input.
  explicit JWKS(const JSON &value);
  /// Parse a JSON Web Key Set from a JSON value, throwing a `JWKSParseError`
  /// on invalid input.
  explicit JWKS(JSON &&value);

  /// A key set exclusively owns its keys, so it is move-only.
  JWKS(JWKS &&other) noexcept = default;
  auto operator=(JWKS &&other) noexcept -> JWKS & = default;
  JWKS(const JWKS &) = delete;
  auto operator=(const JWKS &) -> JWKS & = delete;

  /// Parse a JSON Web Key Set from a JSON value, returning no value on invalid
  /// input.
  [[nodiscard]] static auto from(const JSON &value) -> std::optional<JWKS>;
  /// Parse a JSON Web Key Set from a JSON value, returning no value on invalid
  /// input.
  [[nodiscard]] static auto from(JSON &&value) -> std::optional<JWKS>;

  /// Look up a key by its identifier (RFC 7515 Section 4.1.4), returning no
  /// pointer when no key in the set carries it.
  [[nodiscard]] auto find(const std::string_view key_id) const noexcept
      -> const JWK *;

  /// The number of keys in the set.
  [[nodiscard]] auto size() const noexcept -> std::size_t {
    return this->keys_.size();
  }

  /// Whether the set holds no keys.
  [[nodiscard]] auto empty() const noexcept -> bool {
    return this->keys_.empty();
  }

  [[nodiscard]] auto begin() const noexcept { return this->keys_.begin(); }
  [[nodiscard]] auto end() const noexcept { return this->keys_.end(); }

private:
  JWKS() = default;
  static auto parse(const JSON &value, JWKS &result) -> bool;

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::vector<JWK> keys_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
