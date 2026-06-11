#include <sourcemeta/core/gzip.h>

#include "bit_reader.h"
#include "deflate.h"

#include <sourcemeta/core/crypto.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdint>     // std::uint8_t, std::uint16_t, std::uint32_t
#include <istream>     // std::istream
#include <string_view> // std::string_view

namespace sourcemeta::core {

static constexpr std::size_t GZIP_BUFFER_SIZE{16384};

struct GZIPStreamBuffer::Internal {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  std::istream &stream;
  BitReader reader;
  DeflateDecoder deflate;
  bool stream_ended{false};
  bool any_member_completed{false};
  bool member_started{false};
  std::uint32_t member_crc32{0};
  std::uint32_t member_isize{0};
  std::array<std::uint8_t, GZIP_BUFFER_SIZE> decompressed_buffer{};

  Internal(std::istream &source)
      : stream{source}, reader{source}, deflate{reader} {}
};

namespace {

// Accumulates the running CRC-32 over the header bytes that the FHCRC check
// covers, computing it only when the FHCRC flag is present
class HeaderChecksum {
public:
  HeaderChecksum(const bool track) : track_{track} {}

  auto feed(const std::uint8_t byte) -> void {
    if (this->track_) {
      const auto data{static_cast<char>(byte)};
      this->checksum_ =
          crc32_update(this->checksum_, std::string_view{&data, 1});
    }
  }

  [[nodiscard]] auto low16() const -> std::uint16_t {
    return static_cast<std::uint16_t>(this->checksum_ & 0xffffu);
  }

private:
  bool track_;
  std::uint32_t checksum_{0};
};

auto read_header_byte(BitReader &reader, HeaderChecksum &checksum)
    -> std::uint8_t {
  const auto byte{reader.read_byte()};
  checksum.feed(byte);
  return byte;
}

auto parse_member_header(BitReader &reader, const std::uint8_t first_byte)
    -> void {
  // RFC 1952 section 2.3.1.2: FHCRC covers every header byte up to but not
  // including the CRC16 itself, so feeding each byte as it is read produces
  // exactly the right value. The bytes are not retained, removing an
  // unbounded-memory path through FNAME and FCOMMENT

  // Caller already consumed the ID1 byte and verified it is 0x1f
  const auto id2{reader.read_byte()};
  if (id2 != 0x8b) {
    throw GZIPError{"Invalid gzip magic bytes"};
  }
  const auto compression_method{reader.read_byte()};
  if (compression_method != 8) {
    throw GZIPError{"Unsupported gzip compression method"};
  }
  const auto flag_byte{reader.read_byte()};
  if ((flag_byte & 0xe0) != 0) {
    throw GZIPError{"Reserved gzip FLG bits must be zero"};
  }

  HeaderChecksum checksum{(flag_byte & 0x02) != 0};
  checksum.feed(first_byte);
  checksum.feed(id2);
  checksum.feed(compression_method);
  checksum.feed(flag_byte);

  // MTIME (4 bytes) + XFL (1 byte) + OS (1 byte) are informational
  for (std::size_t index = 0; index < 6; ++index) {
    read_header_byte(reader, checksum);
  }

  if ((flag_byte & 0x04) != 0) {
    // FEXTRA
    const auto xlen_lo{read_header_byte(reader, checksum)};
    const auto xlen_hi{read_header_byte(reader, checksum)};
    const auto xlen{static_cast<std::size_t>(xlen_lo) |
                    (static_cast<std::size_t>(xlen_hi) << 8)};
    for (std::size_t index = 0; index < xlen; ++index) {
      read_header_byte(reader, checksum);
    }
  }

  if ((flag_byte & 0x08) != 0) {
    // FNAME (null-terminated)
    while (read_header_byte(reader, checksum) != 0) {
    }
  }

  if ((flag_byte & 0x10) != 0) {
    // FCOMMENT (null-terminated)
    while (read_header_byte(reader, checksum) != 0) {
    }
  }

  if ((flag_byte & 0x02) != 0) {
    // FHCRC: low 16 bits of CRC-32 over all preceding header bytes
    const auto stored_lo{reader.read_byte()};
    const auto stored_hi{reader.read_byte()};
    const std::uint16_t stored{static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(stored_lo) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(stored_hi)
                                   << 8))};
    if (stored != checksum.low16()) {
      throw GZIPError{"FHCRC mismatch"};
    }
  }
}

