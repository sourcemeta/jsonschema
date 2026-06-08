#ifndef SOURCEMETA_CORE_IO_BYTESTREAM_H_
#define SOURCEMETA_CORE_IO_BYTESTREAM_H_

#ifndef SOURCEMETA_CORE_IO_EXPORT
#include <sourcemeta/core/io_export.h>
#endif

#include <cstddef>          // std::byte
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <sstream>          // std::istringstream, std::ostringstream
#include <vector>           // std::vector

namespace sourcemeta::core {

// Exporting individual members rather than the whole class avoids MSVC
// instantiating std::basic_istringstream / std::basic_ostringstream vbase
// destructors inside this DLL, which would otherwise clash at link time with
// consumer translation units that use these iostream types directly
// (LNK2005).
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup io
/// An input stream constructed from an inline list of byte values. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
///
/// sourcemeta::core::InputByteStream stream{0x1f, 0x8b, 0x08, 0x00};
/// ```
class InputByteStream : public std::istringstream {
public:
  SOURCEMETA_CORE_IO_EXPORT
  InputByteStream(std::initializer_list<std::uint8_t> bytes);
};

/// @ingroup io
/// An output stream that exposes its accumulated bytes as a byte vector. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/io.h>
/// #include <cassert>
///
/// sourcemeta::core::OutputByteStream stream;
/// stream.put('A');
/// assert(stream.bytes().size() == 1);
/// ```
class OutputByteStream : public std::ostringstream {
public:
  SOURCEMETA_CORE_IO_EXPORT
  auto bytes() const -> std::vector<std::byte>;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
