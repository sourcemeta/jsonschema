#ifndef SOURCEMETA_CORE_JOSE_JWT_H_
#define SOURCEMETA_CORE_JOSE_JWT_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono::system_clock::time_point
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// A parsed JSON Web Token in compact serialization (RFC 7519, RFC 7515). The
/// token does not own its input, so the string it was parsed from must outlive
/// it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
/// #include <string>
///
/// const std::string input{
///     "eyJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY21lIn0.c2ln"};
/// const auto token{sourcemeta::core::JWT::from(input)};
/// assert(token.has_value());
/// assert(token.value().algorithm() == sourcemeta::core::JWSAlgorithm::RS256);
/// ```
class SOURCEMETA_CORE_JOSE_EXPORT JWT {
public:
  /// Parse a JSON Web Token from its compact serialization, throwing a
  /// `JWTParseError` on invalid input.
  explicit JWT(const std::string_view input);

  /// Parse a JSON Web Token from its compact serialization, returning no value
  /// on invalid input.
  [[nodiscard]] static auto from(const std::string_view input)
      -> std::optional<JWT>;

  // Header (RFC 7515 Section 4)

  [[nodiscard]] auto algorithm() const noexcept -> std::optional<JWSAlgorithm> {
    return this->algorithm_;
  }

  [[nodiscard]] auto key_id() const noexcept -> std::optional<std::string_view>;

  [[nodiscard]] auto type() const noexcept -> std::optional<std::string_view>;

  [[nodiscard]] auto header() const noexcept -> const JSON & {
    return this->header_;
  }

  // Registered claims (RFC 7519 Section 4.1)

  [[nodiscard]] auto issuer() const noexcept -> std::optional<std::string_view>;

  [[nodiscard]] auto subject() const noexcept
      -> std::optional<std::string_view>;

  [[nodiscard]] auto
  has_audience(const std::string_view audience) const noexcept -> bool;

  [[nodiscard]] auto expires_at() const
      -> std::optional<std::chrono::system_clock::time_point>;

  [[nodiscard]] auto not_before() const
      -> std::optional<std::chrono::system_clock::time_point>;

  [[nodiscard]] auto issued_at() const
      -> std::optional<std::chrono::system_clock::time_point>;

  [[nodiscard]] auto token_id() const noexcept
      -> std::optional<std::string_view>;

  [[nodiscard]] auto payload() const noexcept -> const JSON & {
    return this->payload_;
  }

  // The exact wire bytes the signature is computed over, never re-serialized
  // (RFC 7515 Section 5.1)
  [[nodiscard]] auto signing_input() const noexcept -> std::string_view {
    return this->signing_input_;
  }

  [[nodiscard]] auto signature() const noexcept -> std::string_view {
    return this->signature_;
  }

private:
  JWT() = default;
  static auto parse(const std::string_view input, JWT &result) -> bool;

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::string_view signing_input_;
  std::string signature_;
  JSON header_{nullptr};
  JSON payload_{nullptr};
  std::optional<JWSAlgorithm> algorithm_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
