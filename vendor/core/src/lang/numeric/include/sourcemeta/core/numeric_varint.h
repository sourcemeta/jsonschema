#ifndef SOURCEMETA_CORE_NUMERIC_VARINT_H_
#define SOURCEMETA_CORE_NUMERIC_VARINT_H_

#include <sourcemeta/core/numeric_error.h>

#include <cstddef> // std::byte, std::size_t
#include <cstdint> // std::uint8_t, std::uint64_t
#include <utility> // std::unreachable

namespace sourcemeta::core {

/// @ingroup numeric
/// The number of bytes that the largest variable-length integer occupies
inline constexpr std::size_t VARINT_MAXIMUM_BYTES{10};

/// @ingroup numeric
/// Encode an unsigned integer as a variable-length integer, returning the
/// number of bytes written to the destination, which must have room for the
/// maximum size. For example:
///
/// ```cpp
/// #include <sourcemeta/core/numeric.h>
///
/// #include <array>
/// #include <cassert>
/// #include <cstddef>
///
/// std::array<std::byte, sourcemeta::core::VARINT_MAXIMUM_BYTES> buffer{};
/// const auto size{sourcemeta::core::varint_encode(300, buffer.data())};
/// assert(size == 2);
/// assert(buffer[0] == std::byte{0xAC});
/// assert(buffer[1] == std::byte{0x02});
/// ```
inline auto varint_encode(const std::uint64_t value,
                          std::byte *const destination) -> std::size_t {
  // DWARF Debugging Information Format v5, Section 7.6 "Variable Length Data"
  constexpr std::uint8_t PAYLOAD_MASK{0b01111111};
  constexpr std::uint8_t CONTINUATION_BIT{0b10000000};
  constexpr std::uint8_t PAYLOAD_BITS{7};
  std::uint64_t accumulator{value};
  for (std::size_t index = 0; index < VARINT_MAXIMUM_BYTES; index++) {
    if (accumulator <= PAYLOAD_MASK) {
      destination[index] = static_cast<std::byte>(accumulator);
      return index + 1;
    }

    destination[index] = static_cast<std::byte>(
        static_cast<std::uint8_t>(accumulator & PAYLOAD_MASK) |
        CONTINUATION_BIT);
    accumulator >>= PAYLOAD_BITS;
  }

  std::unreachable();
}

/// @ingroup numeric
/// Decode the given bytes, which must hold exactly one variable-length
/// integer, back into an unsigned integer. For example:
///
/// ```cpp
/// #include <sourcemeta/core/numeric.h>
///
/// #include <array>
/// #include <cassert>
/// #include <cstddef>
///
/// const std::array<std::byte, 2> bytes{{std::byte{0xAC}, std::byte{0x02}}};
/// assert(sourcemeta::core::varint_decode(bytes.data(), bytes.size()) == 300);
/// ```
inline auto varint_decode(const std::byte *const source, const std::size_t size)
    -> std::uint64_t {
  // DWARF Debugging Information Format v5, Section 7.6 "Variable Length Data"
  constexpr std::uint8_t PAYLOAD_MASK{0b01111111};
  constexpr std::uint8_t CONTINUATION_BIT{0b10000000};
  constexpr std::uint8_t PAYLOAD_BITS{7};
  if (size == 0 || size > VARINT_MAXIMUM_BYTES) {
    throw VarintParseError{};
  }

  const auto last{static_cast<std::uint8_t>(source[size - 1])};
  if ((last & CONTINUATION_BIT) != 0) {
    throw VarintParseError{};
  }

  // The last byte of the longest possible encoding only has room for the
  // highest bit of the result
  if (size == VARINT_MAXIMUM_BYTES && last > 1) {
    throw NumericOutOfRangeError{};
  }

  std::uint64_t result{0};
  for (std::size_t index = 0; index < size; index++) {
    const auto byte{static_cast<std::uint8_t>(source[index])};
    if (index + 1 != size && (byte & CONTINUATION_BIT) == 0) {
      throw VarintParseError{};
    }

    result |= static_cast<std::uint64_t>(byte & PAYLOAD_MASK)
              << (PAYLOAD_BITS * index);
  }

  return result;
}

} // namespace sourcemeta::core

#endif
