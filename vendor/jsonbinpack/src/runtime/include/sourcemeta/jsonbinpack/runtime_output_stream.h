#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_OUTPUT_STREAM_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_OUTPUT_STREAM_H_

#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT
#include <sourcemeta/jsonbinpack/runtime_export.h>
#endif

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>

#include <cstdint> // std::uint64_t, std::int64_t
#include <ostream> // std::basic_ostream

namespace sourcemeta::jsonbinpack {

/// @ingroup runtime
class SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT OutputStream
    : public sourcemeta::core::BinaryWriter {
public:
  using Stream = std::basic_ostream<sourcemeta::core::JSON::Char,
                                    sourcemeta::core::JSON::CharTraits>;
  OutputStream(Stream &output);

  auto put_varint(const std::uint64_t value) -> void;
  auto put_varint_zigzag(const std::int64_t value) -> void;
  auto put_string_utf8(const sourcemeta::core::JSON::String &string,
                       const std::uint64_t length) -> void;
};

} // namespace sourcemeta::jsonbinpack

#endif
