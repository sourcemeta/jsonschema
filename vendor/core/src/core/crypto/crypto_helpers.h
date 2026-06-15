#ifndef SOURCEMETA_CORE_CRYPTO_HELPERS_H_
#define SOURCEMETA_CORE_CRYPTO_HELPERS_H_

#include <sourcemeta/core/crypto_sha256.h>
#include <sourcemeta/core/crypto_sha384.h>
#include <sourcemeta/core/crypto_sha512.h>
#include <sourcemeta/core/crypto_verify.h>

#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

// The largest RSA key any backend accepts, so that every backend agrees on
// the range of valid key sizes
inline constexpr std::size_t MAXIMUM_KEY_BYTES{512};

inline auto curve_field_bytes(const EllipticCurve curve) noexcept
    -> std::size_t {
  switch (curve) {
    case EllipticCurve::P256:
      return 32;
    case EllipticCurve::P384:
      return 48;
    case EllipticCurve::P521:
      return 66;
  }

  std::unreachable();
}

inline auto digest_message(const SignatureHashFunction hash,
                           const std::string_view message) -> std::string {
  switch (hash) {
    case SignatureHashFunction::SHA256: {
      const auto digest{sha256_digest(message)};
      return {reinterpret_cast<const char *>(digest.data()), digest.size()};
    }
    case SignatureHashFunction::SHA384: {
      const auto digest{sha384_digest(message)};
      return {reinterpret_cast<const char *>(digest.data()), digest.size()};
    }
    case SignatureHashFunction::SHA512: {
      const auto digest{sha512_digest(message)};
      return {reinterpret_cast<const char *>(digest.data()), digest.size()};
    }
  }

  std::unreachable();
}

inline auto der_append_length(std::string &output, const std::size_t length)
    -> void {
  if (length < 128) {
    output.push_back(static_cast<char>(length));
  } else if (length < 256) {
    output.push_back('\x81');
    output.push_back(static_cast<char>(length));
  } else {
    output.push_back('\x82');
    output.push_back(static_cast<char>((length >> 8u) & 0xffu));
    output.push_back(static_cast<char>(length & 0xffu));
  }
}

inline auto der_append_unsigned_integer(std::string &output,
                                        std::string_view value) -> void {
  while (!value.empty() && value.front() == '\x00') {
    value.remove_prefix(1);
  }

  // A leading zero byte keeps the value positive when its high bit is set,
  // and represents the value zero when nothing remains
  const auto needs_zero_prefix{
      value.empty() ||
      (static_cast<unsigned char>(value.front()) & 0x80u) != 0};
  output.push_back('\x02');
  der_append_length(output, value.size() + (needs_zero_prefix ? 1 : 0));
  if (needs_zero_prefix) {
    output.push_back('\x00');
  }

  output.append(value);
}

} // namespace sourcemeta::core

#endif
