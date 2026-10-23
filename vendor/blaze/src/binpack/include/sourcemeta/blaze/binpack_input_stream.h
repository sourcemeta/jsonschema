#ifndef SOURCEMETA_BLAZE_BINPACK_INPUT_STREAM_H_
#define SOURCEMETA_BLAZE_BINPACK_INPUT_STREAM_H_

#ifndef SOURCEMETA_BLAZE_BINPACK_EXPORT
#include <sourcemeta/blaze/binpack_export.h>
#endif

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>

#include <cstdint> // std::uint64_t, std::int64_t
#include <istream> // std::basic_istream

namespace sourcemeta::blaze {

/// @ingroup binpack
class SOURCEMETA_BLAZE_BINPACK_EXPORT BinPackInputStream
    : public sourcemeta::core::BinaryReader {
public:
  using Stream = std::basic_istream<sourcemeta::core::JSON::Char,
                                    sourcemeta::core::JSON::CharTraits>;
  BinPackInputStream(Stream &input);

  // Seek backwards given a relative offset
  auto rewind(const std::uint64_t relative_offset, const std::uint64_t position)
      -> std::uint64_t;
  auto get_varint() -> std::uint64_t;
  auto get_varint_zigzag() -> std::int64_t;
  auto get_string_utf8(const std::uint64_t length)
      -> sourcemeta::core::JSON::String;
};

} // namespace sourcemeta::blaze

#endif
