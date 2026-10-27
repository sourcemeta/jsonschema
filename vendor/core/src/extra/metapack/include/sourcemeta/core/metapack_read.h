#ifndef SOURCEMETA_CORE_METAPACK_READ_H_
#define SOURCEMETA_CORE_METAPACK_READ_H_

#ifndef SOURCEMETA_CORE_METAPACK_EXPORT
#include <sourcemeta/core/metapack_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/metapack_format.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>

#include <array>       // std::array
#include <chrono>      // std::chrono::system_clock
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <filesystem>  // std::filesystem::path
#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view
#include <type_traits> // std::is_trivially_copyable_v

namespace sourcemeta::core {

/// @ingroup metapack
/// How far stored contents may inflate before a reader refuses them, so that a
/// small file cannot ask for an unbounded amount of memory
inline constexpr std::uint64_t METAPACK_MAXIMUM_DECOMPRESSION_RATIO{1024};

/// @ingroup metapack
/// Every part of a file, located in a single pass. The views point into the
/// memory-mapped file, so they last exactly as long as it stays mapped
struct MetapackView {
  /// The metadata the file begins with
  const MetapackHeader *header{nullptr};
  /// The media type to serve the contents as
  std::string_view media_type;
  /// The extension area, as the bytes the writer put there
  std::span<const std::uint8_t> extension;
  /// The contents, as stored
  std::span<const std::uint8_t> payload;
};

/// @ingroup metapack
/// Locate every part of a memory-mapped file. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// const sourcemeta::core::FileView view{"/tmp/document.metapack"};
/// const auto parsed{sourcemeta::core::metapack_parse(view)};
/// assert(parsed.has_value());
/// assert(parsed.value().media_type == "application/json");
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_parse(const FileView &view) -> std::optional<MetapackView>;

/// @ingroup metapack
/// Read the contents of a file at the given path as a JSON document. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <cassert>
///
/// const auto document{
///     sourcemeta::core::metapack_read_json("/tmp/document.metapack")};
/// assert(document.has_value());
/// assert(document.value().is_object());
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_read_json(const std::filesystem::path &path,
                        const std::uint64_t maximum_decompression_ratio =
                            METAPACK_MAXIMUM_DECOMPRESSION_RATIO)
    -> std::optional<JSON>;

/// @ingroup metapack
/// Read the contents of a memory-mapped file as a JSON document. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// const sourcemeta::core::FileView view{"/tmp/document.metapack"};
/// const auto document{sourcemeta::core::metapack_read_json(view)};
/// assert(document.has_value());
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_read_json(const FileView &view,
                        const std::uint64_t maximum_decompression_ratio =
                            METAPACK_MAXIMUM_DECOMPRESSION_RATIO)
    -> std::optional<JSON>;

/// @ingroup metapack
/// Read the contents of an already located file as a JSON document. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// const sourcemeta::core::FileView view{"/tmp/document.metapack"};
/// const auto parsed{sourcemeta::core::metapack_parse(view).value()};
/// const auto document{sourcemeta::core::metapack_read_json(parsed)};
/// assert(document.has_value());
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_read_json(const MetapackView &parsed,
                        const std::uint64_t maximum_decompression_ratio =
                            METAPACK_MAXIMUM_DECOMPRESSION_RATIO)
    -> std::optional<JSON>;

/// @ingroup metapack
/// Render the checksum a header records as lowercase hexadecimal. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/io.h>
/// #include <iostream>
///
/// const sourcemeta::core::FileView view{"/tmp/document.metapack"};
/// const auto parsed{sourcemeta::core::metapack_parse(view).value()};
/// const auto checksum{sourcemeta::core::metapack_checksum_hex(
///     *parsed.header)};
/// std::cout.write(checksum.data(), checksum.size());
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_checksum_hex(const MetapackHeader &header)
    -> std::array<char, METAPACK_CHECKSUM_HEX_LENGTH>;

/// @ingroup metapack
/// The point in time a header records. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
/// #include <chrono>
///
/// const sourcemeta::core::FileView view{"/tmp/document.metapack"};
/// const auto parsed{sourcemeta::core::metapack_parse(view).value()};
/// assert(sourcemeta::core::metapack_last_modified(*parsed.header) <=
///        std::chrono::system_clock::now());
/// ```
SOURCEMETA_CORE_METAPACK_EXPORT
auto metapack_last_modified(const MetapackHeader &header)
    -> std::chrono::system_clock::time_point;

/// @ingroup metapack
/// What an extension area holds, as the layout the writer of the file put
/// there, or nothing when it is too small to hold one. For example:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// #pragma pack(push, 1)
/// struct Revision {
///   std::uint32_t number;
/// };
/// #pragma pack(pop)
///
/// const sourcemeta::core::FileView view{"/tmp/document.metapack"};
/// const auto parsed{sourcemeta::core::metapack_parse(view).value()};
/// const auto *revision{
///     sourcemeta::core::metapack_extension_as<Revision>(parsed.extension)};
/// assert(revision != nullptr);
/// ```
template <typename T>
auto metapack_extension_as(const std::span<const std::uint8_t> extension)
    -> const T * {
  static_assert(std::is_trivially_copyable_v<T>,
                "An extension layout is read where it lies, so it has to be "
                "trivially copyable");
  static_assert(alignof(T) == 1,
                "An extension begins at an offset the media type decides, so "
                "its layout must be packed to be read where it lies");
  if (extension.size() < sizeof(T)) {
    return nullptr;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const T *>(extension.data());
}

} // namespace sourcemeta::core

#endif
