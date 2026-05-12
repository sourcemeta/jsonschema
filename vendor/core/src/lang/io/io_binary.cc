#include <sourcemeta/core/io_binary.h>
#include <sourcemeta/core/io_error.h>

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

} // namespace

namespace sourcemeta::core {

BinaryWriter::BinaryWriter(std::ostream &stream) noexcept : stream_{&stream} {}

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
}

auto BinaryWriter::position() const -> std::size_t {
  const auto position{this->stream_->tellp()};
  if (position == std::streampos{-1}) {
    throw IOStreamWriteError{};
  }

  return static_cast<std::size_t>(position);
}

BinaryReader::BinaryReader(const FileView &view) noexcept : view_{&view} {}

BinaryReader::BinaryReader(std::istream &stream) noexcept : stream_{&stream} {}

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

auto BinaryReader::position() const -> std::size_t {
  if (this->view_) {
    return this->offset_;
  }

  assert(this->stream_);
  const auto position{this->stream_->tellg()};
  if (position == std::streampos{-1}) {
    throw IOReadOutOfBoundsError{};
  }

  return static_cast<std::size_t>(position);
}

auto BinaryReader::seek(const std::size_t position) -> void {
  if (this->view_) {
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
}

auto BinaryReader::get_bytes(std::byte *destination, const std::size_t size)
    -> void {
  if (this->view_) {
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
  if (consumed != size) {
    throw IOReadOutOfBoundsError{};
  }
}

auto BinaryReader::has_more_data() const -> bool {
  if (this->view_) {
    return this->offset_ < this->view_->size();
  }

  assert(this->stream_);
  auto *buffer{this->stream_->rdbuf()};
  return buffer->in_avail() > 0 ||
         buffer->sgetc() != std::char_traits<char>::eof();
}

} // namespace sourcemeta::core
