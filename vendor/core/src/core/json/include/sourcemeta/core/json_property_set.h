#ifndef SOURCEMETA_CORE_JSON_PROPERTY_SET_H_
#define SOURCEMETA_CORE_JSON_PROPERTY_SET_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

#include <sourcemeta/core/json_hash.h>
#include <sourcemeta/core/json_value.h>

#include <cassert>  // assert
#include <optional> // std::optional
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup json
/// A set of JSON object property names, sorted in lexicographic order, that
/// keeps the hash of every name alongside it so that repeated lookups on JSON
/// objects do not have to hash the same names over and over again. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// sourcemeta::core::JSONPropertySet properties;
/// properties.insert("foo");
/// properties.insert("bar");
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2 }");
/// for (const auto &property : properties) {
///   assert(document.defines(property.first, property.second));
/// }
/// ```
class SOURCEMETA_CORE_JSON_EXPORT JSONPropertySet {
public:
  JSONPropertySet() = default;

  /// The type of the property names held by the set
  using string_type = JSON::String;
  /// The type of the hash kept alongside every property name
  using hash_type = JSON::Object::hash_type;
  /// A property name together with its hash
  using value_type = std::pair<string_type, hash_type>;
  /// The underlying container that holds the property names
  using underlying_type = std::vector<value_type>;
  using size_type = underlying_type::size_type;
  using difference_type = underlying_type::difference_type;
  using const_iterator = underlying_type::const_iterator;

  /// Check whether the set contains a property name whose hash is already
  /// known, avoiding the cost of hashing it again
  [[nodiscard]] auto contains(const string_type &value,
                              const hash_type hash) const -> bool {
    assert(HASHER(value) == hash);
    if (HASHER.is_perfect(hash)) {
      // A perfect hash captures the property name bytes but not its length, so
      // two names that differ only by trailing NUL bytes hash equal. Comparing
      // sizes disambiguates them without the cost of a full string comparison
      for (const auto &entry : this->data_) {
        if (entry.second == hash && entry.first.size() == value.size()) {
          return true;
        }
      }
    } else {
      for (const auto &entry : this->data_) {
        if (entry.second == hash && entry.first == value) {
          return true;
        }
      }
    }

    return false;
  }

  /// Check whether the set contains a property name
  [[nodiscard]] auto contains(const string_type &value) const -> bool {
    return this->contains(value, HASHER(value));
  }

  /// Add a property name whose hash is already known to the set, keeping the
  /// set sorted. Returns whether the name was added, so that a caller can tell
  /// a name it had not seen before from one the set already held
  auto insert(const string_type &value, const hash_type hash) -> bool;

  /// Add a property name whose hash is already known to the set, keeping the
  /// set sorted. Returns whether the name was added, so that a caller can tell
  /// a name it had not seen before from one the set already held
  auto insert(string_type &&value, const hash_type hash) -> bool;

  /// Add a property name to the set, keeping the set sorted. Returns whether
  /// the name was added, so that a caller can tell a name it had not seen
  /// before from one the set already held
  auto insert(const string_type &value) -> bool;

  /// Add a property name to the set, keeping the set sorted. Returns whether
  /// the name was added, so that a caller can tell a name it had not seen
  /// before from one the set already held
  auto insert(string_type &&value) -> bool;

  /// Get a property name and its hash by index
  [[nodiscard]] auto at(const size_type index) const noexcept
      -> const value_type & {
    assert(index < this->data_.size());
    return this->data_[index];
  }

  /// Check whether the set is empty
  [[nodiscard]] auto empty() const noexcept -> bool {
    return this->data_.empty();
  }

  /// Get the number of property names in the set
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->data_.size();
  }

  /// Get a constant begin iterator on the set
  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    return this->data_.begin();
  }

  /// Get a constant end iterator on the set
  [[nodiscard]] auto end() const noexcept -> const_iterator {
    return this->data_.end();
  }

  /// Get a constant begin iterator on the set
  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return this->data_.cbegin();
  }

  /// Get a constant end iterator on the set
  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return this->data_.cend();
  }

  /// Serialise the set as a JSON array of property names
  [[nodiscard]] auto to_json() const -> JSON;

  /// Reconstruct a set from a JSON array of property names, yielding no result
  /// if the given document is not an array of strings
  static auto from_json(const JSON &value) -> std::optional<JSONPropertySet>;

private:
  static constexpr PropertyHashJSON<string_type> HASHER{};
  underlying_type data_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
