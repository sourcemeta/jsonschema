#include <sourcemeta/core/metapack_read.h>

#include "signature.h"

#include <sourcemeta/core/gzip.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/text.h>

#include <array>       // std::array
#include <cassert>     // assert
#include <chrono>      // std::chrono
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t, std::uint64_t
#include <cstring>     // std::memcpy
#include <filesystem>  // std::filesystem::path
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::in_range

namespace sourcemeta::core {

namespace {

// The contents as they read once the stored compression is undone
auto decompress(const MetapackView &parsed,
                const std::uint64_t maximum_decompression_ratio)
    -> std::optional<std::string> {
  assert(!parsed.payload.empty());
  // Integer division floors, so what lands exactly on the limit is only
  // within it when nothing is left over
  const auto declared_ratio{parsed.header->content_bytes /
                            parsed.payload.size()};
  if (declared_ratio > maximum_decompression_ratio ||
      (declared_ratio == maximum_decompression_ratio &&
       parsed.header->content_bytes % parsed.payload.size() != 0)) {
    return std::nullopt;
  }

  if (!std::in_range<std::size_t>(parsed.header->content_bytes)) {
    return std::nullopt;
  }

  const auto content_bytes{
      static_cast<std::size_t>(parsed.header->content_bytes)};

  try {
    // The declared size bounds the work as well as sizing the buffer, so
    // that contents which inflate past it are abandoned as they decode
    // rather than after a buffer has grown to hold all of them
    auto decompressed{gunzip(parsed.payload.data(), parsed.payload.size(),
                             content_bytes, content_bytes)};
    // Contents that inflate to less than the declared size are corrupt, or a
    // decompression bomb whose small declared size slipped past the guards
    if (decompressed.size() != content_bytes) {
      return std::nullopt;
    }

    return decompressed;
  } catch (const GZIPError &) {
    return std::nullopt;
  }
}

} // namespace

auto metapack_parse(const FileView &view) -> std::optional<MetapackView> {
  if (view.size() < sizeof(MetapackHeader) + sizeof(std::uint32_t)) {
    return std::nullopt;
  }

  const auto *header{view.as<MetapackHeader>()};
  if (header->magic != METAPACK_MAGIC ||
      header->format_version != METAPACK_VERSION) {
    return std::nullopt;
  }

  // An encoding that this revision of the format does not define leaves a
  // reader no way to know what the contents are, so the file is refused
  // rather than read as if the contents had been stored as they are
  if (header->encoding != MetapackEncoding::Identity &&
      header->encoding != MetapackEncoding::GZIP) {
    return std::nullopt;
  }

  const auto extension_size_offset{sizeof(MetapackHeader) +
                                   header->media_type_length};
  if (extension_size_offset + sizeof(std::uint32_t) > view.size()) {
    return std::nullopt;
  }

  // The media type decides where this lands, which is rarely an offset that a
  // four byte length may be read from in place
  std::uint32_t extension_size{0};
  std::memcpy(&extension_size, view.as<std::uint8_t>(extension_size_offset),
              sizeof(extension_size));

  const auto extension_offset{extension_size_offset + sizeof(std::uint32_t)};
  if (extension_size > view.size() - extension_offset) {
    return std::nullopt;
  }

  const auto payload_offset{extension_offset + extension_size};
  const auto bytes{view.bytes()};
  return MetapackView{
      .header = header,
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      .media_type = std::string_view{reinterpret_cast<const char *>(
                                         bytes.data() + sizeof(MetapackHeader)),
                                     header->media_type_length},
      .extension = bytes.subspan(extension_offset, extension_size),
      .payload = bytes.subspan(payload_offset)};
}

auto metapack_read_json(const MetapackView &parsed,
                        const std::uint64_t maximum_decompression_ratio)
    -> std::optional<JSON> {
  if (parsed.header == nullptr || parsed.payload.empty()) {
    return std::nullopt;
  }

  if (parsed.header->encoding == MetapackEncoding::GZIP) {
    const auto decompressed{decompress(parsed, maximum_decompression_ratio)};
    if (!decompressed.has_value()) {
      return std::nullopt;
    }

    return try_parse_json(decompressed.value());
  }

  // Contents stored as they are run to the end of the file, so a declared
  // size that disagrees means the file was truncated or appended to
  if (parsed.header->content_bytes != parsed.payload.size()) {
    return std::nullopt;
  }

  return try_parse_json(std::string_view{
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<const char *>(parsed.payload.data()),
      static_cast<std::size_t>(parsed.header->content_bytes)});
}

auto metapack_read_json(const FileView &view,
                        const std::uint64_t maximum_decompression_ratio)
    -> std::optional<JSON> {
  const auto parsed{metapack_parse(view)};
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  return metapack_read_json(parsed.value(), maximum_decompression_ratio);
}

auto metapack_read_json(const std::filesystem::path &path,
                        const std::uint64_t maximum_decompression_ratio)
    -> std::optional<JSON> {
  try {
    const FileView view{path};
    return metapack_read_json(view, maximum_decompression_ratio);
  } catch (const FileViewError &) {
    return std::nullopt;
  }
}

auto metapack_checksum_hex(const MetapackHeader &header)
    -> std::array<char, METAPACK_CHECKSUM_HEX_LENGTH> {
  std::array<char, METAPACK_CHECKSUM_HEX_LENGTH> result;
  bytes_to_hex(header.checksum, result);
  return result;
}

auto metapack_last_modified(const MetapackHeader &header)
    -> std::chrono::system_clock::time_point {
  const std::chrono::nanoseconds nanoseconds{header.last_modified};
  return std::chrono::system_clock::time_point{
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          nanoseconds)};
}

} // namespace sourcemeta::core
