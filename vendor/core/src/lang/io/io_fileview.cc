#include <sourcemeta/core/io_error.h>
#include <sourcemeta/core/io_fileview.h>

#if defined(_WIN32)
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
    throw FileViewError(path, "Could not open the file");
  }

  LARGE_INTEGER file_size;
  if (GetFileSizeEx(this->file_handle_, &file_size) == 0) {
    CloseHandle(this->file_handle_);
    throw FileViewError(path, "Could not determine the file size");
  }
  this->size_ = static_cast<std::size_t>(file_size.QuadPart);

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
  this->size_ = static_cast<std::size_t>(file_stat.st_size);

  void *mapped = mmap(nullptr, this->size_, PROT_READ, MAP_PRIVATE,
                      this->file_descriptor_, 0);
  if (mapped == MAP_FAILED) {
    close(this->file_descriptor_);
    throw FileViewError(path, "Could not map the file into memory");
  }

  this->data_ = static_cast<const std::uint8_t *>(mapped);
}

FileView::~FileView() {
  if (this->data_ != nullptr && this->size_ > 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    munmap(const_cast<std::uint8_t *>(this->data_), this->size_);
  }

  if (this->file_descriptor_ != -1) {
    close(this->file_descriptor_);
  }
}

#endif

auto FileView::size() const noexcept -> std::size_t { return this->size_; }

} // namespace sourcemeta::core
