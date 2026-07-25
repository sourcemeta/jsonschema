#ifndef SOURCEMETA_CORE_CRYPTO_SHA1_OTHER_H_
#define SOURCEMETA_CORE_CRYPTO_SHA1_OTHER_H_

// The raw SHA-1 digest (RFC 3174) for the fallback backend, used by the modes
// that need the digest bytes rather than the hexadecimal string

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto sha1_digest(const std::string_view input) -> std::array<std::uint8_t, 20>;

} // namespace sourcemeta::core

#endif
