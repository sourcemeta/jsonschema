#include <sourcemeta/core/io_error.h>
#include <sourcemeta/core/io_fileview.h>

#if defined(_WIN32)
// Ahead of the platform header, so that the macros it brings in cannot reach
// the standard ones
#include <cassert>      // assert
#include <filesystem>   // std::filesystem::is_directory
#include <system_error> // std::error_code

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>    // open, O_RDONLY
#include <sys/mman.h> // mmap, munmap
#include <sys/stat.h> // fstat
#include <unistd.h>   // close
#endif

namespace sourcemeta::core {

#if defined(_WIN32)

FileView::FileView(const std::filesystem::path &path) {
  this->file_handle_ =
      CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (this->file_handle_ == INVALID_HANDLE_VALUE) {
    // Opening a directory here needs a flag this does not ask for, so one
    // fails to open at all rather than failing to be mapped. Reporting that as
    // a file that could not be opened would have the same refusal give a
    // different account of itself depending on the system it ran on
    std::error_code directory_error;
    if (std::filesystem::is_directory(path, directory_error)) {
      throw FileViewError(path, "Could not map a directory into memory");
    }

    throw FileViewError(path, "Could not open the file");
  }

  // Only a file on disk reports a size that says what can be read from it, so
  // anything else is refused here rather than handed back as a view of nothing
  if (GetFileType(this->file_handle_) != FILE_TYPE_DISK) {
    CloseHandle(this->file_handle_);
    throw FileViewError(path, "Could not map this kind of file into memory");
  }

  LARGE_INTEGER file_size;
  if (GetFileSizeEx(this->file_handle_, &file_size) == 0) {
    CloseHandle(this->file_handle_);
    throw FileViewError(path, "Could not determine the file size");
  }
  this->size_ = static_cast<std::size_t>(file_size.QuadPart);

  // Mapping a zero-length file is not possible, so leave the view empty
  if (this->size_ == 0) {
    this->data_ = nullptr;
    return;
  }

  this->mapping_handle_ = CreateFileMappingW(this->file_handle_, nullptr,
                                             PAGE_READONLY, 0, 0, nullptr);
  if (this->mapping_handle_ == nullptr) {
    CloseHandle(this->file_handle_);
    throw FileViewError(path, "Could not create a file mapping");
  }

  this->data_ = static_cast<const std::uint8_t *>(
      MapViewOfFile(this->mapping_handle_, FILE_MAP_READ, 0, 0, 0));
  if (this->data_ == nullptr) {
    CloseHandle(this->mapping_handle_);
    CloseHandle(this->file_handle_);
    throw FileViewError(path, "Could not map the file into memory");
  }
}

FileView::~FileView() {
  if (this->data_ != nullptr) {
    UnmapViewOfFile(this->data_);
  }

  if (this->mapping_handle_ != nullptr) {
    CloseHandle(this->mapping_handle_);
  }

  if (this->file_handle_ != nullptr &&
      this->file_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(this->file_handle_);
  }
}

#else

FileView::FileView(const std::filesystem::path &path) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  this->file_descriptor_ = open(path.c_str(), O_RDONLY);
  if (this->file_descriptor_ == -1) {
    throw FileViewError(path, "Could not open the file");
  }

  struct stat file_stat;
  if (fstat(this->file_descriptor_, &file_stat) != 0) {
    close(this->file_descriptor_);
    throw FileViewError(path, "Could not determine the file size");
  }

  // Opening something that is not a regular file is allowed, the size it
  // reports says nothing about what can be read from it, and whether it can be
  // mapped at all is unspecified. Where that size comes back as zero, and it
  // does for a directory on at least one widely used filesystem and for a
  // character device everywhere, the empty-view shortcut below would hand back
  // a view of nothing rather than refusing
  if (S_ISDIR(file_stat.st_mode)) {
    close(this->file_descriptor_);
    throw FileViewError(path, "Could not map a directory into memory");
  }

  if (!S_ISREG(file_stat.st_mode)) {
    close(this->file_descriptor_);
    throw FileViewError(path, "Could not map this kind of file into memory");
  }

  this->size_ = static_cast<std::size_t>(file_stat.st_size);

  // Mapping a zero-length region fails with EINVAL, so leave the view empty
  if (this->size_ == 0) {
    this->data_ = nullptr;
    return;
  }

  void *mapped = mmap(nullptr, this->size_, PROT_READ, MAP_PRIVATE,
                      this->file_descriptor_, 0);
  if (mapped == MAP_FAILED) {
    close(this->file_descriptor_);
    throw FileViewError(path, "Could not map the file into memory");
  }

  this->data_ = static_cast<const std::uint8_t *>(mapped);
}

FileView::~FileView() {
  // A view is neither copied nor moved and its constructor throws rather than
  // returning with no descriptor, so there is always one to close, and a
  // mapping only exists where there were bytes to map
  assert(this->file_descriptor_ != -1);
  if (this->data_ != nullptr) {
    assert(this->size_ > 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    munmap(const_cast<std::uint8_t *>(this->data_), this->size_);
  }

  close(this->file_descriptor_);
}

#endif

auto FileView::size() const noexcept -> std::size_t { return this->size_; }

} // namespace sourcemeta::core
