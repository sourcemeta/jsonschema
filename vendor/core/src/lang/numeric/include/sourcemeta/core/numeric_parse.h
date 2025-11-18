#ifndef SOURCEMETA_CORE_NUMERIC_PARSE_H
#define SOURCEMETA_CORE_NUMERIC_PARSE_H

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <cstdint>  // std::int64_t
#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::core {

/// @ingroup numeric
/// Attempt to parse a string as a double
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_double(const std::string &input) noexcept -> std::optional<double>;

/// @ingroup numeric
/// Attempt to parse a string as a signed 64-bit integer
SOURCEMETA_CORE_NUMERIC_EXPORT
auto to_int64_t(const std::string &input) noexcept
    -> std::optional<std::int64_t>;

} // namespace sourcemeta::core

#endif
