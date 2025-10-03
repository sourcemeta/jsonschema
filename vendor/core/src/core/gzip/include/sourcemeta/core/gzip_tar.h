#ifndef SOURCEMETA_CORE_GZIP_TAR_H_
#define SOURCEMETA_CORE_GZIP_TAR_H_

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

#include <cstddef>    // std::size_t
#include <filesystem> // std::filesystem::path
#include <fstream>    // std::ofstream
#include <memory>     // std::unique_ptr

namespace sourcemeta::core {

/// @ingroup gzip
///
/// Create GZIP-compressed TAR archives in memory. For example:
///
/// ```cpp
/// #include <sourcemeta/core/gzip_tar.h>
/// #include <fstream>
///
/// sourcemeta::core::GZIPTar archive;
/// archive.push("hello.txt", "Hello, World!");
///
/// std::ofstream file{"archive.tar.gz", std::ios::binary};
/// file.write(archive.data(), archive.size());
/// ```
class SOURCEMETA_CORE_GZIP_EXPORT GZIPTar {
public:
  GZIPTar();
  ~GZIPTar();

  // Add a file to the archive with string content
  auto push(const std::filesystem::path &path, std::string_view content)
      -> void;

  // Add a file with raw binary data
  auto push(const std::filesystem::path &path, const void *data,
            std::size_t size) -> void;

  // Get pointer to the compressed archive data
  auto data() -> const char *;

  // Get the size of the compressed data
  auto size() -> std::size_t;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  // PIMPL to hide archive.h dependency
  struct Implementation;
  std::unique_ptr<Implementation> internal_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::core

#endif
