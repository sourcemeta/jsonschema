#ifndef SOURCEMETA_CORE_JSON_OBJECT_H_
#define SOURCEMETA_CORE_JSON_OBJECT_H_

#include <cassert>          // assert
#include <cstddef>          // std::size_t
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::advance
#include <utility>          // std::pair, std::move
#include <vector>           // std::vector

namespace sourcemeta::core {

/// @ingroup json
template <typename Key, typename Value, typename Hash> class JSONObject {
public:
  JSONObject() = default;

  using key_type = Key;
  using mapped_type = Value;
  using hash_type = typename Hash::hash_type;
  using pair_value_type = std::pair<key_type, mapped_type>;

  JSONObject(std::initializer_list<pair_value_type> entries) : data{} {
    this->data.reserve(entries.size());
    for (auto &&entry : entries) {
      this->emplace(std::move(entry.first), std::move(entry.second));
    }
  }

  struct Entry {
    key_type first;
    mapped_type second;
    hash_type hash;
  };

  using underlying_type = std::vector<Entry>;
  using value_type = typename underlying_type::value_type;
  using size_type = typename underlying_type::size_type;
  using difference_type = typename underlying_type::difference_type;
  using allocator_type = typename underlying_type::allocator_type;
  using reference = typename underlying_type::reference;
  using const_reference = typename underlying_type::const_reference;
  using pointer = typename underlying_type::pointer;
  using const_pointer = typename underlying_type::const_pointer;
  using const_iterator = typename underlying_type::const_iterator;

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
    for (const auto &entry : *this) {
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
    if (this->size() != other.size()) {
      return false;
    }

    for (const auto &entry : this->data) {
      const auto *result{other.try_at(entry.first, entry.hash)};
      if (!result || *result != entry.second) {
        return false;
      }
    }

    return true;
  }

  auto operator!=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool = default;

  [[nodiscard]] inline auto begin() const noexcept -> const_iterator {
    return this->data.begin();
  }
  /// Get a constant end iterator on the object
  [[nodiscard]] inline auto end() const noexcept -> const_iterator {
    return this->data.end();
  }
  /// Get a constant begin iterator on the object
  [[nodiscard]] inline auto cbegin() const noexcept -> const_iterator {
    return this->data.cbegin();
  }
  /// Get a constant end iterator on the object
  [[nodiscard]] inline auto cend() const noexcept -> const_iterator {
    return this->data.cend();
  }

  /// Compute a hash for a key
  [[nodiscard]] inline auto hash(const Key &key) const noexcept -> hash_type {
    return this->hasher(key);
  }

  /// Attempt to find an entry by key
  [[nodiscard]] inline auto find(const Key &key) const -> const_iterator {
    const auto key_hash{this->hash(key)};
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data[index].hash == key_hash) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    } else {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data[index].hash == key_hash &&
            this->data[index].first == key) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    }

    return this->cend();
  }

  /// Check if an entry with the given key exists
  [[nodiscard]] inline auto defines(const Key &key, const hash_type hash) const
      -> bool {
    assert(this->hash(key) == hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(hash)) {
      for (const auto &entry : *this) {
        if (entry.hash == hash) {
          return true;
        }
      }
    } else {
      for (const auto &entry : *this) {
        if (entry.hash == hash && entry.first == key) {
          return true;
        }
      }
    }

    return false;
  }

  /// Check the size of the object
  [[nodiscard]] inline auto size() const -> std::size_t {
    return this->data.size();
  }

  /// Check if the object is empty
  [[nodiscard]] inline auto empty() const -> bool { return this->data.empty(); }

  /// Access an object entry by its underlying positional index
  [[nodiscard]] inline auto at(const size_type index) const -> const Entry & {
    return this->data.at(index);
  }

  /// Access an object entry by its key name
  [[nodiscard]] inline auto at(const Key &key, const hash_type key_hash) const
      -> const mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (const auto &entry : *this) {
        if (entry.hash == key_hash) {
          return entry.second;
        }
      }
    } else {
      for (const auto &entry : *this) {
        if (entry.hash == key_hash && entry.first == key) {
          return entry.second;
        }
      }
    }

// See https://en.cppreference.com/w/cpp/utility/unreachable
#if defined(_MSC_VER) && !defined(__clang__)
    __assume(false);
#else
    __builtin_unreachable();
