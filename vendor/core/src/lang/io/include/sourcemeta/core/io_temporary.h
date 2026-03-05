#ifndef SOURCEMETA_CORE_IO_TEMPORARY_H_
#define SOURCEMETA_CORE_IO_TEMPORARY_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <filesystem>  // std::filesystem::path
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup io
/// An RAII class that creates a uniquely-named temporary directory on
/// construction and removes it on destruction.
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// sourcemeta::core::TemporaryDirectory staging{"/tmp", ".my-prefix-"};
/// assert(std::filesystem::exists(staging.path()));
/// ```
class SOURCEMETA_CORE_IO_EXPORT TemporaryDirectory {
public:
  TemporaryDirectory(const std::filesystem::path &parent,
                     const std::string_view prefix);
  ~TemporaryDirectory();

  TemporaryDirectory(const TemporaryDirectory &) = delete;
  auto operator=(const TemporaryDirectory &) -> TemporaryDirectory & = delete;
  TemporaryDirectory(TemporaryDirectory &&) = delete;
  auto operator=(TemporaryDirectory &&) -> TemporaryDirectory & = delete;

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path &;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::filesystem::path path_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
