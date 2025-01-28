#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_OUTPUT_STREAM_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_OUTPUT_STREAM_H_

#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT
#include <sourcemeta/jsonbinpack/runtime_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <cstdint> // std::uint8_t, std::uint16_t, std::uint64_t
#include <ostream> // std::basic_ostream

namespace sourcemeta::jsonbinpack {

/// @ingroup runtime
class SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT OutputStream {
public:
  using Stream = std::basic_ostream<sourcemeta::core::JSON::Char,
                                    sourcemeta::core::JSON::CharTraits>;
  OutputStream(Stream &output);

  // Prevent copying, as this class is tied to a stream resource
  OutputStream(const OutputStream &) = delete;
  auto operator=(const OutputStream &) -> OutputStream & = delete;

  auto position() const noexcept -> std::uint64_t;
  auto put_byte(const std::uint8_t byte) -> void;
  auto put_bytes(const std::uint16_t bytes) -> void;
  auto put_varint(const std::uint64_t value) -> void;
  auto put_varint_zigzag(const std::int64_t value) -> void;
  auto put_string_utf8(const sourcemeta::core::JSON::String &string,
                       const std::uint64_t length) -> void;

private:
  Stream &stream;
};

} // namespace sourcemeta::jsonbinpack

#endif
