#ifndef SOURCEMETA_CORE_JOSE_JWE_H_
#define SOURCEMETA_CORE_JOSE_JWE_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// A parsed JSON Web Encryption object in compact serialization (RFC 7516). It
/// owns its decoded segments, so the string it was parsed from need not outlive
/// it. The plaintext is recovered by passing it to `jwe_decrypt`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// const auto object{sourcemeta::core::JWE::from(compact)};
/// assert(object.has_value());
/// assert(object.value().encryption().has_value());
/// ```
class SOURCEMETA_CORE_JOSE_EXPORT JWE {
public:
  /// Parse a JSON Web Encryption object from its compact serialization,
  /// throwing a `JWEParseError` on invalid input.
  explicit JWE(const std::string_view input);

  /// Parse a JSON Web Encryption object from its compact serialization,
  /// returning no value on invalid input.
  [[nodiscard]] static auto from(const std::string_view input)
      -> std::optional<JWE>;

  // Protected header (RFC 7516 Section 4)

  /// The key-management algorithm declared in the protected header, if it names
  /// a supported one.
  [[nodiscard]] auto algorithm() const noexcept -> std::optional<JWEAlgorithm> {
    return this->algorithm_;
  }

  /// The content encryption algorithm declared in the protected header, if it
  /// names a supported one.
  [[nodiscard]] auto encryption() const noexcept
      -> std::optional<JWEEncryption> {
    return this->encryption_;
  }

  /// The key identifier from the protected header, if present.
  [[nodiscard]] auto key_id() const noexcept -> std::optional<std::string_view>;

  /// The decoded protected header.
  [[nodiscard]] auto header() const noexcept -> const JSON & {
    return this->header_;
  }

  // Wire segments (RFC 7516 Section 7.1), consumed by jwe_decrypt

  /// The exact base64url-encoded protected header, the Additional Authenticated
  /// Data the content encryption is bound to (RFC 7516 Section 5.1).
  [[nodiscard]] auto protected_header() const noexcept -> std::string_view {
    return this->protected_header_;
  }

  /// The decoded JWE Encrypted Key, empty for direct key agreement and direct
  /// encryption.
  [[nodiscard]] auto encrypted_key() const noexcept -> std::string_view {
    return this->encrypted_key_;
  }

  /// The decoded JWE Initialization Vector.
  [[nodiscard]] auto initialization_vector() const noexcept
      -> std::string_view {
    return this->initialization_vector_;
  }

  /// The decoded JWE Ciphertext.
  [[nodiscard]] auto ciphertext() const noexcept -> std::string_view {
    return this->ciphertext_;
  }

  /// The decoded JWE Authentication Tag.
  [[nodiscard]] auto tag() const noexcept -> std::string_view {
    return this->tag_;
  }

private:
  JWE() = default;
  static auto parse(const std::string_view input, JWE &result) -> bool;

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::string protected_header_;
  std::string encrypted_key_;
  std::string initialization_vector_;
  std::string ciphertext_;
  std::string tag_;
  JSON header_{nullptr};
  std::optional<JWEAlgorithm> algorithm_;
  std::optional<JWEEncryption> encryption_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
