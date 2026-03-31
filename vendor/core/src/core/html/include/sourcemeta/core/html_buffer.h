#ifndef SOURCEMETA_CORE_HTML_BUFFER_H_
#define SOURCEMETA_CORE_HTML_BUFFER_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <sourcemeta/core/preprocessor.h>

#include <cstring>     // std::memcpy
#include <iostream>    // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup html
/// A fast append-only string buffer
class SOURCEMETA_CORE_HTML_EXPORT HTMLBuffer {
public:
  HTMLBuffer() = default;
  HTMLBuffer(const HTMLBuffer &) = delete;
  auto operator=(const HTMLBuffer &) -> HTMLBuffer & = delete;
  HTMLBuffer(HTMLBuffer &&) = delete;
  auto operator=(HTMLBuffer &&) -> HTMLBuffer & = delete;

  SOURCEMETA_FORCEINLINE inline auto reserve(const std::size_t bytes) -> void {
    this->buffer_.resize(bytes);
    this->cursor_ = this->buffer_.data();
    this->end_ = this->cursor_ + bytes;
  }

  SOURCEMETA_FORCEINLINE inline auto append(const char character) -> void {
    if (!this->cursor_ || this->cursor_ >= this->end_) [[unlikely]] {
      this->grow(1);
    }

    *this->cursor_ = character;
    ++this->cursor_;
  }

  SOURCEMETA_FORCEINLINE inline auto append(const std::string_view data)
      -> void {
    const auto length{data.size()};
    if (length == 0) {
      return;
    }

    const auto remaining{
        this->cursor_ ? static_cast<std::size_t>(this->end_ - this->cursor_)
                      : 0uz};
    if (remaining < length) [[unlikely]] {
      this->grow(length);
    }

    std::memcpy(this->cursor_, data.data(), length);
    this->cursor_ += length;
  }

  [[nodiscard]] SOURCEMETA_FORCEINLINE inline auto str()
      -> const std::string & {
    if (this->cursor_) {
      this->buffer_.resize(
          static_cast<std::size_t>(this->cursor_ - this->buffer_.data()));
      this->cursor_ = nullptr;
      this->end_ = nullptr;
    }

    return this->buffer_;
  }

  auto write(std::ostream &stream) -> void;

private:
  auto grow(std::size_t needed) -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::string buffer_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
  char *cursor_{nullptr};
  char *end_{nullptr};
};

} // namespace sourcemeta::core

#endif
