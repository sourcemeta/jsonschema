#ifndef SOURCEMETA_CORE_JSON_ARRAY_H_
#define SOURCEMETA_CORE_JSON_ARRAY_H_

#include <initializer_list> // std::initializer_list
#include <vector>           // std::vector

namespace sourcemeta::core {

/// @ingroup json
/// An ordered sequence of JSON values
template <typename Value> class JSONArray {
public:
  // Constructors
  /// The underlying container type that holds the array elements
  using Container =
      std::vector<Value, typename Value::template Allocator<Value>>;
  JSONArray() : data_{} {}
  /// Construct an array from a list of values
  JSONArray(std::initializer_list<Value> values) : data_{values} {}

  // Operators
  // We cannot default given that this class references
  // a JSON "value" as an incomplete type
  auto operator<(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data_ < other.data_;
  }
  auto operator<=(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data_ <= other.data_;
  }
  auto operator>(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data_ > other.data_;
  }
  auto operator>=(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data_ >= other.data_;
  }
  auto operator==(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data_ == other.data_;
  }
  auto operator!=(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data_ != other.data_;
  }

  // Member types
  using value_type = Container::value_type;
  using allocator_type = Container::allocator_type;
  using size_type = Container::size_type;
  using difference_type = Container::difference_type;
  using reference = Container::reference;
  using const_reference = Container::const_reference;
  using pointer = Container::pointer;
  using const_pointer = Container::const_pointer;
  using iterator = Container::iterator;
  using const_iterator = Container::const_iterator;
  using reverse_iterator = Container::reverse_iterator;
  using const_reverse_iterator = Container::const_reverse_iterator;

  /// Get a mutable begin iterator on the array
  auto begin() noexcept -> iterator { return this->data_.begin(); }
  /// Get a mutable end iterator on the array
  auto end() noexcept -> iterator { return this->data_.end(); }
  /// Get a constant begin iterator on the array
  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    return this->data_.begin();
  }
  /// Get a constant end iterator on the array
  [[nodiscard]] auto end() const noexcept -> const_iterator {
    return this->data_.end();
  }
  /// Get a constant begin iterator on the array
  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return this->data_.cbegin();
  }
  /// Get a constant end iterator on the array
  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return this->data_.cend();
  }
  /// Get a mutable reverse begin iterator on the array
  auto rbegin() noexcept -> reverse_iterator { return this->data_.rbegin(); }
  /// Get a mutable reverse end iterator on the array
  auto rend() noexcept -> reverse_iterator { return this->data_.rend(); }
  /// Get a constant reverse begin iterator on the array
  [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
    return this->data_.rbegin();
  }
  /// Get a constant reverse end iterator on the array
  [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
    return this->data_.rend();
  }
  /// Get a constant reverse begin iterator on the array
  [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
    return this->data_.crbegin();
  }
  /// Get a constant reverse end iterator on the array
  [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
    return this->data_.crend();
  }

  /// Get array size
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->data_.size();
  }

  /// Reserve capacity for a given number of elements
  auto reserve(const size_type capacity) -> void {
    this->data_.reserve(capacity);
  }

private:
  friend Value;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  Container data_;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};

} // namespace sourcemeta::core

#endif
