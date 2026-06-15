#ifndef SOURCEMETA_CORE_CRYPTO_RANDOM_H_
#define SOURCEMETA_CORE_CRYPTO_RANDOM_H_

#include <array> // std::array

namespace sourcemeta::core {

// Fill the given buffer with random bytes from the system provider where
// available. Defined once per backend
auto fill_random_bytes(std::array<unsigned char, 16> &bytes) -> void;

} // namespace sourcemeta::core

#endif
