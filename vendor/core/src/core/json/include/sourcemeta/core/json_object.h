#ifndef SOURCEMETA_CORE_JSON_OBJECT_H_
#define SOURCEMETA_CORE_JSON_OBJECT_H_

#include <algorithm>        // std::sort
#include <cassert>          // assert
#include <concepts>         // std::same_as
#include <cstddef>          // std::size_t
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::advance
#include <string_view>      // std::basic_string_view
#include <type_traits>      // std::remove_cvref_t
#include <utility>          // std::pair, std::move, std::unreachable
#include <vector>           // std::vector

namespace sourcemeta::core {

/// @ingroup json
/// A JSON object mapping property keys to values
template <typename Key, typename Value, typename Hash> class JSONObject {
public:
  JSONObject() = default;

  using key_type = Key;
  using mapped_type = Value;
  using hash_type = Hash::HashType;
  using pair_value_type = std::pair<key_type, mapped_type>;
  /// The string view type used to look up object keys
  using KeyView = std::basic_string_view<typename Key::value_type,
                                         typename Key::traits_type>;

  /// Construct an object from a list of key and value pairs
  JSONObject(std::initializer_list<pair_value_type> entries) : data_{} {
    this->data_.reserve(entries.size());
    for (auto &&entry : entries) {
      this->emplace(std::move(entry.first), std::move(entry.second));
    }
  }

  /// A single object property entry
  struct Entry {
    /// The property key
    key_type first;
    /// The property value
    mapped_type second;
    /// The precomputed hash of the property key
    hash_type hash;

    /// Check whether this entry's key equals the given key, comparing the
    /// precomputed hashes first and only falling back to a string comparison
    /// when the hash is not perfect. For example:
    ///
    /// ```cpp
    /// #include <sourcemeta/core/json.h>
    /// #include <cassert>
    ///
    /// const sourcemeta::core::JSON document =
    ///   sourcemeta::core::parse_json("{ \"foo\": 1 }");
    /// const auto &entry{*document.as_object().cbegin()};
    /// assert(entry.key_equals(
    ///   "foo", sourcemeta::core::JSON::Object::hash("foo")));
    /// ```
    [[nodiscard]] auto key_equals(const KeyView key,
                                  const hash_type key_hash) const -> bool {
      assert(JSONObject::hash(key) == key_hash);
      // A perfect hash captures the key bytes but not its length, so two keys
      // that differ only by trailing NUL bytes hash equal. Comparing sizes
      // disambiguates them without the cost of a full string comparison
      return this->hash == key_hash &&
             (HASHER.is_perfect(key_hash) ? this->first.size() == key.size()
                                          : this->first == key);
    }
  };

  using underlying_type = std::vector<Entry>;
  using value_type = underlying_type::value_type;
  using size_type = underlying_type::size_type;
  using difference_type = underlying_type::difference_type;
  using allocator_type = underlying_type::allocator_type;
  using reference = underlying_type::reference;
  using const_reference = underlying_type::const_reference;
  using pointer = underlying_type::pointer;
  using const_pointer = underlying_type::const_pointer;
  using const_iterator = underlying_type::const_iterator;

  // Operators
  // We cannot default given that this class references
  // a JSON "value" as an incomplete type

  auto operator<(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    // Objects have no inherent order, but a deterministic strict weak ordering
    // independent of insertion order is needed so that collections of objects
    // can be sorted. Smaller objects come first, and objects of equal size are
    // ordered as their entries would compare in key order. That outcome is
    // decided entirely by the smallest key at which the two objects differ,
    // which is found by scanning the entries in place to avoid allocating
    if (this->data_.size() != other.data_.size()) {
      return this->data_.size() < other.data_.size();
    }

    const Key *decisive_key{nullptr};
    bool decision{false};
    for (const auto &entry : this->data_) {
      const auto match{other.find(entry.first)};
      const bool differs{match == other.cend() ||
                         !(entry.second == match->second)};
      if (differs && (decisive_key == nullptr || entry.first < *decisive_key)) {
        decisive_key = &entry.first;
        decision = match == other.cend() || entry.second < match->second;
      }
    }

    for (const auto &entry : other.data_) {
      if (this->find(entry.first) == this->cend() &&
          (decisive_key == nullptr || entry.first < *decisive_key)) {
        decisive_key = &entry.first;
        decision = false;
      }
    }

    return decision;
  }

  auto operator<=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return !(other < *this);
  }
  auto operator>(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return other < *this;
  }
  auto operator>=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    return !(*this < other);
  }

  auto operator==(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool {
    if (this->size() != other.size()) {
      return false;
    }

    for (const auto &entry : this->data_) {
      const auto *result{other.try_at(entry.first, entry.hash)};
      if ((result == nullptr) || *result != entry.second) {
        return false;
      }
    }

    return true;
  }

  auto operator!=(const JSONObject<Key, Value, Hash> &other) const noexcept
      -> bool = default;

  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    return this->data_.begin();
  }
  /// Get a constant end iterator on the object
  [[nodiscard]] auto end() const noexcept -> const_iterator {
    return this->data_.end();
  }
  /// Get a constant begin iterator on the object
  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return this->data_.cbegin();
  }
  /// Get a constant end iterator on the object
  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return this->data_.cend();
  }

  // GCC does not optimise well across implicit type conversions such as
  // std::string to std::string_view, so we provide separate overloads with
  // duplicated logic instead of unifying on a single parameter type. The
  // `KeyView`-accepting overloads are constrained to actual `std::string_view`
  // arguments only, so callers passing a string literal continue to bind to
  // the `Key`-accepting overload as before.

  /// Compute a hash for a key
  [[nodiscard]] static constexpr auto hash(const Key &key) noexcept
      -> hash_type {
    return HASHER(key);
  }

  /// Compute a hash for a key
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] static constexpr auto hash(T key) noexcept -> hash_type {
    return HASHER(key.data(), key.size());
  }

  /// Compute a hash from raw data
  [[nodiscard]] static constexpr auto hash(const char *raw_data,
                                           const std::size_t raw_size) noexcept
      -> hash_type {
    return HASHER(raw_data, raw_size);
  }

  /// Attempt to find an entry by key
  [[nodiscard]] auto find(const Key &key) const -> const_iterator {
    const auto key_hash{this->hash(key)};

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first.size() == key.size()) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    } else {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first == key) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    }

    return this->cend();
  }

  /// Attempt to find an entry by key
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] auto find(T key) const -> const_iterator {
    const auto key_hash{this->hash(key)};

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first.size() == key.size()) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    } else {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first == key) {
          auto iterator{this->cbegin()};
          std::advance(iterator, index);
          return iterator;
        }
      }
    }

    return this->cend();
  }

  /// Check if an entry with the given key exists
  [[nodiscard]] auto defines(const Key &key, const hash_type hash) const
      -> bool {
    assert(this->hash(key) == hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(hash)) {
      for (const auto &entry : *this) {
        if (entry.hash == hash && entry.first.size() == key.size()) {
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

  /// Check if an entry with the given key exists
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] auto defines(T key, const hash_type hash) const -> bool {
    assert(this->hash(key) == hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(hash)) {
      for (const auto &entry : *this) {
        if (entry.hash == hash && entry.first.size() == key.size()) {
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
  [[nodiscard]] auto size() const -> std::size_t { return this->data_.size(); }

  /// Check if the object is empty
  [[nodiscard]] auto empty() const -> bool { return this->data_.empty(); }

  /// Reserve capacity for a given number of entries
  auto reserve(const size_type capacity) -> void {
    this->data_.reserve(capacity);
  }

  /// Access an object entry by its underlying positional index
  [[nodiscard]] auto at(const size_type index) const -> const Entry & {
    return this->data_.at(index);
  }

  /// Access an object entry by its key name
  [[nodiscard]] auto at(const Key &key, const hash_type key_hash) const
      -> const mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (const auto &entry : *this) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
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

    std::unreachable();
  }

  /// Access an object entry by its key name
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] auto at(T key, const hash_type key_hash) const
      -> const mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (const auto &entry : *this) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
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

    std::unreachable();
  }

  /// Access an object entry by its key name
  auto at(const Key &key, const hash_type key_hash) -> mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          return entry.second;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          return entry.second;
        }
      }
    }

    std::unreachable();
  }

  /// Access an object entry by its key name
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  auto at(T key, const hash_type key_hash) -> mapped_type & {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          return entry.second;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          return entry.second;
        }
      }
    }

    std::unreachable();
  }

  /// Try to access an object entry by its key name
  [[nodiscard]] auto try_at(const Key &key, const hash_type key_hash)
      -> mapped_type * {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          return &entry.second;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          return &entry.second;
        }
      }
    }

    return nullptr;
  }

  /// Try to access an object entry by its key name
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] auto try_at(T key, const hash_type key_hash) -> mapped_type * {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          return &entry.second;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          return &entry.second;
        }
      }
    }

    return nullptr;
  }

  /// Try to access an object entry by its underlying positional index
  [[nodiscard]] auto try_at(const Key &key, const hash_type key_hash) const
      -> const mapped_type * {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first.size() == key.size()) {
          return &this->data_[index].second;
        }
      }
    } else {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first == key) {
          return &this->data_[index].second;
        }
      }
    }

    return nullptr;
  }

  /// Try to access an object entry by its underlying positional index
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] auto try_at(T key, const hash_type key_hash) const
      -> const mapped_type * {
    assert(this->hash(key) == key_hash);

    // Move the perfect hash condition out of the loop for extra performance
    if (this->HASHER.is_perfect(key_hash)) {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first.size() == key.size()) {
          return &this->data_[index].second;
        }
      }
    } else {
      for (size_type index = 0; index < this->size(); index++) {
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first == key) {
          return &this->data_[index].second;
        }
      }
    }

    return nullptr;
  }

  /// Try to access an object entry, scanning from a caller-provided start
  /// offset. On hit, advances `start` past the found index
  [[nodiscard]] auto try_at(const Key &key, const hash_type key_hash,
                            size_type &start) const -> const mapped_type * {
    assert(this->hash(key) == key_hash);
    const auto object_size{this->size()};
    assert(start <= object_size);
    if (this->HASHER.is_perfect(key_hash)) {
      for (size_type count = 0; count < object_size; count++) {
        const auto index{(start + count) % object_size};
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first.size() == key.size()) {
          start = index + 1;
          return &this->data_[index].second;
        }
      }
    } else {
      for (size_type count = 0; count < object_size; count++) {
        const auto index{(start + count) % object_size};
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first == key) {
          start = index + 1;
          return &this->data_[index].second;
        }
      }
    }

    return nullptr;
  }

  /// Try to access an object entry, scanning from a caller-provided start
  /// offset. On hit, advances `start` past the found index
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  [[nodiscard]] auto try_at(T key, const hash_type key_hash,
                            size_type &start) const -> const mapped_type * {
    assert(this->hash(key) == key_hash);
    const auto object_size{this->size()};
    assert(start <= object_size);
    if (this->HASHER.is_perfect(key_hash)) {
      for (size_type count = 0; count < object_size; count++) {
        const auto index{(start + count) % object_size};
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first.size() == key.size()) {
          start = index + 1;
          return &this->data_[index].second;
        }
      }
    } else {
      for (size_type count = 0; count < object_size; count++) {
        const auto index{(start + count) % object_size};
        if (this->data_[index].hash == key_hash &&
            this->data_[index].first == key) {
          start = index + 1;
          return &this->data_[index].second;
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

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto iterator = this->data_.begin(); iterator != this->data_.end();
           ++iterator) {
        if (iterator->hash == key_hash &&
            iterator->first.size() == key.size()) {
          iterator->second = value;
          return key_hash;
        }
        if (iterator->hash == suffix_hash && iterator->first == suffix) {
          this->data_.insert(iterator, {key, value, key_hash});
          return key_hash;
        }
      }
    } else {
      for (auto iterator = this->data_.begin(); iterator != this->data_.end();
           ++iterator) {
        if (iterator->hash == key_hash && iterator->first == key) {
          iterator->second = value;
          return key_hash;
        }
        if (iterator->hash == suffix_hash && iterator->first == suffix) {
          this->data_.insert(iterator, {key, value, key_hash});
          return key_hash;
        }
      }
    }

    this->data_.push_back({key, value, key_hash});
    return key_hash;
  }

  /// Emplace an object property
  auto emplace(Key &&key, mapped_type &&value) -> hash_type {
    const auto key_hash{this->hash(key)};

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          entry.second = std::move(value);
          return key_hash;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.second = std::move(value);
          return key_hash;
        }
      }
    }

    this->data_.push_back({std::move(key), std::move(value), key_hash});
    return key_hash;
  }

  /// Emplace an object property
  auto emplace(const Key &key, mapped_type &&value) -> hash_type {
    const auto key_hash{this->hash(key)};

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          entry.second = std::move(value);
          return key_hash;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.second = std::move(value);
          return key_hash;
        }
      }
    }

    this->data_.push_back({key, std::move(value), key_hash});
    return key_hash;
  }

  /// Emplace an object property
  auto emplace(const Key &key, const mapped_type &value) -> hash_type {
    const auto key_hash{this->hash(key)};

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          entry.second = value;
          return key_hash;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.second = value;
          return key_hash;
        }
      }
    }

    this->data_.push_back({key, value, key_hash});
    return key_hash;
  }

  /// Emplace an object property assuming the key does not already exist
  auto emplace_assume_new(Key &&key, mapped_type &&value) -> hash_type {
    const auto key_hash{this->hash(key)};
    this->data_.push_back({std::move(key), std::move(value), key_hash});
    return key_hash;
  }

  /// Emplace an object property assuming the key does not already exist
  auto emplace_assume_new(const Key &key, mapped_type &&value) -> hash_type {
    const auto key_hash{this->hash(key)};
    this->data_.push_back({key, std::move(value), key_hash});
    return key_hash;
  }

  /// Emplace an object property with a pre-computed hash, returning the
  /// inserted value
  auto emplace_assume_new(Key &&key, mapped_type &&value,
                          const hash_type key_hash) -> mapped_type & {
    this->data_.push_back({std::move(key), std::move(value), key_hash});
    return this->data_.back().second;
  }

  /// Emplace an object property with a pre-computed hash
  auto emplace_assume_new(const Key &key, mapped_type &&value,
                          const hash_type key_hash) -> void {
    this->data_.push_back({key, std::move(value), key_hash});
  }

  /// Get the key of the last-inserted property
  [[nodiscard]] auto back_key() const noexcept -> const Key & {
    assert(!this->data_.empty());
    return this->data_.back().first;
  }

  /// Remove every property in the object
  auto clear() noexcept -> void { this->data_.clear(); }

  /// Rename an object property in place
  auto rename(const Key &key, const hash_type key_hash, Key &&target,
              const hash_type target_hash) -> void {
    this->erase(target, target_hash);

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first.size() == key.size()) {
          entry.first = std::move(target);
          entry.hash = target_hash;
          break;
        }
      }
    } else {
      for (auto &entry : this->data_) {
        if (entry.hash == key_hash && entry.first == key) {
          entry.first = std::move(target);
          entry.hash = target_hash;
          break;
        }
      }
    }
  }

  /// Erase an object property
  auto erase(const Key &key, const hash_type key_hash) -> size_type {
    const auto current_size{this->size()};

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto iterator = this->data_.begin(); iterator != this->data_.end();
           ++iterator) {
        if (iterator->hash == key_hash &&
            iterator->first.size() == key.size()) {
          this->data_.erase(iterator);
          return current_size - 1;
        }
      }
    } else {
      for (auto iterator = this->data_.begin(); iterator != this->data_.end();
           ++iterator) {
        if (iterator->hash == key_hash && iterator->first == key) {
          this->data_.erase(iterator);
          return current_size - 1;
        }
      }
    }

    return current_size;
  }

  /// Erase an object property
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  auto erase(T key, const hash_type key_hash) -> size_type {
    const auto current_size{this->size()};

    if (this->HASHER.is_perfect(key_hash)) {
      for (auto iterator = this->data_.begin(); iterator != this->data_.end();
           ++iterator) {
        if (iterator->hash == key_hash &&
            iterator->first.size() == key.size()) {
          this->data_.erase(iterator);
          return current_size - 1;
        }
      }
    } else {
      for (auto iterator = this->data_.begin(); iterator != this->data_.end();
           ++iterator) {
        if (iterator->hash == key_hash && iterator->first == key) {
          this->data_.erase(iterator);
          return current_size - 1;
        }
      }
    }

    return current_size;
  }

  /// Erase an object property
  auto erase(const Key &key) -> size_type {
    return this->erase(key, this->hash(key));
  }

  /// Erase an object property
  template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, KeyView>
  auto erase(T key) -> size_type {
    return this->erase(key, this->hash(key));
  }

  /// Reorder object properties by keys according to a comparator function
  template <typename Compare> auto reorder(const Compare &compare) -> void {
    std::sort(this->data_.begin(), this->data_.end(),
              [&compare](const auto &left, const auto &right) -> auto {
                return compare(left.first, right.first);
              });
  }

private:
  friend Value;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  static constexpr Hash HASHER{};
  underlying_type data_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
