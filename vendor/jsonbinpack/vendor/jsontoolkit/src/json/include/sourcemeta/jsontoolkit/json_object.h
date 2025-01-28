#ifndef SOURCEMETA_JSONTOOLKIT_JSON_OBJECT_H_
#define SOURCEMETA_JSONTOOLKIT_JSON_OBJECT_H_

#include <functional>       // std::equal_to, std::less
#include <initializer_list> // std::initializer_list

#include <sourcemeta/noa/flat_map.h>

namespace sourcemeta::jsontoolkit {

/// @ingroup json
template <typename Key, typename Value, typename Hash> class JSONObject {
public:
  // Constructors
  using Container = sourcemeta::noa::FlatMap<Key, Value, Hash>;

  JSONObject() : data{} {}
  JSONObject(std::initializer_list<typename Container::value_type> values)
      : data{values} {}

  // Operators
  // We cannot default given that this class references
  // a JSON "value" as an incomplete type

  auto operator<(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    // The `std::unordered_map` container, by definition, does not provide
    // ordering. However, we still want some level of ordering to allow
    // arrays of objects to be sorted.

    // First try a size comparison
    if (this->data.size() != other.data.size()) {
      return this->data.size() < other.data.size();
    }

    // Otherwise do value comparison for common properties
    for (const auto &entry : this->data) {
      const auto other_entry{other.find(entry.first)};
      if (other_entry != other.cend() && entry.second < other_entry->second) {
        return true;
      }
    }

    return false;
  }

  auto operator<=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return this->data <= other.data;
  }
  auto operator>(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return this->data > other.data;
  }
  auto operator>=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return this->data >= other.data;
  }
  auto operator==(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return this->data == other.data;
  }
  auto operator!=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return this->data != other.data;
  }

  // Member types
  using key_type = typename Container::key_type;
  using mapped_type = typename Container::mapped_type;
  using value_type = typename Container::Entry;
  using size_type = typename Container::size_type;
  using difference_type = typename Container::difference_type;
  using allocator_type = typename Container::allocator_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using pointer = typename Container::pointer;
  using const_pointer = typename Container::const_pointer;
  using const_iterator = typename Container::const_iterator;

  inline auto begin() const noexcept -> const_iterator {
    return this->data.begin();
  }
  /// Get a constant end iterator on the object
  inline auto end() const noexcept -> const_iterator {
    return this->data.end();
  }
  /// Get a constant begin iterator on the object
  inline auto cbegin() const noexcept -> const_iterator {
    return this->data.cbegin();
  }
  /// Get a constant end iterator on the object
  inline auto cend() const noexcept -> const_iterator {
    return this->data.cend();
  }

  /// Attempt to find an entry by key
  inline auto find(const Key &key) const -> const_iterator {
    return this->data.find(key, this->data.hash(key));
  }

  /// Check if an entry with the given key exists
  inline auto defines(const Key &key,
                      const typename Container::hash_type hash) const -> bool {
    return this->data.contains(key, hash);
  }

  /// Check the size of the object
  inline auto size() const -> std::size_t { return this->data.size(); }

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
