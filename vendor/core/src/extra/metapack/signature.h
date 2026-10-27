#ifndef SOURCEMETA_CORE_METAPACK_SIGNATURE_H_
#define SOURCEMETA_CORE_METAPACK_SIGNATURE_H_

#include <cstdint> // std::uint16_t, std::uint32_t

namespace sourcemeta::core {

// Spells META when read as the four bytes it occupies
inline constexpr std::uint32_t METAPACK_MAGIC{0x4154454D};

inline constexpr std::uint16_t METAPACK_VERSION{1};

} // namespace sourcemeta::core

#endif
