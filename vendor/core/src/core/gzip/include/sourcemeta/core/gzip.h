#ifndef SOURCEMETA_CORE_GZIP_H_
#define SOURCEMETA_CORE_GZIP_H_

/// @defgroup gzip GZIP
/// @brief GZIP compression and decompression as per IETF RFC 1952.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
/// ```

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/gzip_error.h>
#include <sourcemeta/core/gzip_streambuf.h>
// NOLINTEND(misc-include-cleaner)

#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <string>  // std::string

namespace sourcemeta::core {

/// @ingroup gzip
/// Compress a byte buffer using the GZIP format (RFC 1952). For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
///
/// const std::string input{"hello world"};
/// const auto compressed{sourcemeta::core::gzip(
///     reinterpret_cast<const std::uint8_t *>(input.data()), input.size())};
/// ```
auto SOURCEMETA_CORE_GZIP_EXPORT gzip(const std::uint8_t *input,
                                      std::size_t size) -> std::string;

/// @ingroup gzip
/// Decompress a GZIP compressed byte buffer (RFC 1952). An optional output
/// size hint can be provided to avoid repeated buffer resizing. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip.h>
///
/// const auto decompressed{sourcemeta::core::gunzip(
///     reinterpret_cast<const std::uint8_t *>(compressed.data()),
///     compressed.size())};
/// ```
auto SOURCEMETA_CORE_GZIP_EXPORT gunzip(const std::uint8_t *input,
                                        std::size_t size,
                                        std::size_t output_hint = 0)
    -> std::string;

} // namespace sourcemeta::core

#endif
