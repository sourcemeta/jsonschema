#ifndef SOURCEMETA_JSONBINPACK_NUMERIC_H_
#define SOURCEMETA_JSONBINPACK_NUMERIC_H_

#include <cstdint> // std::uint8_t, std::int64_t
#include <limits>  // std::numeric_limits

/// @defgroup numeric Numeric
/// @brief A comprehensive numeric library for JSON BinPack
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/jsonbinpack/numeric.h>
/// ```

namespace sourcemeta::jsonbinpack {

/// @ingroup numeric
template <typename T> constexpr auto is_byte(const T value) noexcept -> bool {
  return value <= std::numeric_limits<std::uint8_t>::max();
}

/// @ingroup numeric
constexpr auto count_multiples(const std::int64_t minimum,
                               const std::int64_t maximum,
                               const std::int64_t multiplier) -> std::uint64_t {
  assert(minimum <= maximum);
  assert(multiplier > 0);
  return static_cast<std::uint64_t>((maximum / multiplier) -
                                    ((minimum - 1) / multiplier));
}

} // namespace sourcemeta::jsonbinpack

#endif
