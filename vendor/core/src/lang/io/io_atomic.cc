#include <sourcemeta/core/io.h>
#include <sourcemeta/core/io_atomic.h>

#include <cassert>      // assert
#include <cerrno>       // EACCES, errno
#include <filesystem>   // std::filesystem
#include <fstream>      // std::ofstream
#include <ios>          // std::ios::binary, std::ios::trunc
#include <ostream>      // std::ostream
#include <system_error> // std::error_code, std::generic_category

#if defined(__linux__)
#include <fcntl.h>       // AT_FDCWD
#include <linux/fs.h>    // RENAME_EXCHANGE
#include <sys/syscall.h> // SYS_renameat2
#include <unistd.h>      // syscall
#elif defined(__APPLE__)
#include <fcntl.h>     // AT_FDCWD
#include <sys/stdio.h> // renameatx_np, RENAME_SWAP
#endif

namespace {

class AtomicFileWriter {
public:
  AtomicFileWriter(const std::filesystem::path &destination)
      : destination_{destination}, staging_{destination} {
    // The staging file lives next to the destination so that
    // `std::filesystem::rename` stays on a single filesystem and remains
    // atomic. Using the system-wide temporary directory would risk `EXDEV`
    // errors on cross-filesystem builds (CI containers, NFS mounts, etc.).
    this->staging_ += ".tmp";

    if (this->destination_.has_parent_path()) {
      std::filesystem::create_directories(this->destination_.parent_path());
    }

    this->stream_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
      this->stream_.open(this->staging_, std::ios::binary | std::ios::trunc);
    } catch (...) {
      // Capture before any other syscall can clobber errno
      const auto open_errno{errno};
      std::error_code remove_error;
      std::filesystem::remove(this->staging_, remove_error);
      if (open_errno == EACCES) {
        throw sourcemeta::core::IOFilePermissionError{this->destination_};
      }

      throw;
    }
  }

  ~AtomicFileWriter() {
    if (!this->committed_) {
      if (this->stream_.is_open()) {
        this->stream_.exceptions(std::ios::goodbit);
        this->stream_.close();
      }

      std::error_code error;
      std::filesystem::remove(this->staging_, error);
    }
  }

  AtomicFileWriter(const AtomicFileWriter &) = delete;
  AtomicFileWriter(AtomicFileWriter &&) = delete;
  auto operator=(const AtomicFileWriter &) -> AtomicFileWriter & = delete;
  auto operator=(AtomicFileWriter &&) -> AtomicFileWriter & = delete;

  [[nodiscard]] auto stream() -> std::ostream & { return this->stream_; }

  auto commit() -> void {
    assert(!this->committed_);
    this->stream_.flush();
    this->stream_.close();
    sourcemeta::core::flush(this->staging_);
    std::filesystem::rename(this->staging_, this->destination_);
    this->committed_ = true;
  }

private:
  std::filesystem::path destination_;
  std::filesystem::path staging_;
  std::ofstream stream_;
  bool committed_{false};
};

} // namespace

namespace sourcemeta::core {

auto atomic_write_file(const std::filesystem::path &path,
                       const std::string_view contents) -> void {
  AtomicFileWriter file{path};
  file.stream().write(contents.data(),
                      static_cast<std::streamsize>(contents.size()));
  file.commit();
}

auto atomic_write_file(const std::filesystem::path &path,
                       const std::span<const std::byte> contents) -> void {
  AtomicFileWriter file{path};
  file.stream().write(reinterpret_cast<const char *>(contents.data()),
                      static_cast<std::streamsize>(contents.size()));
  file.commit();
}

auto atomic_write_file(const std::filesystem::path &path,
                       const std::function<void(std::ostream &)> &writer)
    -> void {
  AtomicFileWriter file{path};
  writer(file.stream());
  file.commit();
}

auto atomic_directory_swap(const std::filesystem::path &original,
                           const std::filesystem::path &replacement) -> void {
  assert(std::filesystem::is_directory(replacement));
  assert(!std::filesystem::exists(original) ||
         std::filesystem::is_directory(original));
  assert(!original.parent_path().empty());

  if (!std::filesystem::exists(original)) {
    std::filesystem::rename(replacement, original);
    return;
  }

  // Atomic swap via renameat2 with RENAME_EXCHANGE
#if defined(__linux__)
  if (syscall(SYS_renameat2, AT_FDCWD, replacement.c_str(), AT_FDCWD,
              original.c_str(), RENAME_EXCHANGE) != 0) {
    const auto error_code = std::error_code{errno, std::generic_category()};
    if (error_code == std::errc::no_such_file_or_directory) {
      throw IOFileNotFoundError{original};
    }
    if (error_code == std::errc::permission_denied) {
      throw IOFilePermissionError{original};
    }

    throw std::filesystem::filesystem_error{
        "failed to atomically swap directories", replacement, original,
        error_code};
  }

  // Atomic swap via renameatx_np with RENAME_SWAP
#elif defined(__APPLE__)
  if (renameatx_np(AT_FDCWD, replacement.c_str(), AT_FDCWD, original.c_str(),
                   RENAME_SWAP) != 0) {
    const auto error_code = std::error_code{errno, std::generic_category()};
    if (error_code == std::errc::no_such_file_or_directory) {
      throw IOFileNotFoundError{original};
    }
    if (error_code == std::errc::permission_denied) {
      throw IOFilePermissionError{original};
    }

    throw std::filesystem::filesystem_error{
        "failed to atomically swap directories", replacement, original,
        error_code};
  }

#else
  // Non-atomic fallback: two-rename approach with rollback
  //
  // Note we cannot safely use the temporary directory of the system as it
  // might be in another volume
  TemporaryDirectory temporary{original.parent_path(), ".swap-"};
  std::filesystem::remove(temporary.path());
  std::filesystem::rename(original, temporary.path());
  try {
    std::filesystem::rename(replacement, original);
  } catch (...) {
    std::filesystem::rename(temporary.path(), original);
    throw;
  }

  std::filesystem::rename(temporary.path(), replacement);
#endif
}

} // namespace sourcemeta::core
