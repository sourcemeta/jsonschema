#ifndef SOURCEMETA_CORE_METAPACK_WRITE_H_
#define SOURCEMETA_CORE_METAPACK_WRITE_H_

#ifndef SOURCEMETA_CORE_METAPACK_EXPORT
#include <sourcemeta/core/metapack_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/metapack_error.h>
#include <sourcemeta/core/metapack_format.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>

#include <cstdint>     // std::uint8_t
#include <filesystem>  // std::filesystem::path
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup metapack
/// Write a JSON document as the contents of a file. A media type or
/// extension too large for the format to record throws. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/json.h>
///
/// const auto document{sourcemeta::core::parse_json("{ \"foo\": 1 }")};
/// sourcemeta::core::metapack_write_json(
///     "/tmp/document.metapack", document, "application/json",
///     sourcemeta::core::MetapackEncoding::GZIP);
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_write_json(const std::filesystem::path &destination,
                         const JSON &document,
                         const std::string_view media_type,
                         const MetapackEncoding encoding,
                         const std::span<const std::uint8_t> extension = {})
    -> void;

/// @ingroup metapack
/// Write a JSON document as the contents of a file, indented for a human to
/// read. A media type or extension too large for the format to record
/// throws. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/json.h>
///
/// const auto document{sourcemeta::core::parse_json("{ \"foo\": 1 }")};
/// sourcemeta::core::metapack_write_json_pretty(
///     "/tmp/document.metapack", document, "application/json",
///     sourcemeta::core::MetapackEncoding::Identity);
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_write_json_pretty(
    const std::filesystem::path &destination, const JSON &document,
    const std::string_view media_type, const MetapackEncoding encoding,
    const std::span<const std::uint8_t> extension = {}) -> void;

/// @ingroup metapack
/// Write text as the contents of a file, terminated by a line feed. A media
/// type or extension too large for the format to record throws. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
///
/// sourcemeta::core::metapack_write_text(
///     "/tmp/greeting.metapack", "Hello World", "text/plain",
///     sourcemeta::core::MetapackEncoding::Identity);
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_write_text(const std::filesystem::path &destination,
                         const std::string_view contents,
                         const std::string_view media_type,
                         const MetapackEncoding encoding,
                         const std::span<const std::uint8_t> extension = {})
    -> void;

/// @ingroup metapack
/// Write bytes as the contents of a file. A media type or extension too
/// large for the format to record throws. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <array>
///
/// constexpr std::array<std::uint8_t, 3> bytes{{0x41, 0x42, 0x43}};
/// sourcemeta::core::metapack_write_bytes(
///     "/tmp/blob.metapack", bytes, "application/octet-stream",
///     sourcemeta::core::MetapackEncoding::Identity);
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_write_bytes(const std::filesystem::path &destination,
                          const std::span<const std::uint8_t> contents,
                          const std::string_view media_type,
                          const MetapackEncoding encoding,
                          const std::span<const std::uint8_t> extension = {})
    -> void;

/// @ingroup metapack
/// Write another file as the contents of a file. A media type or extension
/// too large for the format to record throws. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
///
/// sourcemeta::core::metapack_write_file(
///     "/tmp/page.metapack", "/tmp/page.html", "text/html",
///     sourcemeta::core::MetapackEncoding::GZIP);
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_write_file(const std::filesystem::path &destination,
                         const std::filesystem::path &source,
                         const std::string_view media_type,
                         const MetapackEncoding encoding,
                         const std::span<const std::uint8_t> extension = {})
    -> void;

} // namespace sourcemeta::core

#endif
