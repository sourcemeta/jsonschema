#ifndef SOURCEMETA_CORE_NUMERIC_PARSE_H
#define SOURCEMETA_CORE_NUMERIC_PARSE_H

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <cstdint>     // std::int64_t, std::uint32_t, std::uint64_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup numeric
/// Attempt to parse a string as a double
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_double(const std::string_view input) noexcept -> std::optional<double>;

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

} // namespace sourcemeta::core

#endif
