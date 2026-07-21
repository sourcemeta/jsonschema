#ifndef SOURCEMETA_CORE_CRYPTO_SECURE_H_
#define SOURCEMETA_CORE_CRYPTO_SECURE_H_

#ifndef SOURCEMETA_CORE_CRYPTO_EXPORT
#include <sourcemeta/core/crypto_export.h>
#endif

#include <cstddef>     // std::size_t
#include <functional>  // std::less
#include <limits>      // std::numeric_limits
#include <new>         // operator new, operator delete, std::align_val_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

/// @ingroup crypto
/// Overwrite a buffer that held secret material with zeroes, so it does not
/// linger in memory after it is no longer needed. The write goes through a
/// volatile access, so the compiler does not elide it as a dead store. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <array>
/// #include <cstdint>
///
/// std::array<std::uint8_t, 4> buffer{{1, 2, 3, 4}};
/// sourcemeta::core::secure_zero(buffer.data(), buffer.size());
/// ```
inline auto secure_zero(void *const data, const std::size_t size) noexcept
    -> void {
  if (data == nullptr) {
    return;
  }

  auto *pointer{static_cast<volatile unsigned char *>(data)};
  for (std::size_t index{0}; index < size; index += 1) {
    pointer[index] = 0;
  }
}

/// @ingroup crypto
/// Overwrite the storage a string owns with zeroes, so a secret it held does
/// not linger in memory. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
/// #include <string>
///
/// std::string secret{"hunter2"};
/// sourcemeta::core::secure_zero(secret);
/// assert(secret == std::string(7, '\x00'));
/// ```
inline auto secure_zero(std::string &value) noexcept -> void {
  secure_zero(value.data(), value.size());
}

/// @ingroup crypto
/// Overwrite the referenced string when leaving the current scope, so secret
/// material a local holds is wiped across every return path without threading
/// a manual call through each one. It clears only the live bytes the string
/// owns at scope exit, so a reassignment, an in-place shrink, or a growth that
/// reallocates before then can still leave earlier bytes in freed memory or in
/// the capacity beyond the final length, a residual that only a wiping
/// allocator closes. Prefer a wiping string for secrets that change size. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <cassert>
/// #include <string>
///
/// std::string secret{"hunter2"};
/// {
///   const sourcemeta::core::SecureStringScope scope{secret};
/// }
/// assert(secret == std::string(7, '\x00'));
/// ```
struct SecureStringScope {
  /// Capture the string to wipe when leaving the current scope.
  explicit SecureStringScope(std::string &value) noexcept : target{value} {}
  SecureStringScope(const SecureStringScope &) = delete;
  auto operator=(const SecureStringScope &) -> SecureStringScope & = delete;
  SecureStringScope(SecureStringScope &&) = delete;
  auto operator=(SecureStringScope &&) -> SecureStringScope & = delete;
  ~SecureStringScope() { secure_zero(this->target); }
  /// The captured string, whose storage is wiped at scope exit.
  std::string &target;
};

/// @ingroup crypto
/// A standard allocator that wipes every block it owns before releasing it, so
/// a secret the storage held does not survive in freed memory. The deallocation
/// covers the whole block, so a reallocation, a reassignment, or the
/// destruction of the owner all clear the earlier bytes, closing the residue
/// that the scope-based cleanup above cannot reach on its own. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
/// #include <vector>
///
/// std::vector<char, sourcemeta::core::SecureAllocator<char>> secret;
/// secret.push_back('x');
/// ```
template <typename T> struct SecureAllocator {
  /// The type of object this allocator hands out storage for.
  using value_type = T;

  /// Construct an allocator, which holds no state of its own.
  SecureAllocator() noexcept = default;

  /// Construct from an allocator for another type, as the containers require.
  template <typename Other>
  constexpr SecureAllocator(const SecureAllocator<Other> &) noexcept {}

  /// Allocate storage for the given number of objects.
  [[nodiscard]] auto allocate(const std::size_t count) -> T * {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      throw std::bad_array_new_length{};
    }

    return static_cast<T *>(
        ::operator new(count * sizeof(T), std::align_val_t{alignof(T)}));
  }

  /// Wipe and release the storage of the given number of objects.
  auto deallocate(T *const pointer, const std::size_t count) noexcept -> void {
    secure_zero(pointer, count * sizeof(T));
    ::operator delete(pointer, std::align_val_t{alignof(T)});
  }

  /// Every instance is interchangeable, since the allocator holds no state.
  template <typename Other>
  auto operator==(const SecureAllocator<Other> &) const noexcept -> bool {
    return true;
  }

  /// Every instance is interchangeable, since the allocator holds no state.
  template <typename Other>
  auto operator!=(const SecureAllocator<Other> &) const noexcept -> bool {
    return false;
  }
};

