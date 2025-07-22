#ifndef SOURCEMETA_CORE_IO_H_
#define SOURCEMETA_CORE_IO_H_

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

  std::ifstream stream{
      // On Linux, FIFO files (like /dev/fd/XX due to process substitution)
      // cannot be
      // made canonical
      // See https://github.com/sourcemeta/jsonschema/issues/252
      std::filesystem::is_fifo(path) ? path : std::filesystem::canonical(path)};
  stream.exceptions(std::ifstream::badbit);
  assert(!stream.fail());
  assert(stream.is_open());
  return stream;
}

} // namespace sourcemeta::core

#endif
