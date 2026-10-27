#ifndef SOURCEMETA_CORE_METAPACK_FORMAT_H_
#define SOURCEMETA_CORE_METAPACK_FORMAT_H_

#ifndef SOURCEMETA_CORE_METAPACK_EXPORT
#include <sourcemeta/core/metapack_export.h>
#endif

#include <array>   // std::array
#include <bit>     // std::endian
#include <cstddef> // std::size_t
#include <cstdint> // std::int64_t, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <limits> // std::numeric_limits

namespace sourcemeta::core {

// Every multi-byte field is read where it lies rather than byte-swapped, so
// the byte order the format is written in is the only one it can be read in
static_assert(std::endian::native == std::endian::little,
              "The metapack format is little-endian and this host is not");

/// @ingroup metapack
/// How the contents of a file are stored
enum class MetapackEncoding : std::uint8_t {
  /// The contents are stored as they are
  Identity = 0,
  /// The contents are stored compressed as per RFC 1952
  GZIP = 1
};

#pragma pack(push, 1)
/// @ingroup metapack
/// The metadata that a file of this format begins with
struct MetapackHeader {
  /// The magic number that identifies the format
  std::uint32_t magic{0};
  /// The revision of the format the file was written to
  std::uint16_t format_version{0};
  /// The encoding the contents are stored under
  MetapackEncoding encoding{MetapackEncoding::Identity};
  /// Reserved for a future revision of the format
  std::uint8_t reserved{0};
  /// Nanoseconds since the Unix epoch at which the file was written
  std::int64_t last_modified{0};
  /// How many bytes the contents take once decoded
  std::uint64_t content_bytes{0};
  /// The SHA-256 digest of the decoded contents, which names this revision of
  /// them for a cache to compare against rather than being checked on read
  std::array<std::uint8_t, 32> checksum{};
  /// How many bytes the media type that follows the header takes
  std::uint16_t media_type_length{0};
};
#pragma pack(pop)

static_assert(alignof(MetapackHeader) == 1,
              "The header is read where it lies, so its layout must be packed");
static_assert(sizeof(MetapackHeader) == 58,
              "The header layout is part of the format, so moving, resizing or "
              "adding a field means bumping the format revision");

/// @ingroup metapack
/// How many characters it takes to render a checksum
inline constexpr std::size_t METAPACK_CHECKSUM_HEX_LENGTH{64};

static_assert(METAPACK_CHECKSUM_HEX_LENGTH ==
                  2 * sizeof(MetapackHeader::checksum),
              "Rendering a checksum takes two characters per byte");

/// @ingroup metapack
/// The longest media type a file of this format can declare
inline constexpr std::uint16_t METAPACK_MAXIMUM_MEDIA_TYPE_BYTES{
    std::numeric_limits<std::uint16_t>::max()};

/// @ingroup metapack
/// The largest extension area a file of this format can carry
inline constexpr std::uint32_t METAPACK_MAXIMUM_EXTENSION_BYTES{
    std::numeric_limits<std::uint32_t>::max()};

} // namespace sourcemeta::core

#endif
