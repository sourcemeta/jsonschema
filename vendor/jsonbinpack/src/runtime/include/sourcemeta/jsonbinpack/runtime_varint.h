#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_VARINT_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_VARINT_H_

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint64_t
#include <istream> // std::basic_istream
#include <ostream> // std::basic_ostream

namespace sourcemeta::jsonbinpack {

// This encoder does not handle negative integers. Use ZigZag first instead.
template <typename CharT, typename Traits>
auto varint_encode(std::basic_ostream<CharT, Traits> &stream,
                   const std::uint64_t value)
    -> std::basic_ostream<CharT, Traits> & {
  constexpr std::uint8_t LEAST_SIGNIFICANT_BITS{0b01111111};
  constexpr std::uint8_t MOST_SIGNIFICANT_BIT{0b10000000};
  constexpr std::uint8_t SHIFT{7};
  std::uint64_t accumulator = value;

  while (accumulator > LEAST_SIGNIFICANT_BITS) {
    stream.put(static_cast<CharT>((accumulator & LEAST_SIGNIFICANT_BITS) |
                                  MOST_SIGNIFICANT_BIT));
    accumulator >>= SHIFT;
  }

  stream.put(static_cast<CharT>(accumulator));
  return stream;
}

template <typename CharT, typename Traits>
auto varint_decode(std::basic_istream<CharT, Traits> &stream) -> std::uint64_t {
  constexpr std::uint8_t LEAST_SIGNIFICANT_BITS{0b01111111};
  constexpr std::uint8_t MOST_SIGNIFICANT_BIT{0b10000000};
  constexpr std::uint8_t SHIFT{7};
  std::uint64_t result{0};
  std::size_t cursor{0};
  while (true) {
    const std::uint8_t byte{static_cast<std::uint8_t>(stream.get())};
    assert(!stream.eof());
    const std::uint64_t value{
        static_cast<std::uint64_t>(byte & LEAST_SIGNIFICANT_BITS)};
#ifndef NDEBUG
    const std::uint64_t current = result;
#endif
    result += static_cast<std::uint64_t>(value << SHIFT * cursor);
    // Try to catch potential overflows from the above addition
    assert(result >= current);
    cursor += 1;
    if ((byte & MOST_SIGNIFICANT_BIT) == 0) {
      break;
    }
  }

  return result;
}

} // namespace sourcemeta::jsonbinpack

#endif
