#include <sourcemeta/core/gzip.h>

extern "C" {
#include <archive.h>
#include <zlib.h>
}

#include <array>   // std::array
#include <sstream> // std::istringstream, std::ostringstream
#include <utility> // std::move

namespace sourcemeta::core {

constexpr auto ZLIB_BUFFER_SIZE{4096};

auto gzip(std::istream &input, std::ostream &output) -> void {
  z_stream zstream{};
  if (deflateInit2(&zstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS,
                   8, Z_DEFAULT_STRATEGY) != Z_OK) {
    throw GZIPError{"Could not compress input"};
  }

  std::array<char, ZLIB_BUFFER_SIZE> buffer_input;
  std::array<char, ZLIB_BUFFER_SIZE> buffer_output;
  bool reached_end_of_input{false};
  auto code{Z_OK};

  while (code != Z_STREAM_END) {
    if (zstream.avail_in == 0 && !reached_end_of_input) {
      input.read(buffer_input.data(), buffer_input.size());
      const auto bytes_read = input.gcount();
      if (bytes_read > 0) {
        zstream.next_in = reinterpret_cast<Bytef *>(buffer_input.data());
        zstream.avail_in = static_cast<uInt>(bytes_read);
      } else {
        reached_end_of_input = true;
      }
    }

    zstream.next_out = reinterpret_cast<Bytef *>(buffer_output.data());
    zstream.avail_out = static_cast<uInt>(buffer_output.size());

    const int flush_mode = reached_end_of_input ? Z_FINISH : Z_NO_FLUSH;
    code = deflate(&zstream, flush_mode);
    if (code == Z_STREAM_ERROR) {
      deflateEnd(&zstream);
      throw GZIPError{"Could not compress input"};
    }

    const auto bytes_written = buffer_output.size() - zstream.avail_out;
    if (bytes_written > 0) {
      output.write(buffer_output.data(),
                   static_cast<std::streamsize>(bytes_written));
      if (!output) {
        deflateEnd(&zstream);
        throw GZIPError{"Could not compress input"};
      }
    }
  }

  if (deflateEnd(&zstream) != Z_OK) {
    throw GZIPError{"Could not compress input"};
  }
}

auto gunzip(std::istream &input, std::ostream &output) -> void {
  z_stream zstream{};
  if (inflateInit2(&zstream, 16 + MAX_WBITS) != Z_OK) {
    throw GZIPError("Could not decompress input");
  }

  std::array<char, ZLIB_BUFFER_SIZE> buffer_input;
  std::array<char, ZLIB_BUFFER_SIZE> buffer_output;

  auto code{Z_OK};
  while (code != Z_STREAM_END) {
    if (zstream.avail_in == 0 && input) {
      input.read(buffer_input.data(), buffer_input.size());
      const auto bytes_read = input.gcount();
      if (bytes_read > 0) {
        zstream.next_in = reinterpret_cast<Bytef *>(buffer_input.data());
        zstream.avail_in = static_cast<uInt>(bytes_read);
      } else {
        break;
      }
    }

    zstream.next_out = reinterpret_cast<Bytef *>(buffer_output.data());
    zstream.avail_out = static_cast<uInt>(buffer_output.size());

    code = inflate(&zstream, Z_NO_FLUSH);
    if (code != Z_OK && code != Z_STREAM_END) {
      inflateEnd(&zstream);
      throw GZIPError("Could not decompress input");
    } else {
      const auto bytes_written = buffer_output.size() - zstream.avail_out;
      output.write(buffer_output.data(),
                   static_cast<std::streamsize>(bytes_written));
      if (!output) {
        inflateEnd(&zstream);
        throw GZIPError("Could not decompress input");
      }
    }
  }

  code = inflateEnd(&zstream);
  if (code != Z_OK) {
    throw GZIPError("Could not decompress input");
  }
}

auto gzip(std::istream &stream) -> std::string {
  std::ostringstream output;
  gzip(stream, output);
  return output.str();
}

auto gzip(const std::string &input) -> std::string {
  std::istringstream stream{input};
  return gzip(stream);
}

auto gunzip(std::istream &stream) -> std::string {
  std::ostringstream output;
  gunzip(stream, output);
  return output.str();
}

auto gunzip(const std::string &input) -> std::string {
  std::istringstream stream{input};
  return gunzip(stream);
}

} // namespace sourcemeta::core
