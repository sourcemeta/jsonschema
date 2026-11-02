#ifndef SOURCEMETA_CORE_IO_ERROR_H_
#define SOURCEMETA_CORE_IO_ERROR_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <filesystem>   // std::filesystem::path
#include <system_error> // std::error_code
#include <utility>      // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup io
/// An error that represents a failure to memory-map a file
class SOURCEMETA_CORE_IO_EXPORT FileViewError : public Error {
public:
  /// Construct the error given a message and the offending path.
  FileViewError(const StaticString message, std::filesystem::path path)
      : Error{message}, path_{std::move(path)} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The requested file does not exist.
class SOURCEMETA_CORE_IO_EXPORT IOFileNotFoundError : public Error {
public:
  /// Construct the error given the offending path.
  IOFileNotFoundError(std::filesystem::path path)
      : Error{"File not found"}, path_{std::move(path)} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The current process lacks permission to access the requested path.
class SOURCEMETA_CORE_IO_EXPORT IOFilePermissionError : public Error {
public:
  /// Construct the error given the offending path.
  IOFilePermissionError(std::filesystem::path path)
      : Error{"Permission denied"}, path_{std::move(path)} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The path resolves to a directory where a regular file was expected.
class SOURCEMETA_CORE_IO_EXPORT IOIsADirectoryError : public Error {
public:
  /// Construct the error given the offending path.
  IOIsADirectoryError(std::filesystem::path path)
      : Error{"Expected a file but got a directory"}, path_{std::move(path)} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The path resolves to a regular file where a directory was expected.
class SOURCEMETA_CORE_IO_EXPORT IONotADirectoryError : public Error {
public:
  /// Construct the error given the offending path.
  IONotADirectoryError(std::filesystem::path path)
      : Error{"Expected a directory but got a file"}, path_{std::move(path)} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The destination path already exists and cannot be replaced.
class SOURCEMETA_CORE_IO_EXPORT IOFileAlreadyExistsError : public Error {
public:
  /// Construct the error given the offending path.
  IOFileAlreadyExistsError(std::filesystem::path path)
      : Error{"File already exists"}, path_{std::move(path)} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// A filesystem operation failed for a reason the platform reports as a code.
class SOURCEMETA_CORE_IO_EXPORT IOFilesystemError : public Error {
public:
  /// Construct the error given a message, the offending path, and the code the
  /// platform reported.
  IOFilesystemError(const StaticString message, std::filesystem::path path,
                    const std::error_code code)
      : Error{message}, path_{std::move(path)}, code_{code} {}

  /// Construct the error given a message, the two paths the operation names,
  /// and the code the platform reported.
  IOFilesystemError(const StaticString message, std::filesystem::path path,
                    std::filesystem::path other, const std::error_code code)
      : Error{message}, path_{std::move(path)}, other_{std::move(other)},
        code_{code} {}

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

  /// The other path the operation names, empty when it names only one.
  [[nodiscard]] auto other() const noexcept -> const std::filesystem::path & {
    return this->other_;
  }

  /// The code the platform reported.
  [[nodiscard]] auto code() const noexcept -> std::error_code {
    return this->code_;
  }

private:
  std::filesystem::path path_;
  std::filesystem::path other_;
  std::error_code code_;
};

/// @ingroup io
/// A read attempted to access bytes outside the bounds of the underlying
/// data.
class SOURCEMETA_CORE_IO_EXPORT IOReadOutOfBoundsError : public Error {
public:
  IOReadOutOfBoundsError() noexcept
      : Error{"Read past the end of the underlying data"} {}
};

/// @ingroup io
/// A write to the underlying stream failed.
class SOURCEMETA_CORE_IO_EXPORT IOStreamWriteError : public Error {
public:
  IOStreamWriteError() noexcept : Error{"Failed to write to stream"} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
