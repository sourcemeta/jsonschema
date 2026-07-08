#ifndef SOURCEMETA_CORE_ERROR_FILE_H_
#define SOURCEMETA_CORE_ERROR_FILE_H_

#include <filesystem> // std::filesystem::path
#include <utility>    // std::move, std::forward

namespace sourcemeta::core {

/// @ingroup error
/// A wrapper that decorates an arbitrary exception type with a file path.
///
/// ```cpp
/// #include <sourcemeta/core/error.h>
/// #include <stdexcept>
///
/// throw sourcemeta::core::FileError<std::runtime_error>(
///     "/tmp/foo.json", "something went wrong");
/// ```
template <typename T> class FileError : public T {
public:
  /// Construct the error given the offending file path and the underlying
  /// exception arguments.
  template <typename... Args>
  FileError(std::filesystem::path path, Args &&...args)
      : T{std::forward<Args>(args)...}, path_{std::move(path)} {}

  /// The offending file path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

} // namespace sourcemeta::core

#endif
