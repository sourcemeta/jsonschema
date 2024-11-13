#ifndef SOURCEMETA_JSONTOOLKIT_JSON_OBJECT_H_
#define SOURCEMETA_JSONTOOLKIT_JSON_OBJECT_H_

#include <functional>       // std::equal_to, std::less
#include <initializer_list> // std::initializer_list

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 12)
#include <map> // std::map
#else
#include <unordered_map> // std::unordered_map
#endif

namespace sourcemeta::jsontoolkit {

/// @ingroup json
template <typename Key, typename Value> class JSONObject {
public:
  // Constructors

  // Older versions of GCC don't allow `std::unordered_map` to incomplete
  // types, and in this case, `Value` is an incomplete type.
  using Container =
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 12)
      std::map<Key, Value, std::less<Key>,
#else
      std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
#endif
               typename Value::template Allocator<
                   std::pair<const typename Value::String, Value>>>;

  JSONObject() : data{} {}
  JSONObject(std::initializer_list<typename Container::value_type> values)
      : data{values} {}

  // Operators
  // We cannot default given that this class references
  // a JSON "value" as an incomplete type

  auto operator<(const JSONObject<Key, Value> &other) const noexcept -> bool {
    // The `std::unordered_map` container, by definition, does not provide
    // ordering. However, we still want some level of ordering to allow
    // arrays of objects to be sorted.

    // First try a size comparison
    if (this->data.size() != other.data.size()) {
      return this->data.size() < other.data.size();
    }

    // Otherwise do value comparison for common properties
    for (const auto &[key, value] : this->data) {
      if (other.data.contains(key) && value < other.data.at(key)) {
        return true;
      }
    }

    return false;
  }

  auto operator<=(const JSONObject<Key, Value> &other) const noexcept -> bool {
    return this->data <= other.data;
  }
  auto operator>(const JSONObject<Key, Value> &other) const noexcept -> bool {
    return this->data > other.data;
  }
  auto operator>=(const JSONObject<Key, Value> &other) const noexcept -> bool {
    return this->data >= other.data;
  }
  auto operator==(const JSONObject<Key, Value> &other) const noexcept -> bool {
    return this->data == other.data;
  }
  auto operator!=(const JSONObject<Key, Value> &other) const noexcept -> bool {
    return this->data != other.data;
  }

  // Member types
  using key_type = typename Container::key_type;
  using mapped_type = typename Container::mapped_type;
  using value_type = typename Container::value_type;
  using size_type = typename Container::size_type;
  using difference_type = typename Container::difference_type;
  using allocator_type = typename Container::allocator_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using pointer = typename Container::pointer;
  using const_pointer = typename Container::const_pointer;
  using iterator = typename Container::iterator;
  using const_iterator = typename Container::const_iterator;

  /// Get a mutable begin iterator on the object
  auto begin() noexcept -> iterator { return this->data.begin(); }
  /// Get a mutable end iterator on the object
  auto end() noexcept -> iterator { return this->data.end(); }
  /// Get a constant begin iterator on the object
  auto begin() const noexcept -> const_iterator { return this->data.begin(); }
  /// Get a constant end iterator on the object
  auto end() const noexcept -> const_iterator { return this->data.end(); }
  /// Get a constant begin iterator on the object
  auto cbegin() const noexcept -> const_iterator { return this->data.cbegin(); }
  /// Get a constant end iterator on the object
  auto cend() const noexcept -> const_iterator { return this->data.cend(); }

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
