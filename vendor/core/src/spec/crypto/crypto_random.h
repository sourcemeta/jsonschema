#ifndef SOURCEMETA_CORE_CRYPTO_RANDOM_H_
#define SOURCEMETA_CORE_CRYPTO_RANDOM_H_

#include <cstdint> // std::uint8_t
#include <span>    // std::span

namespace sourcemeta::core {

// Fill the given buffer with random bytes from the system provider where
// available. Defined once per backend
auto fill_random_bytes(std::span<std::uint8_t> bytes) -> void;

} // namespace sourcemeta::core

#endif
