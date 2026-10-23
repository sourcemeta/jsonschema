#ifndef SOURCEMETA_CORE_NUMERIC_PARSE_H
#define SOURCEMETA_CORE_NUMERIC_PARSE_H

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <cstdint>  // std::int64_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <optional> // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup numeric
/// Attempt to parse a string as a double
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_double(const std::string_view input) noexcept -> std::optional<double>;

/// @ingroup numeric
/// Attempt to parse a fixed point decimal string as one of the values of the
/// double precision interchange format of IEEE 754-2019, reporting no value
/// unless the string denotes one of them exactly. A number the format does not
/// hold, a string carrying an exponent, and a run of digits wider than a
/// 64-bit accumulator each report no value, which leaves them to an arbitrary
/// precision representation. For example:
///
/// ```cpp
/// #include <sourcemeta/core/numeric.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::to_double_exact("0.5").value() == 0.5);
/// assert(!sourcemeta::core::to_double_exact("0.1").has_value());
/// ```
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_double_exact(const std::string_view input) noexcept
    -> std::optional<double>;

/// @ingroup numeric
/// Attempt to parse a string as a signed 64-bit integer
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_int64_t(const std::string_view input) noexcept
    -> std::optional<std::int64_t>;

/// @ingroup numeric
/// Attempt to parse a string as a signed 64-bit integer in a given base
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_int64_t(const std::string_view input, const int base) noexcept
    -> std::optional<std::int64_t>;

/// @ingroup numeric
/// Attempt to parse a string as an unsigned 64-bit decimal integer.
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_uint64_t(const std::string_view input) noexcept
    -> std::optional<std::uint64_t>;

/// @ingroup numeric
/// Attempt to parse a string as an unsigned 32-bit decimal integer
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_uint32_t(const std::string_view input) noexcept
    -> std::optional<std::uint32_t>;

/// @ingroup numeric
/// Attempt to parse a string as an unsigned 32-bit integer in a given base
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_uint32_t(const std::string_view input, const int base) noexcept
    -> std::optional<std::uint32_t>;

/// @ingroup numeric
/// Attempt to parse a string as an unsigned 16-bit decimal integer
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_uint16_t(const std::string_view input) noexcept
    -> std::optional<std::uint16_t>;

} // namespace sourcemeta::core

#endif
