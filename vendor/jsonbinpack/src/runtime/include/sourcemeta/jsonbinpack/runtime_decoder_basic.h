#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_DECODER_BASIC_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_DECODER_BASIC_H_
#ifndef DOXYGEN

#include <sourcemeta/jsonbinpack/numeric.h>

#include <sourcemeta/jsonbinpack/runtime_varint.h>

#include <cassert> // assert
#include <cmath>   // std::ceil
#include <cstdint> // std::uint8_t, std::int64_t, std::uint64_t
#include <ios>     // std::ios_base
#include <istream> // std::basic_istream

namespace sourcemeta::jsonbinpack {

template <typename CharT, typename Traits> class BasicDecoder {
public:
  BasicDecoder(std::basic_istream<CharT, Traits> &input) : stream{input} {
    this->stream.exceptions(std::ios_base::badbit | std::ios_base::failbit |
                            std::ios_base::eofbit);
  }

  // Prevent copying, as this class is tied to a stream resource
  BasicDecoder(const BasicDecoder &) = delete;
  auto operator=(const BasicDecoder &) -> BasicDecoder & = delete;

  inline auto position() const noexcept -> std::uint64_t {
    return static_cast<std::uint64_t>(this->stream.tellg());
  }

  inline auto seek(const std::uint64_t offset) -> void {
    this->stream.seekg(static_cast<std::streamoff>(offset));
  }

  // Seek backwards given a relative offset
  inline auto rewind(const std::uint64_t relative_offset,
                     const std::uint64_t position) -> std::uint64_t {
    assert(position >= relative_offset);
    const std::uint64_t offset{position - relative_offset};
    assert(offset < position);
    const std::uint64_t current{this->position()};
    this->seek(offset);
    return current;
  }

  inline auto get_byte() -> std::uint8_t {
    return static_cast<std::uint8_t>(this->stream.get());
  }

  // A "word" corresponds to two bytes
  // See https://stackoverflow.com/questions/28066462/how-many-bits-is-a-word
  inline auto get_word() -> std::uint16_t {
    std::uint16_t word;
    this->stream.read(reinterpret_cast<char *>(&word), sizeof word);
    return word;
  }

  inline auto get_varint() -> std::uint64_t {
    return varint_decode(this->stream);
  }

  inline auto get_varint_zigzag() -> std::int64_t {
    const std::uint64_t value = varint_decode(this->stream);
    return zigzag_decode(value);
  }

  inline auto has_more_data() const noexcept -> bool {
    // A way to check if the stream is empty without using `.peek()`,
    // which throws given we set exceptions on the EOF bit.
    // However, `in_avail()` works on characters and will return zero
    // if all that's remaining is 0x00 (null), so we need to handle
    // that case separately.
    return this->stream.rdbuf()->in_avail() > 0 ||
           this->stream.rdbuf()->sgetc() == '\0';
  }

  inline auto get_string_utf8(const std::uint64_t length)
      -> std::basic_string<CharT> {
    std::basic_string<CharT> result;
    result.reserve(length);
    std::uint64_t counter = 0;
    while (counter < length) {
      result += static_cast<CharT>(this->get_byte());
      counter += 1;
    }

    assert(counter == length);
    assert(result.size() == length);
    return result;
  }

private:
  std::basic_istream<CharT, Traits> &stream;
};

} // namespace sourcemeta::jsonbinpack

#endif
#endif
