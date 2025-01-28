#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_INPUT_STREAM_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_INPUT_STREAM_H_

#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT
#include <sourcemeta/jsonbinpack/runtime_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <cstdint> // std::uint8_t, std::uint16_t, std::uint64_t
#include <istream> // std::basic_istream

namespace sourcemeta::jsonbinpack {

/// @ingroup runtime
class SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT InputStream {
public:
  using Stream = std::basic_istream<sourcemeta::core::JSON::Char,
                                    sourcemeta::core::JSON::CharTraits>;
  InputStream(Stream &input);
  // Prevent copying, as this class is tied to a stream resource
  InputStream(const InputStream &) = delete;
  auto operator=(const InputStream &) -> InputStream & = delete;

  auto position() const noexcept -> std::uint64_t;
  auto seek(const std::uint64_t offset) -> void;
  // Seek backwards given a relative offset
  auto rewind(const std::uint64_t relative_offset, const std::uint64_t position)
      -> std::uint64_t;
  auto get_byte() -> std::uint8_t;
  // A "word" corresponds to two bytes
  // See https://stackoverflow.com/questions/28066462/how-many-bits-is-a-word
  auto get_word() -> std::uint16_t;
  auto get_varint() -> std::uint64_t;
  auto get_varint_zigzag() -> std::int64_t;
  auto has_more_data() const noexcept -> bool;
  auto get_string_utf8(const std::uint64_t length)
      -> sourcemeta::core::JSON::String;

private:
  Stream &stream;
};

} // namespace sourcemeta::jsonbinpack

#endif
