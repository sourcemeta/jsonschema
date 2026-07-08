#ifndef SOURCEMETA_CORE_IO_ERROR_H_
#define SOURCEMETA_CORE_IO_ERROR_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <exception>   // std::exception
#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup io
/// An error that represents a failure to memory-map a file
class SOURCEMETA_CORE_IO_EXPORT FileViewError : public std::exception {
public:
  /// Construct the error given the offending path and a message.
  FileViewError(std::filesystem::path path, const char *message)
      : path_{std::move(path)}, message_{message} {}
  FileViewError(std::filesystem::path path, std::string message) = delete;
  FileViewError(std::filesystem::path path, std::string &&message) = delete;
  FileViewError(std::filesystem::path path, std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
  const char *message_;
};

/// @ingroup io
/// The requested file does not exist.
class SOURCEMETA_CORE_IO_EXPORT IOFileNotFoundError : public std::exception {
public:
  /// Construct the error given the offending path.
  IOFileNotFoundError(std::filesystem::path path) : path_{std::move(path)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "File not found";
  }

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The current process lacks permission to access the requested path.
class SOURCEMETA_CORE_IO_EXPORT IOFilePermissionError : public std::exception {
public:
  /// Construct the error given the offending path.
  IOFilePermissionError(std::filesystem::path path) : path_{std::move(path)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Permission denied";
  }

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The path resolves to a directory where a regular file was expected.
class SOURCEMETA_CORE_IO_EXPORT IOIsADirectoryError : public std::exception {
public:
  /// Construct the error given the offending path.
  IOIsADirectoryError(std::filesystem::path path) : path_{std::move(path)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Expected a file but got a directory";
  }

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The path resolves to a regular file where a directory was expected.
class SOURCEMETA_CORE_IO_EXPORT IONotADirectoryError : public std::exception {
public:
  /// Construct the error given the offending path.
  IONotADirectoryError(std::filesystem::path path) : path_{std::move(path)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Expected a directory but got a file";
  }

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// The destination path already exists and cannot be replaced.
class SOURCEMETA_CORE_IO_EXPORT IOFileAlreadyExistsError
    : public std::exception {
public:
  /// Construct the error given the offending path.
  IOFileAlreadyExistsError(std::filesystem::path path)
      : path_{std::move(path)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "File already exists";
  }

  /// The offending path.
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup io
/// A read attempted to access bytes outside the bounds of the underlying
/// data.
class SOURCEMETA_CORE_IO_EXPORT IOReadOutOfBoundsError : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Read past the end of the underlying data";
  }
};

/// @ingroup io
/// A write to the underlying stream failed.
class SOURCEMETA_CORE_IO_EXPORT IOStreamWriteError : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Failed to write to stream";
  }
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
