#ifndef SOURCEMETA_CORE_GZIP_H_
#define SOURCEMETA_CORE_GZIP_H_

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup gzip GZIP
/// @brief A growing implementation of RFC 1952 GZIP.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// ```

namespace sourcemeta::core {

/// @ingroup gzip
///
/// Compress an input string into a sequence of bytes represented using a
/// string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::gzip("Hello World")};
/// assert(result.has_value());
/// assert(!result.value().empty());
/// ```
SOURCEMETA_CORE_GZIP_EXPORT auto gzip(std::string_view input)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
