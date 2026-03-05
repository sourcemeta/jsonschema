#ifndef SOURCEMETA_CORE_IO_H_
#define SOURCEMETA_CORE_IO_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/io_error.h>
#include <sourcemeta/core/io_fileview.h>
#include <sourcemeta/core/io_temporary.h>
// NOLINTEND(misc-include-cleaner)

#include <cassert>    // assert
#include <filesystem> // std::filesystem
#include <fstream>    // std::basic_ifstream
#include <string>     // std::char_traits

/// @defgroup io I/O
/// @brief A growing collection of I/O utilities
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// ```

namespace sourcemeta::core {

/// @ingroup io
///
/// A safe variant of `std::filesystem::canonical` that takes into account
/// platform-specific oddities like FIFO on GNU/Linux. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// const auto output{sourcemeta::core::canonical("/tmp/../foo.json")};
/// assert(output == "/foo.json");
/// ```
SOURCEMETA_CORE_IO_EXPORT
auto canonical(const std::filesystem::path &path) -> std::filesystem::path;

/// @ingroup io
///
/// A safe variant of `std::filesystem::weakly_canonical` that takes into
/// account platform-specific oddities like FIFO on GNU/Linux. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// const auto output{sourcemeta::core::weakly_canonical("/tmp/../foo.json")};
/// assert(output == "/foo.json");
/// ```
SOURCEMETA_CORE_IO_EXPORT
auto weakly_canonical(const std::filesystem::path &path)
    -> std::filesystem::path;

/// @ingroup io
///
/// Check if a file path starts with another path. This function assumes the
/// paths are canonicalised. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::starts_with("/foo/bar", "/foo"));
/// ```
SOURCEMETA_CORE_IO_EXPORT
auto starts_with(const std::filesystem::path &path,
                 const std::filesystem::path &prefix) -> bool;

/// @ingroup io
///
/// A convenience function to open a stream from a file. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// auto stream{sourcemeta::core::read_file("/tmp/foo.json")};
/// assert(stream.is_open());
/// ```
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
auto read_file(const std::filesystem::path &path)
    -> std::basic_ifstream<CharT, Traits> {
  if (std::filesystem::is_directory(path)) {
    throw std::filesystem::filesystem_error(
        "Cannot open a directory as a file", path,
        std::make_error_code(std::errc::is_a_directory));
  }

  std::ifstream stream{sourcemeta::core::canonical(path)};
  stream.exceptions(std::ifstream::badbit);
  assert(!stream.fail());
  assert(stream.is_open());
  return stream;
}

/// @ingroup io
///
/// Recursively mirror a directory tree using hard links for regular files.
/// Directories are created, regular files are hard-linked. Both paths must
/// reside on the same filesystem. The destination must not be inside the
/// source tree, as that would cause infinite recursion.
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
///
/// sourcemeta::core::hardlink_directory("/source", "/destination");
/// ```
SOURCEMETA_CORE_IO_EXPORT
auto hardlink_directory(const std::filesystem::path &source,
                        const std::filesystem::path &destination) -> void;

/// @ingroup io
///
/// Atomically swap two directories. Both directories must reside on the same
/// filesystem and the original path must not be a bare filename (it must have
/// a parent component). After the call, the original path holds the contents
/// of the replacement and the replacement path holds the former contents of
/// the original. If the original does not exist, the replacement is simply
/// renamed into place and the replacement path will no longer exist.
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
///
/// sourcemeta::core::atomic_directory_swap("/output", "/staging");
/// ```
SOURCEMETA_CORE_IO_EXPORT
auto atomic_directory_swap(const std::filesystem::path &original,
                           const std::filesystem::path &replacement) -> void;

/// @ingroup io
///
/// Flush an existing file to disk, beyond just to the operating system. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
///
/// sourcemeta::core::flush("/foo/bar.txt");
/// ```
SOURCEMETA_CORE_IO_EXPORT
auto flush(const std::filesystem::path &path) -> void;

} // namespace sourcemeta::core

#endif