#endif
  }

  /// Access an object entry by its key name
  inline auto at(const Key &key, const hash_type key_hash) -> mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash) {
          return entry.second;
        }
      }
    } else {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash && entry.first == key) {
          return entry.second;
        }
      }
    }

// See https://en.cppreference.com/w/cpp/utility/unreachable
#if defined(_MSC_VER) && !defined(__clang__)
    __assume(false);
#else
    __builtin_unreachable();
#endif
  }

  /// Try to access an object entry by its underlying positional index
  [[nodiscard]] inline auto try_at(const Key &key,
                                   const hash_type key_hash) const
      -> const mapped_type * {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data[index].hash == key_hash) {
          return &this->data[index].second;
        }
      }
    } else {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data[index].hash == key_hash &&
            this->data[index].first == key) {
          return &this->data[index].second;
        }
      }
    }

    return nullptr;
  }

  /// Try to emplace a property before another property
  auto try_emplace_before(const Key &key, const mapped_type &value,
                          const Key &suffix) -> hash_type {
    const auto key_hash{this->hash(key)};
    const auto suffix_hash{this->hash(suffix)};

    if (this->hasher.is_perfect(key_hash)) {
      for (auto iterator = this->data.begin(); iterator != this->data.end();
           ++iterator) {
        if (iterator->hash == key_hash) {
          iterator->second = value;
          return key_hash;
        } else if (iterator->hash == suffix_hash && iterator->first == suffix) {
          this->data.insert(iterator, {key, value, key_hash});
          return key_hash;
        }
      }
    } else {
      for (auto iterator = this->data.begin(); iterator != this->data.end();
           ++iterator) {
        if (iterator->hash == key_hash && iterator->first == key) {
          iterator->second = value;
          return key_hash;
        } else if (iterator->hash == suffix_hash && iterator->first == suffix) {
          this->data.insert(iterator, {key, value, key_hash});
          return key_hash;
        }
      }
    }

    this->data.push_back({key, value, key_hash});
    return key_hash;
  }

  /// Emplace an object property
  inline auto emplace(Key &&key, mapped_type &&value) -> hash_type {
    const auto key_hash{this->hash(key)};

    if (this->hasher.is_perfect(key_hash)) {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash) {
          entry.second = std::move(value);
          return key_hash;
        }
      }
    } else {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.second = std::move(value);
          return key_hash;
        }
      }
    }

    this->data.push_back({key, value, key_hash});
    return key_hash;
  }

  /// Emplace an object property
  inline auto emplace(const Key &key, const mapped_type &value) -> hash_type {
    const auto key_hash{this->hash(key)};

    if (this->hasher.is_perfect(key_hash)) {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash) {
          entry.second = value;
          return key_hash;
        }
      }
    } else {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.second = value;
          return key_hash;
        }
      }
    }

    this->data.push_back({key, value, key_hash});
    return key_hash;
  }

  /// Remove every property in the object
  inline auto clear() noexcept -> void { this->data.clear(); }

  /// Rename an object property in place
  auto rename(const Key &key, const hash_type key_hash, Key &&to,
              const hash_type to_hash) -> void {
    this->erase(to, to_hash);

    if (this->hasher.is_perfect(key_hash)) {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash) {
          entry.first = std::move(to);
          entry.hash = to_hash;
          break;
        }
      }
    } else {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.first = std::move(to);
          entry.hash = to_hash;
          break;
        }
      }
    }
  }

  /// Erase an object property
  auto erase(const Key &key, const hash_type key_hash) -> size_type {
    const auto current_size{this->size()};

    if (this->hasher.is_perfect(key_hash)) {
      for (auto iterator = this->data.begin(); iterator != this->data.end();
           ++iterator) {
        if (iterator->hash == key_hash) {
          this->data.erase(iterator);
          return current_size - 1;
        }
      }
    } else {
      for (auto iterator = this->data.begin(); iterator != this->data.end();
           ++iterator) {
        if (iterator->hash == key_hash && iterator->first == key) {
          this->data.erase(iterator);
          return current_size - 1;
        }
      }
    }

    return current_size;
  }

  /// Erase an object property
  inline auto erase(const Key &key) -> size_type {
    return this->erase(key, this->hash(key));
  }

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  Hash hasher;
  underlying_type data;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
