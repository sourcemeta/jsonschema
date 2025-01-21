#ifndef SOURCEMETA_NOA_FLAT_MAP_H_
#define SOURCEMETA_NOA_FLAT_MAP_H_

#include <algorithm>        // std::swap
#include <cassert>          // assert
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::advance
#include <utility>          // std::pair, std::move
#include <vector>           // std::vector

/// @defgroup flat_map Flat Map

namespace sourcemeta::noa {

// TODO: Allow passing custom allocators
/// @ingroup flat_map
template <typename Key, typename Value, typename Hash> class FlatMap {
public:
  FlatMap() = default;

  using key_type = Key;
  using mapped_type = Value;
  using hash_type = typename Hash::hash_type;
  using value_type = std::pair<key_type, mapped_type>;

  struct Entry {
    key_type first;
    mapped_type second;
    hash_type hash;
  };

  using underlying_type = std::vector<Entry>;
  using size_type = typename underlying_type::size_type;
  using difference_type = typename underlying_type::difference_type;
  using allocator_type = typename underlying_type::allocator_type;
  using reference = typename underlying_type::reference;
  using const_reference = typename underlying_type::const_reference;
  using pointer = typename underlying_type::pointer;
  using const_pointer = typename underlying_type::const_pointer;
  using const_iterator = typename underlying_type::const_iterator;

  FlatMap(std::initializer_list<value_type> entries) {
    this->data.reserve(entries.size());
    for (auto &&entry : entries) {
      this->assign(std::move(entry.first), std::move(entry.second));
    }
  }

  auto begin() const noexcept -> const_iterator { return this->data.begin(); }
  auto end() const noexcept -> const_iterator { return this->data.end(); }
  auto cbegin() const noexcept -> const_iterator { return this->data.cbegin(); }
  auto cend() const noexcept -> const_iterator { return this->data.cend(); }

  inline auto hash(const key_type &key) const noexcept -> hash_type {
    return this->hasher(key);
  }

  auto assign(key_type &&key, mapped_type &&value) -> hash_type {
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

    this->data.push_back({std::move(key), std::move(value), key_hash});
    return key_hash;
  }

  auto assign(const key_type &key, const mapped_type &value) -> hash_type {
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

  // As a performance optimisation if the hash is known
  inline auto find(const key_type &key, const hash_type key_hash) const
      -> const_iterator {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->data.size(); index++) {
        if (this->data[index].hash == key_hash) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    } else {
      for (size_type index = 0; index < this->data.size(); index++) {
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

  // As a performance optimisation if the hash is known
  auto contains(const key_type &key, const hash_type key_hash) const -> bool {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (const auto &entry : this->data) {
        if (entry.hash == key_hash) {
          return true;
        }
      }
    } else {
      for (const auto &entry : this->data) {
        if (entry.hash == key_hash && entry.first == key) {
          return true;
        }
      }
    }

    return false;
  }

  // As a performance optimisation if the hash is known

  inline auto at(const key_type &key, const hash_type key_hash) const
      -> const mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->hasher.is_perfect(key_hash)) {
      for (const auto &entry : this->data) {
        if (entry.hash == key_hash) {
          return entry.second;
        }
      }
    } else {
      for (const auto &entry : this->data) {
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

  inline auto at(const key_type &key, const hash_type key_hash)
      -> mapped_type & {
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

  auto erase(const key_type &key, const hash_type key_hash) -> size_type {
    const auto current_size{this->size()};

    if (this->hasher.is_perfect(key_hash)) {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash) {
          std::swap(entry, this->data.back());
          this->data.pop_back();
          return current_size - 1;
        }
      }
    } else {
      for (auto &entry : this->data) {
        if (entry.hash == key_hash && entry.first == key) {
          std::swap(entry, this->data.back());
          this->data.pop_back();
          return current_size - 1;
        }
      }
    }

    return current_size;
  }

  inline auto erase(const key_type &key) -> size_type {
    return this->erase(key, this->hash(key));
  }

  inline auto size() const noexcept -> size_type { return this->data.size(); }

  inline auto empty() const noexcept -> bool { return this->data.empty(); }

  inline auto clear() noexcept -> void { this->data.clear(); }

  auto operator!=(const FlatMap &other) const -> bool = default;

  auto operator==(const FlatMap &other) const -> bool {
    if (this->size() != other.size()) {
      return false;
    }

    for (const auto &entry : this->data) {
      const auto iterator{other.find(entry.first, entry.hash)};
      if (iterator == other.cend()) {
        return false;
      } else if (iterator->second != entry.second) {
        return false;
      }
    }

    return true;
  }

private:
  underlying_type data;
  Hash hasher;
};

} // namespace sourcemeta::noa

#endif
