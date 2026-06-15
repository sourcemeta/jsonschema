#ifndef SOURCEMETA_CORE_JOSE_JWK_H_
#define SOURCEMETA_CORE_JOSE_JWK_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// A parsed public JSON Web Key (RFC 7517), restricted to RSA and elliptic
/// curve keys. The key owns its decoded material, so the source JSON document
/// does not need to outlive it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(
///     R"({ "kty": "RSA", "n": "0vx7ag", "e": "AQAB" })")};
/// const auto key{sourcemeta::core::JWK::from(document)};
/// assert(key.has_value());
/// assert(key.value().type() == sourcemeta::core::JWK::Type::RSA);
/// ```
class SOURCEMETA_CORE_JOSE_EXPORT JWK {
public:
  enum class Type : std::uint8_t { RSA, EllipticCurve };

  /// Parse a JSON Web Key from a JSON value, throwing on invalid input.
  explicit JWK(const JSON &value);

  /// Parse a JSON Web Key from a JSON value, throwing on invalid input.
  explicit JWK(JSON &&value);

  /// Parse a JSON Web Key from a JSON value, returning no value on invalid
  /// input.
  [[nodiscard]] static auto from(const JSON &value) -> std::optional<JWK>;

  /// Parse a JSON Web Key from a JSON value, returning no value on invalid
  /// input.
  [[nodiscard]] static auto from(JSON &&value) -> std::optional<JWK>;

  [[nodiscard]] auto type() const noexcept -> Type { return this->type_; }

  [[nodiscard]] auto key_id() const noexcept
      -> std::optional<std::string_view> {
    if (this->key_id_.has_value()) {
      return std::string_view{this->key_id_.value()};
    }

    return std::nullopt;
  }

  [[nodiscard]] auto algorithm() const noexcept -> std::optional<JWSAlgorithm> {
    return this->algorithm_;
  }

  // RSA keys (RFC 7518 Section 6.3): big-endian modulus and exponent
  [[nodiscard]] auto modulus() const noexcept -> std::string_view {
    return this->modulus_;
  }

  [[nodiscard]] auto exponent() const noexcept -> std::string_view {
    return this->exponent_;
  }

  // Elliptic curve keys (RFC 7518 Section 6.2): curve name and coordinates
  [[nodiscard]] auto curve() const noexcept -> std::string_view {
    return this->curve_;
  }

  [[nodiscard]] auto coordinate_x() const noexcept -> std::string_view {
    return this->coordinate_x_;
  }

  [[nodiscard]] auto coordinate_y() const noexcept -> std::string_view {
    return this->coordinate_y_;
  }

private:
  JWK() = default;
  static auto parse(const JSON &value, JWK &result) -> bool;

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  Type type_{Type::RSA};
  std::optional<std::string> key_id_;
  std::optional<JWSAlgorithm> algorithm_;
  std::string modulus_;
  std::string exponent_;
  std::string curve_;
  std::string coordinate_x_;
  std::string coordinate_y_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
