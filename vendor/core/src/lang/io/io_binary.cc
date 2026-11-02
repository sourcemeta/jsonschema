#include <sourcemeta/core/io_binary.h>
#include <sourcemeta/core/io_error.h>

#include <sourcemeta/core/numeric.h>

#include <array>     // std::array
#include <bit>       // std::endian, std::byteswap
#include <cassert>   // assert
#include <cstring>   // std::memcpy
#include <ios>       // std::streamsize, std::streamoff
#include <streambuf> // std::streambuf

namespace {

template <typename T> auto to_little_endian(const T value) -> T {
  if constexpr (std::endian::native == std::endian::big) {
    return std::byteswap(value);
  } else {
    return value;
  }
}

// Going through the stream to take its position would run its sentry, which
// turns a stream that reports no position into a failed one. A stream that
// cannot report a position, such as one without a buffer or one that cannot
// seek, counts from the start of what it writes or reads instead
auto buffer_position(std::streambuf *const buffer,
                     const std::ios::openmode mode) -> std::size_t {
  if (buffer == nullptr) {
    return 0;
  }

  const auto position{buffer->pubseekoff(0, std::ios::cur, mode)};
  if (position == std::streampos{-1}) {
    return 0;
  }

  return static_cast<std::size_t>(position);
}

} // namespace

namespace sourcemeta::core {

BinaryWriter::BinaryWriter(std::ostream &stream)
    : stream_{&stream},
      offset_{buffer_position(stream.rdbuf(), std::ios::out)} {}

auto BinaryWriter::put_byte(const std::uint8_t value) -> void {
  this->put(value);
}

auto BinaryWriter::put_word(const std::uint16_t value) -> void {
  this->put(to_little_endian(value));
}

auto BinaryWriter::put_dword(const std::uint32_t value) -> void {
  this->put(to_little_endian(value));
}

auto BinaryWriter::put_qword(const std::uint64_t value) -> void {
  this->put(to_little_endian(value));
}

auto BinaryWriter::put_varint(const std::uint64_t value) -> void {
  std::array<std::byte, VARINT_MAXIMUM_BYTES> buffer{};
  const auto size{varint_encode(value, buffer.data())};
  this->put_bytes(buffer.data(), size);
}

auto BinaryWriter::put_varint_zigzag(const std::int64_t value) -> void {
  this->put_varint(zigzag_encode(value));
}

auto BinaryWriter::put_bytes(const std::byte *data, const std::size_t size)
    -> void {
  if (size == 0) {
    return;
  }

  this->stream_->write(reinterpret_cast<const char *>(data),
                       static_cast<std::streamsize>(size));
  if (this->stream_->fail()) {
    throw IOStreamWriteError{};
  }

  this->offset_ += size;
}

auto BinaryWriter::put_string(const std::string_view value) -> void {
  this->put_bytes(reinterpret_cast<const std::byte *>(value.data()),
                  value.size());
}

auto BinaryWriter::position() const noexcept -> std::size_t {
  return this->offset_;
}

BinaryReader::BinaryReader(const FileView &view) noexcept : view_{&view} {}

BinaryReader::BinaryReader(std::istream &stream)
    : stream_{&stream}, offset_{buffer_position(stream.rdbuf(), std::ios::in)} {
}

auto BinaryReader::get_byte() -> std::uint8_t {
  return this->get<std::uint8_t>();
}

auto BinaryReader::get_word() -> std::uint16_t {
  return to_little_endian(this->get<std::uint16_t>());
}

auto BinaryReader::get_dword() -> std::uint32_t {
  return to_little_endian(this->get<std::uint32_t>());
}

auto BinaryReader::get_qword() -> std::uint64_t {
  return to_little_endian(this->get<std::uint64_t>());
}

auto BinaryReader::get_varint() -> std::uint64_t {
  constexpr std::uint8_t CONTINUATION_BIT{0b10000000};
  std::array<std::byte, VARINT_MAXIMUM_BYTES> buffer{};
  std::size_t size{0};
  while (size < VARINT_MAXIMUM_BYTES) {
    const auto byte{this->get_byte()};
    buffer[size] = static_cast<std::byte>(byte);
    size += 1;
    if ((byte & CONTINUATION_BIT) == 0) {
      return varint_decode(buffer.data(), size);
    }
  }

  throw VarintParseError{};
}

auto BinaryReader::get_varint_zigzag() -> std::int64_t {
  return zigzag_decode(this->get_varint());
}

auto BinaryReader::get_string(const std::size_t size) -> std::string {
  std::string result;
  result.resize(size);
  this->get_bytes(reinterpret_cast<std::byte *>(result.data()), size);
  return result;
}

auto BinaryReader::position() const noexcept -> std::size_t {
  return this->offset_;
}

auto BinaryReader::seek(const std::size_t position) -> void {
  if (this->view_ != nullptr) {
    if (position > this->view_->size()) {
      throw IOReadOutOfBoundsError{};
    }

    this->offset_ = position;
    return;
  }

  assert(this->stream_);
  this->stream_->seekg(static_cast<std::streamoff>(position));
  if (this->stream_->fail()) {
    throw IOReadOutOfBoundsError{};
  }

  this->offset_ = position;
}

auto BinaryReader::rewind(const std::size_t relative_offset,
                          const std::size_t position) -> std::size_t {
  if (relative_offset > position) {
    throw IOReadOutOfBoundsError{};
  }

  const auto current{this->position()};
  this->seek(position - relative_offset);
  return current;
}

auto BinaryReader::get_bytes(std::byte *destination, const std::size_t size)
    -> void {
  if (this->view_ != nullptr) {
    if (size > this->view_->size() - this->offset_) {
      throw IOReadOutOfBoundsError{};
    }

    if (size > 0) {
      std::memcpy(destination, this->view_->as<std::byte>(this->offset_), size);
      this->offset_ += size;
    }

    return;
  }

  assert(this->stream_);
  this->stream_->read(reinterpret_cast<char *>(destination),
                      static_cast<std::streamsize>(size));
  const auto consumed{static_cast<std::size_t>(this->stream_->gcount())};
  this->offset_ += consumed;
  if (consumed != size) {
    throw IOReadOutOfBoundsError{};
  }
}

auto BinaryReader::has_more_data() const -> bool {
  if (this->view_ != nullptr) {
    return this->offset_ < this->view_->size();
  }

  assert(this->stream_);
  auto *buffer{this->stream_->rdbuf()};
  if (buffer == nullptr) {
    return false;
  }

  return buffer->in_avail() > 0 ||
         buffer->sgetc() != std::char_traits<char>::eof();
}

} // namespace sourcemeta::core
