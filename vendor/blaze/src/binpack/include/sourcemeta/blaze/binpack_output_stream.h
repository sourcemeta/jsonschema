#ifndef SOURCEMETA_BLAZE_BINPACK_OUTPUT_STREAM_H_
#define SOURCEMETA_BLAZE_BINPACK_OUTPUT_STREAM_H_

#ifndef SOURCEMETA_BLAZE_BINPACK_EXPORT
#include <sourcemeta/blaze/binpack_export.h>
#endif

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>

#include <cstdint> // std::uint64_t, std::int64_t
#include <ostream> // std::basic_ostream

namespace sourcemeta::blaze {

/// @ingroup binpack
class SOURCEMETA_BLAZE_BINPACK_EXPORT BinPackOutputStream
    : public sourcemeta::core::BinaryWriter {
public:
  using Stream = std::basic_ostream<sourcemeta::core::JSON::Char,
                                    sourcemeta::core::JSON::CharTraits>;
  BinPackOutputStream(Stream &output);

  auto put_varint(const std::uint64_t value) -> void;
  auto put_varint_zigzag(const std::int64_t value) -> void;
  auto put_string_utf8(const sourcemeta::core::JSON::String &string,
                       const std::uint64_t length) -> void;
};

} // namespace sourcemeta::blaze

#endif
