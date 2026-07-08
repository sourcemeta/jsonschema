#ifndef SOURCEMETA_CORE_IO_BINARY_H_
#define SOURCEMETA_CORE_IO_BINARY_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <sourcemeta/core/io_fileview.h>

#include <cstddef> // std::byte, std::size_t
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <istream> // std::istream
#include <ostream> // std::ostream
#include <type_traits> // std::is_trivially_copyable_v

namespace sourcemeta::core {

/// @ingroup io
///
/// Typed wrapper over an output stream.
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <fstream>
///
/// std::ofstream raw{"/tmp/out.bin", std::ios::binary};
/// sourcemeta::core::BinaryWriter writer{raw};
/// writer.put_dword(0x12345678);
/// ```
class SOURCEMETA_CORE_IO_EXPORT BinaryWriter {
public:
  /// Construct a writer over the given output stream.
  BinaryWriter(std::ostream &stream) noexcept;

  // Prevent copying, as this class is tied to a stream resource
  BinaryWriter(const BinaryWriter &) = delete;
  BinaryWriter(BinaryWriter &&) = delete;
  auto operator=(const BinaryWriter &) -> BinaryWriter & = delete;
  auto operator=(BinaryWriter &&) -> BinaryWriter & = delete;

  /// Write a single byte.
  auto put_byte(const std::uint8_t value) -> void;
  /// Write a 16-bit unsigned integer.
  auto put_word(const std::uint16_t value) -> void;
  /// Write a 32-bit unsigned integer.
  auto put_dword(const std::uint32_t value) -> void;
  /// Write a 64-bit unsigned integer.
  auto put_qword(const std::uint64_t value) -> void;

  /// Write a raw sequence of bytes.
  auto put_bytes(const std::byte *data, const std::size_t size) -> void;

  /// The number of bytes written so far.
  [[nodiscard]] auto position() const -> std::size_t;

private:
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  auto put(const T &value) -> void {
    this->put_bytes(reinterpret_cast<const std::byte *>(&value), sizeof(T));
  }

  std::ostream *stream_;
};

/// @ingroup io
///
/// Cursor-tracking reader over a `FileView` or an `std::istream`.
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
///
/// sourcemeta::core::FileView view{"/tmp/out.bin"};
/// sourcemeta::core::BinaryReader reader{view};
/// const auto value{reader.get_dword()};
/// ```
class SOURCEMETA_CORE_IO_EXPORT BinaryReader {
public:
  /// Construct a reader over the given file view.
  BinaryReader(const FileView &view) noexcept;
  /// Construct a reader over the given input stream.
  BinaryReader(std::istream &stream) noexcept;

  // Prevent copying, as this class is tied to an input resource
  BinaryReader(const BinaryReader &) = delete;
  BinaryReader(BinaryReader &&) = delete;
  auto operator=(const BinaryReader &) -> BinaryReader & = delete;
  auto operator=(BinaryReader &&) -> BinaryReader & = delete;

  /// Read a single byte.
  [[nodiscard]] auto get_byte() -> std::uint8_t;
  /// Read a 16-bit unsigned integer.
  [[nodiscard]] auto get_word() -> std::uint16_t;
  /// Read a 32-bit unsigned integer.
  [[nodiscard]] auto get_dword() -> std::uint32_t;
  /// Read a 64-bit unsigned integer.
  [[nodiscard]] auto get_qword() -> std::uint64_t;

  /// Read a raw sequence of bytes.
  auto get_bytes(std::byte *destination, const std::size_t size) -> void;

  /// The current cursor position in bytes.
  [[nodiscard]] auto position() const -> std::size_t;

  /// Move the cursor to `position`.
  auto seek(const std::size_t position) -> void;

  /// Whether the source has unconsumed bytes at the current cursor.
  [[nodiscard]] auto has_more_data() const -> bool;

private:
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  [[nodiscard]] auto get() -> T {
    T value;
    this->get_bytes(reinterpret_cast<std::byte *>(&value), sizeof(T));
    return value;
  }

  // Exactly one is non-null
  const FileView *view_{nullptr};
  std::istream *stream_{nullptr};
  std::size_t offset_{0};
};

} // namespace sourcemeta::core

#endif