// Used for members past the first, where gzip(1) tolerates trailing
// non-member data, so a header that fails to validate is reported as
// trailing garbage rather than propagated as an error
auto try_parse_member_header(BitReader &reader, const std::uint8_t first_byte)
    -> bool {
  try {
    parse_member_header(reader, first_byte);
    return true;
  } catch (const GZIPError &) {
    return false;
  }
}

} // namespace

GZIPStreamBuffer::GZIPStreamBuffer(std::istream &compressed_stream)
    : internal{new Internal{compressed_stream}} {}

GZIPStreamBuffer::~GZIPStreamBuffer() = default;

auto GZIPStreamBuffer::underflow() -> int_type {
  if (this->gptr() && this->gptr() < this->egptr()) {
    return traits_type::to_int_type(*this->gptr());
  }
  if (this->internal->stream_ended) {
    return traits_type::eof();
  }

  while (true) {
    if (!this->internal->member_started) {
      std::uint8_t first_byte{0};
      if (!this->internal->reader.try_read_byte(first_byte)) {
        if (!this->internal->any_member_completed) {
          throw GZIPError{"Empty source stream"};
        }
        this->internal->stream_ended = true;
        return traits_type::eof();
      }
      if (this->internal->any_member_completed) {
        // gzip(1) silently ignores any trailing data after a complete member
        // rather than treating it as the start of a new member. Bytes that do
        // not form a valid member header end the stream without error,
        // independent of the first byte value
        if (first_byte != 0x1f ||
            !try_parse_member_header(this->internal->reader, first_byte)) {
          this->internal->stream_ended = true;
          return traits_type::eof();
        }
      } else {
        if (first_byte != 0x1f) {
          throw GZIPError{"Invalid gzip magic bytes"};
        }
        parse_member_header(this->internal->reader, first_byte);
      }

      this->internal->deflate.reset();
      this->internal->member_started = true;
      this->internal->member_crc32 = 0;
      this->internal->member_isize = 0;
    }

    const auto produced{this->internal->deflate.decompress(
        this->internal->decompressed_buffer.data(),
        this->internal->decompressed_buffer.size())};

    if (produced > 0) {
      this->internal->member_crc32 = crc32_update(
          this->internal->member_crc32,
          std::string_view{reinterpret_cast<const char *>(
                               this->internal->decompressed_buffer.data()),
                           produced});
      this->internal->member_isize += static_cast<std::uint32_t>(produced);

      auto *buffer_start{
          reinterpret_cast<char *>(this->internal->decompressed_buffer.data())};
      this->setg(buffer_start, buffer_start,
                 buffer_start + static_cast<std::ptrdiff_t>(produced));
      return traits_type::to_int_type(*this->gptr());
    }

    if (!this->internal->deflate.stream_ended()) {
      throw GZIPError{"Deflate stream ended unexpectedly"};
    }

    std::array<std::uint8_t, 8> trailer{};
    this->internal->reader.read_bytes(trailer.data(), trailer.size());
    const auto stored_crc32{static_cast<std::uint32_t>(trailer[0]) |
                            (static_cast<std::uint32_t>(trailer[1]) << 8) |
                            (static_cast<std::uint32_t>(trailer[2]) << 16) |
                            (static_cast<std::uint32_t>(trailer[3]) << 24)};
    const auto stored_isize{static_cast<std::uint32_t>(trailer[4]) |
                            (static_cast<std::uint32_t>(trailer[5]) << 8) |
                            (static_cast<std::uint32_t>(trailer[6]) << 16) |
                            (static_cast<std::uint32_t>(trailer[7]) << 24)};
    if (stored_crc32 != this->internal->member_crc32) {
      throw GZIPError{"Gzip member CRC32 mismatch"};
    }
    if (stored_isize != this->internal->member_isize) {
      throw GZIPError{"Gzip member ISIZE mismatch"};
    }

    this->internal->any_member_completed = true;
    this->internal->member_started = false;
  }
}

} // namespace sourcemeta::core
