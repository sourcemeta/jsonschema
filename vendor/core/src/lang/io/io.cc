#include <sourcemeta/core/io.h>

#include <system_error> // std::error_code, std::system_category, std::generic_category

#if defined(_WIN32)
#include <windows.h> // HANDLE, CreateFileW, FlushFileBuffers, CloseHandle
#else
#include <cerrno>   // errno (for error codes)
#include <fcntl.h>  // open, O_RDWR, O_DIRECTORY, O_RDONLY
#include <unistd.h> // close, fsync
#endif

namespace sourcemeta::core {

auto canonical(const std::filesystem::path &path) -> std::filesystem::path {
  // On Linux, FIFO files (like /dev/fd/XX due to process substitution)
  // cannot be made canonical
  // See https://github.com/sourcemeta/jsonschema/issues/252
  if (std::filesystem::is_fifo(path)) {
    return path;
  }

  try {
    return std::filesystem::canonical(path);
  } catch (const std::filesystem::filesystem_error &error) {
    if (error.code() == std::errc::no_such_file_or_directory) {
      throw IOFileNotFoundError{path};
    }

    throw;
  }
}

auto weakly_canonical(const std::filesystem::path &path)
    -> std::filesystem::path {
  // On Linux, FIFO files (like /dev/fd/XX due to process substitution)
  // cannot be made canonical
  // See https://github.com/sourcemeta/jsonschema/issues/252
  if (std::filesystem::is_fifo(path)) {
    return path;
  }

  try {
    return std::filesystem::weakly_canonical(path);
  } catch (const std::filesystem::filesystem_error &error) {
    if (error.code() == std::errc::no_such_file_or_directory) {
      throw IOFileNotFoundError{path};
    }

    throw;
  }
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

auto hardlink_directory(const std::filesystem::path &source,
                        const std::filesystem::path &destination) -> void {
  assert(std::filesystem::is_directory(source));
  assert(!std::filesystem::exists(destination) ||
         std::filesystem::is_directory(destination));
  assert(!starts_with(destination, source));
  std::filesystem::create_directories(destination);
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator{source}) {
    const auto target{destination /
                      std::filesystem::relative(entry.path(), source)};
    if (entry.is_directory()) {
      std::filesystem::create_directories(target);
    } else if (entry.is_regular_file()) {
      std::filesystem::create_hard_link(entry.path(), target);
    }
  }
}

auto flush(const std::filesystem::path &path) -> void {
#if defined(_WIN32)
  HANDLE hFile =
      CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE) {
    const auto error_code = std::error_code{static_cast<int>(GetLastError()),
                                            std::system_category()};
    if (error_code == std::errc::no_such_file_or_directory) {
      throw IOFileNotFoundError{path};
    }
    if (error_code == std::errc::permission_denied) {
      throw IOFilePermissionError{path};
    }

    throw std::filesystem::filesystem_error{"failed to open file for flushing",
                                            path, error_code};
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
    const auto error_code = std::error_code{errno, std::generic_category()};
    if (error_code == std::errc::no_such_file_or_directory) {
      throw IOFileNotFoundError{path};
    }
    if (error_code == std::errc::permission_denied) {
      throw IOFilePermissionError{path};
    }

    throw std::filesystem::filesystem_error{"failed to open file for flushing",
                                            path, error_code};
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
