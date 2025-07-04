#include <sourcemeta/core/gzip.h>

extern "C" {
#include <zlib.h>
}

#include <array>   // std::array
#include <cstring> // std::memset
#include <sstream> // std::ostringstream

namespace sourcemeta::core {

auto gzip(std::string_view input) -> std::optional<std::string> {
  z_stream stream;
  std::memset(&stream, 0, sizeof(stream));
  int code = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                          16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  if (code != Z_OK) {
    return std::nullopt;
  }

  stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
  stream.avail_in = static_cast<uInt>(input.size());

  std::array<char, 4096> buffer;
  std::ostringstream compressed;

  do {
    stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
    stream.avail_out = sizeof(buffer);
    code = deflate(&stream, Z_FINISH);
    compressed.write(buffer.data(), sizeof(buffer) - stream.avail_out);
  } while (code == Z_OK);

  if (code != Z_STREAM_END) {
    return std::nullopt;
  }

  code = deflateEnd(&stream);
  if (code != Z_OK) {
    return std::nullopt;
  }

  return compressed.str();
}

} // namespace sourcemeta::core
