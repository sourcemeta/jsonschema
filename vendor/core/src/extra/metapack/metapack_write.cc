#include <sourcemeta/core/metapack_write.h>

#include "signature.h"

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/gzip.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono
#include <cstdint>     // std::uint16_t, std::uint32_t
#include <filesystem>  // std::filesystem::path
#include <ios>         // std::streamsize
#include <ostream>     // std::ostream
#include <span>        // std::span
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::in_range, std::move

namespace sourcemeta::core {

namespace {

auto write_header(std::ostream &output, const std::string_view media_type,
                  const MetapackEncoding encoding,
                  const std::span<const std::uint8_t> extension,
                  const std::string_view contents) -> void {
  MetapackHeader header;
  header.magic = METAPACK_MAGIC;
  header.format_version = METAPACK_VERSION;
  header.encoding = encoding;

  const auto now{std::chrono::system_clock::now()};
  header.last_modified = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             now.time_since_epoch())
                             .count();
  header.content_bytes = contents.size();
  header.checksum = sha256_digest(contents);
  header.media_type_length = static_cast<std::uint16_t>(media_type.size());

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  output.write(reinterpret_cast<const char *>(&header), sizeof(MetapackHeader));
  output.write(media_type.data(),
               static_cast<std::streamsize>(media_type.size()));

  const auto extension_size{static_cast<std::uint32_t>(extension.size())};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  output.write(reinterpret_cast<const char *>(&extension_size),
               sizeof(extension_size));
  if (!extension.empty()) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    output.write(reinterpret_cast<const char *>(extension.data()),
                 static_cast<std::streamsize>(extension.size()));
  }
}

auto write_metapack(const std::filesystem::path &destination,
                    const std::string_view media_type,
                    const MetapackEncoding encoding,
                    const std::span<const std::uint8_t> extension,
                    const std::string_view contents) -> void {
  // A length the header cannot record would be truncated into one it can,
  // which silently moves every section that follows it
  if (!std::in_range<std::uint16_t>(media_type.size())) {
    throw MetapackError{"The media type is too long for this format"};
  }

  if (!std::in_range<std::uint32_t>(extension.size())) {
    throw MetapackError{"The extension is too large for this format"};
  }

  // An encoding that this revision of the format does not define would be
  // written into a file that no reader of it can accept
  if (encoding != MetapackEncoding::Identity &&
      encoding != MetapackEncoding::GZIP) {
    throw MetapackError{"Unknown encoding"};
  }

  // Only contents that are stored compressed are compressed at all
  const auto compressed{
      encoding == MetapackEncoding::GZIP
          ? gzip(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                reinterpret_cast<const std::uint8_t *>(contents.data()),
                contents.size())
          : std::string{}};
  write_file(destination, [&](std::ostream &output) -> void {
    write_header(output, media_type, encoding, extension, contents);

    if (encoding == MetapackEncoding::GZIP) {
      output.write(compressed.data(),
                   static_cast<std::streamsize>(compressed.size()));
    } else {
      output.write(contents.data(),
                   static_cast<std::streamsize>(contents.size()));
    }
  });
}

} // namespace

auto metapack_write_json(const std::filesystem::path &destination,
                         const JSON &document,
                         const std::string_view media_type,
                         const MetapackEncoding encoding,
                         const std::span<const std::uint8_t> extension)
    -> void {
  std::ostringstream buffer;
  stringify(document, buffer);
  write_metapack(destination, media_type, encoding, extension,
                 std::move(buffer).str());
}

auto metapack_write_json_pretty(const std::filesystem::path &destination,
                                const JSON &document,
                                const std::string_view media_type,
                                const MetapackEncoding encoding,
                                const std::span<const std::uint8_t> extension)
    -> void {
  std::ostringstream buffer;
  prettify(document, buffer);
  write_metapack(destination, media_type, encoding, extension,
                 std::move(buffer).str());
}

auto metapack_write_text(const std::filesystem::path &destination,
                         const std::string_view contents,
                         const std::string_view media_type,
                         const MetapackEncoding encoding,
                         const std::span<const std::uint8_t> extension)
    -> void {
  std::string line{contents};
  line += '\n';
  write_metapack(destination, media_type, encoding, extension, line);
}

auto metapack_write_bytes(const std::filesystem::path &destination,
                          const std::span<const std::uint8_t> contents,
                          const std::string_view media_type,
                          const MetapackEncoding encoding,
                          const std::span<const std::uint8_t> extension)
    -> void {
  write_metapack(
      destination, media_type, encoding, extension,
      std::string_view{
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<const char *>(contents.data()), contents.size()});
}

auto metapack_write_file(const std::filesystem::path &destination,
                         const std::filesystem::path &source,
                         const std::string_view media_type,
                         const MetapackEncoding encoding,
                         const std::span<const std::uint8_t> extension)
    -> void {
  write_metapack(destination, media_type, encoding, extension,
                 read_file_to_string(source));
}

} // namespace sourcemeta::core
