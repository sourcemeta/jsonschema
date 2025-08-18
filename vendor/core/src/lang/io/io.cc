#include <sourcemeta/core/io.h>

#include <system_error> // std::error_code, std::system_category, std::generic_category

#if defined(_WIN32)
#include <windows.h> // HANDLE, CreateFileW, FlushFileBuffers, CloseHandle
#else
#include <cerrno>   // errno (for error codes)
#include <fcntl.h>  // open, O_RDWR
#include <unistd.h> // close, fsync
#endif

namespace sourcemeta::core {

auto canonical(const std::filesystem::path &path) -> std::filesystem::path {
  // On Linux, FIFO files (like /dev/fd/XX due to process substitution)
  // cannot be made canonical
  // See https://github.com/sourcemeta/jsonschema/issues/252
  return std::filesystem::is_fifo(path) ? path
                                        : std::filesystem::canonical(path);
}

auto weakly_canonical(const std::filesystem::path &path)
    -> std::filesystem::path {
  // On Linux, FIFO files (like /dev/fd/XX due to process substitution)
  // cannot be made canonical
  // See https://github.com/sourcemeta/jsonschema/issues/252
  return std::filesystem::is_fifo(path)
             ? path
             : std::filesystem::weakly_canonical(path);
}

auto starts_with(const std::filesystem::path &path,
                 const std::filesystem::path &prefix) -> bool {
  auto path_iterator = path.begin();
  auto prefix_iterator = prefix.begin();

  while (prefix_iterator != prefix.end()) {
    if (path_iterator == path.end() || *path_iterator != *prefix_iterator) {
      return false;
    }

    ++path_iterator;
    ++prefix_iterator;
  }

  return true;
}

auto flush(const std::filesystem::path &path) -> void {
#if defined(_WIN32)
  HANDLE hFile =
      CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE) {
    throw std::filesystem::filesystem_error{
        "failed to open file for flushing", path,
        std::error_code{static_cast<int>(GetLastError()),
                        std::system_category()}};
  }

  if (!FlushFileBuffers(hFile)) {
    const auto error_code = std::error_code{static_cast<int>(GetLastError()),
                                            std::system_category()};
    CloseHandle(hFile);
    throw std::filesystem::filesystem_error{"failed to flush the file to disk",
                                            path, error_code};
  }

  CloseHandle(hFile);

#else
  auto fd = ::open(path.c_str(), O_RDWR);
  if (fd == -1) {
    throw std::filesystem::filesystem_error{
        "failed to open file for flushing", path,
        std::error_code{errno, std::generic_category()}};
  }

  if (::fsync(fd) == -1) {
    const auto error_code = std::error_code{errno, std::generic_category()};
    ::close(fd);
    throw std::filesystem::filesystem_error{"failed to flush the file to disk",
                                            path, error_code};
  }

  ::close(fd);

  // After syncing a file, we should also sync the directory to ensure
  // durability
  auto directory_fd =
      ::open(path.parent_path().c_str(), O_DIRECTORY | O_RDONLY);
  if (directory_fd != -1) {
    if (::fsync(directory_fd) == -1) {
      const auto error_code = std::error_code{errno, std::generic_category()};
      ::close(directory_fd);
      throw std::filesystem::filesystem_error{
          "failed to flush the parent directory to disk", path, error_code};
    }

    ::close(directory_fd);
  }
#endif
}

} // namespace sourcemeta::core
