#ifndef SOURCEMETA_CORE_HTML_BUFFER_H_
#define SOURCEMETA_CORE_HTML_BUFFER_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <sourcemeta/core/preprocessor.h>

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstring>     // std::memcpy
#include <iostream>    // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::cmp_less, std::move

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

  /// Reserve capacity for at least a number of bytes in total up front,
  /// keeping the accumulated contents
  SOURCEMETA_FORCEINLINE auto reserve(const std::size_t bytes) -> void {
    if (this->capacity() < bytes) {
      this->reallocate(bytes);
    }
  }

  /// Append a single character to the buffer
  SOURCEMETA_FORCEINLINE auto append(const char character) -> void {
    if (this->cursor_ == this->end_) [[unlikely]] {
      this->grow(1);
    }

    *this->cursor_ = character;
    ++this->cursor_;
  }

  /// Append a sequence of characters to the buffer
  SOURCEMETA_FORCEINLINE auto append(const std::string_view data) -> void {
    const auto length{data.size()};
    if (length == 0) {
      return;
    }

    if (std::cmp_less(this->end_ - this->cursor_, length)) [[unlikely]] {
      this->grow(length);
    }

    std::memcpy(this->cursor_, data.data(), length);
    this->cursor_ += length;
  }

  /// Reserve capacity for at least a number of bytes on top of the accumulated
  /// contents, so that appending up to that many bytes through
  /// `append_unchecked` needs no further capacity checks
  SOURCEMETA_FORCEINLINE auto reserve_additional(const std::size_t bytes)
      -> void {
    if (std::cmp_less(this->end_ - this->cursor_, bytes)) [[unlikely]] {
      this->grow(bytes);
    }
  }

  /// Append a sequence of characters whose capacity was already reserved
  /// through `reserve_additional`
  SOURCEMETA_FORCEINLINE auto
  append_unchecked(const std::string_view data) noexcept -> void {
    assert(std::cmp_less_equal(data.size(), this->end_ - this->cursor_));
    if (!data.empty()) {
      std::memcpy(this->cursor_, data.data(), data.size());
      this->cursor_ += data.size();
    }
  }

  /// Get the number of bytes accumulated so far
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto size() const noexcept
      -> std::size_t {
    return static_cast<std::size_t>(this->cursor_ - this->begin_);
  }

  /// Get the last character of the buffer, which must not be empty
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto back() const noexcept -> char {
    assert(this->cursor_ != this->begin_);
    return *(this->cursor_ - 1);
  }

  /// Remove a number of bytes from the end of the buffer, which must hold at
  /// least that many bytes
  SOURCEMETA_FORCEINLINE auto remove_suffix(const std::size_t count) noexcept
      -> void {
    assert(count <= this->size());
    this->cursor_ -= count;
  }

  /// Get a view of the accumulated contents, which stays valid until the
  /// buffer is modified
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto view() const noexcept
      -> std::string_view {
    return {this->begin_, this->size()};
  }

  /// Discard the accumulated contents, keeping the capacity for reuse
  SOURCEMETA_FORCEINLINE auto clear() noexcept -> void {
    this->cursor_ = this->begin_;
  }

  /// Get the accumulated contents of the buffer
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto str() -> const std::string & {
    this->buffer_.resize(this->size());
    this->begin_ = this->buffer_.data();
    this->cursor_ = this->begin_ + this->buffer_.size();
    this->end_ = this->cursor_;
    return this->buffer_;
  }

  /// Move the accumulated contents out of the buffer, leaving it empty and
  /// ready to accumulate new contents
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto take() -> std::string {
    this->buffer_.resize(this->size());
    std::string result{std::move(this->buffer_)};
    this->buffer_.clear();
    this->begin_ = nullptr;
    this->cursor_ = nullptr;
    this->end_ = nullptr;
    return result;
  }

  /// Write the accumulated contents to an output stream
  auto write(std::ostream &stream) -> void;

private:
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto capacity() const noexcept
      -> std::size_t {
    return static_cast<std::size_t>(this->end_ - this->begin_);
  }

  auto grow(std::size_t needed) -> void;
  auto reallocate(std::size_t capacity) -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  std::string buffer_;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  char *begin_{nullptr};
  char *cursor_{nullptr};
  char *end_{nullptr};
};

} // namespace sourcemeta::core

#endif