/// @ingroup crypto
/// A string whose bytes always live in heap storage that is wiped whenever it
/// is released, so a secret it holds never reaches freed memory when the string
/// reallocates on growth, is reassigned, or is destroyed, and never lingers in
/// an inline buffer. Bytes abandoned by an in-place shrink stay in the still
/// owned block only until that block is next reallocated or freed, when they
/// too are wiped. For example:
///
/// ```cpp
/// #include <sourcemeta/core/crypto.h>
///
/// sourcemeta::core::SecureString secret{"hunter2"};
/// secret.append("!");
/// ```
class SecureString {
public:
  /// The type that counts and indexes the held bytes.
  using size_type = std::vector<char, SecureAllocator<char>>::size_type;

  /// Construct an empty string.
  SecureString() = default;

  /// Construct from a view of bytes.
  SecureString(const std::string_view value)
      : buffer_(value.begin(), value.end()) {}

  /// Construct from a pointer and a length.
  SecureString(const char *const data, const size_type length)
      : buffer_(data, data + length) {}

  /// Construct a run of a repeated byte.
  SecureString(const size_type count, const char value)
      : buffer_(count, value) {}

  /// The number of bytes held.
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->buffer_.size();
  }

  /// Whether no bytes are held.
  [[nodiscard]] auto empty() const noexcept -> bool {
    return this->buffer_.empty();
  }

  /// Reserve storage for at least the given number of bytes.
  auto reserve(const size_type capacity) -> void {
    this->buffer_.reserve(capacity);
  }

  /// Resize to the given number of bytes, padding new ones with the given
  /// value.
  auto resize(const size_type count, const char value) -> void {
    this->buffer_.resize(count, value);
  }

  /// Append a single byte.
  auto push_back(const char value) -> void { this->buffer_.push_back(value); }

  /// Append a view of bytes.
  auto append(const std::string_view value) -> void {
    // Inserting a range whose iterators point into this container is undefined,
    // so a view that aliases the storage is taken through an independent buffer
    // that wipes itself, while an independent view is inserted directly. The
    // ordering uses the total order over pointers, which is defined even for
    // pointers into different objects
    const char *const first{this->buffer_.data()};
    const std::less<const char *> before{};
    if (!this->buffer_.empty() && !before(value.data(), first) &&
        before(value.data(), first + this->buffer_.size())) {
      const std::vector<char, SecureAllocator<char>> copy(value.begin(),
                                                          value.end());
      this->buffer_.insert(this->buffer_.end(), copy.begin(), copy.end());
    } else {
      this->buffer_.insert(this->buffer_.end(), value.begin(), value.end());
    }
  }

  /// Append a run of a repeated byte.
  auto append(const size_type count, const char value) -> void {
    this->buffer_.insert(this->buffer_.end(), count, value);
  }

  /// A reference to the byte at the given index.
  [[nodiscard]] auto operator[](const size_type index) noexcept -> char & {
    return this->buffer_[index];
  }

  /// The byte at the given index.
  [[nodiscard]] auto operator[](const size_type index) const noexcept -> char {
    return this->buffer_[index];
  }

  /// The first byte.
  [[nodiscard]] auto front() const noexcept -> char {
    return this->buffer_.front();
  }

  /// The last byte.
  [[nodiscard]] auto back() const noexcept -> char {
    return this->buffer_.back();
  }

  /// A view over the held bytes.
  [[nodiscard]] operator std::string_view() const noexcept {
    return {this->buffer_.data(), this->buffer_.size()};
  }

  /// Whether the held bytes equal the given view.
  [[nodiscard]] auto operator==(const std::string_view other) const noexcept
      -> bool {
    return std::string_view{*this} == other;
  }

private:
  std::vector<char, SecureAllocator<char>> buffer_;
};

} // namespace sourcemeta::core

#endif
