#ifndef SOURCEMETA_CORE_IO_FILEVIEW_H_
#define SOURCEMETA_CORE_IO_FILEVIEW_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <cstdint>    // std::uint8_t
#include <filesystem> // std::filesystem::path

namespace sourcemeta::core {

/// @ingroup io
/// A read-only memory-mapped file. For example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// struct Header {
///   std::uint32_t magic;
///   std::uint32_t version;
/// };
///
/// sourcemeta::core::FileView view{"/path/to/file.bin"};
/// const auto *header = view.as<Header>();
/// assert(header->magic == 0x12345678);
/// ```
class SOURCEMETA_CORE_IO_EXPORT FileView {
public:
  FileView(const std::filesystem::path &path);
  ~FileView();

  // Disable copying and moving
  FileView(const FileView &) = delete;
  FileView(FileView &&) = delete;
  auto operator=(const FileView &) -> FileView & = delete;
  auto operator=(FileView &&) -> FileView & = delete;

  /// The size of the memory-mapped data in bytes
  [[nodiscard]] auto size() const noexcept -> std::size_t;

  /// Interpret the memory-mapped data as a pointer to T at the given offset.
  template <typename T>
  [[nodiscard]] auto as(const std::size_t offset = 0) const noexcept
      -> const T * {
    assert(offset + sizeof(T) <= this->size_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<const T *>(this->data_ + offset);
  }

private:
  const std::uint8_t *data_{nullptr};
  std::size_t size_{0};
#if defined(_WIN32)
  void *file_handle_{nullptr};
  void *mapping_handle_{nullptr};
#else
  int file_descriptor_{-1};
#endif
};

} // namespace sourcemeta::core

#endif
