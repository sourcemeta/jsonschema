#ifndef SOURCEMETA_JSONTOOLKIT_JSON_ARRAY_H_
#define SOURCEMETA_JSONTOOLKIT_JSON_ARRAY_H_

#include <initializer_list> // std::initializer_list
#include <vector>           // std::vector

namespace sourcemeta::jsontoolkit {

/// @ingroup json
template <typename Value> class JSONArray {
public:
  // Constructors
  using Container =
      std::vector<Value, typename Value::template Allocator<Value>>;
  JSONArray() : data{} {}
  JSONArray(std::initializer_list<Value> values) : data{values} {}

  // Operators
  // We cannot default given that this class references
  // a JSON "value" as an incomplete type
  auto operator<(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data < other.data;
  }
  auto operator<=(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data <= other.data;
  }
  auto operator>(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data > other.data;
  }
  auto operator>=(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data >= other.data;
  }
  auto operator==(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data == other.data;
  }
  auto operator!=(const JSONArray<Value> &other) const noexcept -> bool {
    return this->data != other.data;
  }

  // Member types
  using value_type = typename Container::value_type;
  using allocator_type = typename Container::allocator_type;
  using size_type = typename Container::size_type;
  using difference_type = typename Container::difference_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using pointer = typename Container::pointer;
  using const_pointer = typename Container::const_pointer;
  using iterator = typename Container::iterator;
  using const_iterator = typename Container::const_iterator;
  using reverse_iterator = typename Container::reverse_iterator;
  using const_reverse_iterator = typename Container::const_reverse_iterator;

  /// Get a mutable begin iterator on the array
  auto begin() noexcept -> iterator { return this->data.begin(); }
  /// Get a mutable end iterator on the array
  auto end() noexcept -> iterator { return this->data.end(); }
  /// Get a constant begin iterator on the array
  auto begin() const noexcept -> const_iterator { return this->data.begin(); }
  /// Get a constant end iterator on the array
  auto end() const noexcept -> const_iterator { return this->data.end(); }
  /// Get a constant begin iterator on the array
  auto cbegin() const noexcept -> const_iterator { return this->data.cbegin(); }
  /// Get a constant end iterator on the array
  auto cend() const noexcept -> const_iterator { return this->data.cend(); }
  /// Get a mutable reverse begin iterator on the array
  auto rbegin() noexcept -> reverse_iterator { return this->data.rbegin(); }
  /// Get a mutable reverse end iterator on the array
  auto rend() noexcept -> reverse_iterator { return this->data.rend(); }
  /// Get a constant reverse begin iterator on the array
  auto rbegin() const noexcept -> const_reverse_iterator {
    return this->data.rbegin();
  }
  /// Get a constant reverse end iterator on the array
  auto rend() const noexcept -> const_reverse_iterator {
    return this->data.rend();
  }
  /// Get a constant reverse begin iterator on the array
  auto crbegin() const noexcept -> const_reverse_iterator {
    return this->data.crbegin();
  }
  /// Get a constant reverse end iterator on the array
  auto crend() const noexcept -> const_reverse_iterator {
    return this->data.crend();
  }

private:
  friend Value;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  Container data;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::jsontoolkit

#endif
