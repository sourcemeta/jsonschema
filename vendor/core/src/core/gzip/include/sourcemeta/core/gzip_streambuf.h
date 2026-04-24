#ifndef SOURCEMETA_CORE_GZIP_STREAMBUF_H_
#define SOURCEMETA_CORE_GZIP_STREAMBUF_H_

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

#include <istream>   // std::istream
#include <memory>    // std::unique_ptr
#include <streambuf> // std::streambuf

namespace sourcemeta::core {

/// @ingroup gzip
/// A stream buffer that performs streaming GZIP decompression (RFC 1952) over
/// an input stream.
class SOURCEMETA_CORE_GZIP_EXPORT GZIPStreamBuffer : public std::streambuf {
public:
  GZIPStreamBuffer(std::istream &compressed_stream);
  ~GZIPStreamBuffer() override;

  GZIPStreamBuffer(const GZIPStreamBuffer &) = delete;
  auto operator=(const GZIPStreamBuffer &) -> GZIPStreamBuffer & = delete;
  GZIPStreamBuffer(GZIPStreamBuffer &&) = delete;
  auto operator=(GZIPStreamBuffer &&) -> GZIPStreamBuffer & = delete;

protected:
  auto underflow() -> int_type override;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  struct Internal;
  std::unique_ptr<Internal> internal;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
