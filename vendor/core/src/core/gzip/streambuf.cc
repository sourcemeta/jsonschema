#include <sourcemeta/core/gzip.h>

extern "C" {
#include <zlib.h>
}

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <istream> // std::istream

namespace sourcemeta::core {

static constexpr std::size_t GZIP_BUFFER_SIZE{16384};

struct GZIPStreamBuffer::Internal {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  std::istream &stream;
  z_stream zlib_stream;
  bool stream_ended;
  std::array<char, GZIP_BUFFER_SIZE> compressed_buffer;
  std::array<char, GZIP_BUFFER_SIZE> decompressed_buffer;
};

GZIPStreamBuffer::GZIPStreamBuffer(std::istream &compressed_stream)
    : internal{new Internal{.stream = compressed_stream,
                            .zlib_stream = {},
                            .stream_ended = false,
                            .compressed_buffer = {},
                            .decompressed_buffer = {}}} {
  this->internal->zlib_stream.zalloc = Z_NULL;
  this->internal->zlib_stream.zfree = Z_NULL;
  this->internal->zlib_stream.opaque = Z_NULL;
  this->internal->zlib_stream.avail_in = 0;
  this->internal->zlib_stream.next_in = Z_NULL;

  // MAX_WBITS + 16 selects gzip decoding per the zlib API
  const auto result{inflateInit2(&this->internal->zlib_stream, MAX_WBITS + 16)};
  if (result != Z_OK) {
    throw GZIPError{"Could not initialize gzip decompressor"};
  }
}

GZIPStreamBuffer::~GZIPStreamBuffer() {
  inflateEnd(&this->internal->zlib_stream);
}

auto GZIPStreamBuffer::underflow() -> int_type {
  if (this->gptr() && this->gptr() < this->egptr()) {
    return traits_type::to_int_type(*this->gptr());
  }

  if (this->internal->stream_ended) {
    return traits_type::eof();
  }

  if (this->internal->zlib_stream.avail_in == 0) {
    this->internal->stream.read(
        this->internal->compressed_buffer.data(),
        static_cast<std::streamsize>(this->internal->compressed_buffer.size()));
    const auto bytes_read{this->internal->stream.gcount()};
    if (bytes_read > 0) {
      this->internal->zlib_stream.next_in =
          reinterpret_cast<Bytef *>(this->internal->compressed_buffer.data());
      this->internal->zlib_stream.avail_in = static_cast<uInt>(bytes_read);
    }
  }

  this->internal->zlib_stream.next_out =
      reinterpret_cast<Bytef *>(this->internal->decompressed_buffer.data());
  this->internal->zlib_stream.avail_out =
      static_cast<uInt>(this->internal->decompressed_buffer.size());

  const auto result{inflate(&this->internal->zlib_stream, Z_NO_FLUSH)};

  if (result != Z_OK && result != Z_STREAM_END) {
    throw GZIPError{"Could not decompress gzip stream"};
  }

  const auto bytes_produced{this->internal->decompressed_buffer.size() -
                            this->internal->zlib_stream.avail_out};
  if (bytes_produced == 0) {
    this->internal->stream_ended = true;
    return traits_type::eof();
  }

  if (result == Z_STREAM_END) {
    this->internal->stream_ended = true;
  }

  auto *buffer_start{this->internal->decompressed_buffer.data()};
  this->setg(buffer_start, buffer_start,
             buffer_start + static_cast<std::ptrdiff_t>(bytes_produced));
  return traits_type::to_int_type(*this->gptr());
}

} // namespace sourcemeta::core
