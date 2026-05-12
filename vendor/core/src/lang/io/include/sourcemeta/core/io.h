#ifndef SOURCEMETA_CORE_IO_H_
#define SOURCEMETA_CORE_IO_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/io_atomic.h>
#include <sourcemeta/core/io_binary.h>
#include <sourcemeta/core/io_error.h>
#include <sourcemeta/core/io_fileview.h>
#include <sourcemeta/core/io_temporary.h>
// NOLINTEND(misc-include-cleaner)

#include <filesystem>   // std::filesystem
#include <fstream>      // std::basic_ifstream
#include <iostream>     // std::cin
#include <istream>      // std::basic_istream
#include <sstream>      // std::basic_ostringstream
#include <string>       // std::basic_string, std::char_traits, std::string
#include <system_error> // std::error_code

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
    throw IOIsADirectoryError{path};
  }

  const auto canonical_path{sourcemeta::core::canonical(path)};
  std::ifstream stream{canonical_path};
  if (!stream.is_open()) {
    throw IOFilePermissionError{canonical_path};
  }

  stream.exceptions(std::ifstream::badbit);
  return stream;
}

/// @ingroup io
///
/// Drain an input stream into a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::istringstream stream{"hello"};
/// const auto contents{sourcemeta::core::read_to_string(stream)};
/// assert(contents == "hello");
/// ```
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
auto read_to_string(std::basic_istream<CharT, Traits> &stream)
    -> std::basic_string<CharT, Traits> {
  const auto start{stream.tellg()};
  if (start != static_cast<std::streampos>(-1)) {
    stream.seekg(0, std::ios::end);
    const auto end{stream.tellg()};
    stream.seekg(start);
    if (end > start) {
      std::basic_string<CharT, Traits> result;
      result.resize(static_cast<std::size_t>(end - start));
      stream.read(result.data(), static_cast<std::streamsize>(result.size()));
      // Text-mode reads may return fewer characters than the byte count
      // (i.e. CRLF collapses to LF on Windows), so trim to actual.
      result.resize(static_cast<std::size_t>(stream.gcount()));
      return result;
    }
  }

  std::basic_ostringstream<CharT, Traits> buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

/// @ingroup io
///
/// Read an entire file into a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// const auto contents{sourcemeta::core::read_file_to_string("/tmp/foo.json")};
/// assert(!contents.empty());
/// ```
template <typename CharT = char, typename Traits = std::char_traits<CharT>>
auto read_file_to_string(const std::filesystem::path &path)
    -> std::basic_string<CharT, Traits> {
  if (std::filesystem::is_directory(path)) {
    throw IOIsADirectoryError{path};
  }

  const auto canonical_path{sourcemeta::core::canonical(path)};
  std::basic_ifstream<CharT, Traits> stream{canonical_path};
  if (!stream.is_open()) {
    throw IOFilePermissionError{canonical_path};
  }

  stream.exceptions(std::basic_ifstream<CharT, Traits>::badbit);

  // `file_size` only works on regular files, so when it cannot determine a
  // size (FIFOs from process substitution, sockets, pipes, or any other
  // stat failure) fall back to a streaming read on the already-open stream.
  std::error_code file_size_error;
  const auto size{std::filesystem::file_size(canonical_path, file_size_error)};
  if (file_size_error) {
    return read_to_string<CharT, Traits>(stream);
  }

  std::basic_string<CharT, Traits> result;
  result.resize(static_cast<std::size_t>(size));
  stream.read(result.data(), static_cast<std::streamsize>(result.size()));
  // Text-mode reads may return fewer characters than the byte count
  // (i.e. CRLF collapses to LF on Windows), so trim to actual.
  result.resize(static_cast<std::size_t>(stream.gcount()));
  return result;
}

/// @ingroup io
///
/// Drain `std::cin` fully into a string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
///
/// const auto input{sourcemeta::core::read_stdin()};
/// ```
inline auto read_stdin() -> std::string { return read_to_string(std::cin); }

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
