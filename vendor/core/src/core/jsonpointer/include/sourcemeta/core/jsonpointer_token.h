#ifndef SOURCEMETA_CORE_JSONPOINTER_TOKEN_H_
#define SOURCEMETA_CORE_JSONPOINTER_TOKEN_H_

#include <sourcemeta/core/json.h>

#include <cassert> // assert

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// A single reference token of a JSON Pointer, holding either an object
/// property or an array index.
template <typename PropertyT, typename Hash> class GenericToken {
public:
  /// The JSON value type these tokens convert to
  using Value = JSON;
  /// The stored type of an object property token
  using Property = PropertyT;
  /// The stored type of an array index token
  using Index = Value::Array::size_type;

  /// This constructor creates an JSON Pointer token from a string given its
  /// precomputed hash. This is advanced functionality that should be used with
  /// care.
  GenericToken(Property value, const Hash::HashType property_hash)
      : as_property_{true}, property_{std::move(value)}, hash_{property_hash},
        index_{0} {}

  /// This constructor creates an JSON Pointer token from a string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// ```
  GenericToken(const Property &value) : GenericToken{value, HASHER(value)} {}

  /// This constructor creates an JSON Pointer token by taking ownership of a
  /// string. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// std::string name{"foo"};
  /// const sourcemeta::core::Pointer::Token token{std::move(name)};
  /// ```
  GenericToken(Property &&value)
      : as_property_{true}, property_{std::move(value)},
        hash_{HASHER(property_)}, index_{0} {}

  /// This constructor creates an JSON Pointer token from a string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// ```
  GenericToken(const JSON::Char *const value)
      : GenericToken{value, HASHER(value)} {}

  /// This constructor creates an JSON Pointer token from a character. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{'a'};
  /// ```
  GenericToken(const JSON::Char value)
      : GenericToken{Property{value}, HASHER(Property{value})} {}

  /// This constructor creates an JSON Pointer token from an item index. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{1};
  /// ```
  GenericToken(const Index value)
      : as_property_{false}, property_{DEFAULT_PROPERTY}, hash_{0},
        index_{value} {}

  /// This constructor creates an JSON Pointer token from an item index. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{1};
  /// ```
  GenericToken(const int value)
      : as_property_{false}, property_{DEFAULT_PROPERTY}, hash_{0},
        index_{static_cast<Index>(value)} {
    assert(value >= 0);
  }

#if defined(_MSC_VER)
  /// This constructor creates an JSON Pointer token from an item index. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{1};
  /// ```
  GenericToken(const unsigned long value)
      : as_property_{false}, property_{DEFAULT_PROPERTY}, hash_{0},
        index_{value} {}
#endif

  /// Check if a JSON Pointer token represents an object property.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// ```
  [[nodiscard]] auto is_property() const noexcept -> bool {
    return this->as_property_;
  }

  /// Check if a JSON Pointer token represents the hyphen constant
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token_1{"-"};
  /// const sourcemeta::core::Pointer::Token token_2{'-'};
  /// assert(token_1.is_hyphen());
  /// assert(token_2.is_hyphen());
  /// ```
  [[nodiscard]] auto is_hyphen() const noexcept -> bool {
    return this->as_property_ && this->property_.size() == 1 &&
           this->property_.front() == '\u002D';
  }

  /// Check if a JSON Pointer token represents an array index.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{2};
  /// assert(token.is_index());
  /// ```
  [[nodiscard]] auto is_index() const noexcept -> bool {
    return !this->as_property_;
  }

  /// Get the underlying value of a JSON Pointer object property token (`const`
  /// overload). For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// assert(token.to_property() == "foo");
  /// ```
  [[nodiscard]] auto to_property() const noexcept -> const auto & {
    assert(this->is_property());
    if constexpr (requires { this->property_.get(); }) {
      return this->property_.get();
    } else {
      return this->property_;
    }
  }

  /// If the JSON Pointer token is a property, get its pre-computed string hash.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// assert(token.property_hash() >= 0);
  /// ```
  [[nodiscard]] auto property_hash() const noexcept -> Hash::HashType {
    assert(this->is_property());
    return this->hash_;
  }

  /// Check whether a JSON Pointer property token equals the given string,
  /// comparing the precomputed hashes first and only falling back to a string
  /// comparison when the hash is not perfect. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// assert(token.property_equals(
  ///     "foo", sourcemeta::core::JSON::Object::hash("foo")));
  /// ```
  [[nodiscard]] auto
  property_equals(const JSON::StringView value,
                  const Hash::HashType value_hash) const noexcept -> bool {
    assert(this->is_property());
    assert(HASHER(value.data(), value.size()) == value_hash);
    if constexpr (requires { HASHER.is_perfect(value_hash); }) {
      // A perfect hash captures the property bytes but not its length, so
      // two properties that differ only by trailing NUL bytes hash equal.
      // Comparing sizes disambiguates them without the cost of a full
      // string comparison
      return this->hash_ == value_hash &&
             (HASHER.is_perfect(value_hash)
                  ? this->to_property().size() == value.size()
                  : this->to_property() == value);
    } else {
      return this->hash_ == value_hash && this->to_property() == value;
    }
  }

  /// Get the underlying value of a JSON Pointer object property token
  /// (non-`const` overload). For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// assert(token.to_property() == "foo");
  /// ```
  auto to_property() noexcept -> auto & {
    assert(this->is_property());
    if constexpr (requires { this->property_.get(); }) {
      return this->property_.get();
    } else {
      return this->property_;
    }
  }

  /// Get the underlying value of a JSON Pointer array index token
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{2};
  /// assert(token.is_index());
  /// assert(token.to_index() == 2);
  /// ```
  [[nodiscard]] auto to_index() const noexcept -> Index {
    assert(this->is_index());
    return this->index_;
  }

  /// Convert a JSON Pointer token into a JSON document, whether it represents a
  /// property or an index. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token index{1};
  /// const sourcemeta::core::Pointer::Token property{"foo"};
  ///
  /// const sourcemeta::core::JSON json_index{index.to_json()};
  /// const sourcemeta::core::JSON json_property{property.to_json()};
  ///
  /// assert(json_index.is_integer());
  /// assert(json_property.is_string());
  /// ```
  [[nodiscard]] auto to_json() const -> JSON {
    if (this->is_property()) {
      return JSON{this->to_property()};
    }
    return JSON{this->to_index()};
  }

  /// Compare JSON Pointer tokens
  auto operator==(const GenericToken<PropertyT, Hash> &other) const noexcept
      -> bool {
    if (this->as_property_ != other.as_property_) {
      return false;
    }
    if (this->as_property_) {
      if constexpr (requires { HASHER.is_perfect(this->hash_); }) {
        // A perfect hash captures the property bytes but not its length, so
        // two properties that differ only by trailing NUL bytes hash equal.
        // Comparing sizes disambiguates them without the cost of a full
        // string comparison
        if (HASHER.is_perfect(this->hash_) && HASHER.is_perfect(other.hash_)) {
          return this->hash_ == other.hash_ &&
                 this->to_property().size() == other.to_property().size();
        }
      }

      return this->hash_ == other.hash_ &&
             this->to_property() == other.to_property();
    }
    return this->index_ == other.index_;
  }

  /// Overload to support ordering of JSON Pointer tokens. Typically for sorting
  /// reasons.
  auto operator<(const GenericToken<PropertyT, Hash> &other) const noexcept
      -> bool {
    if (this->as_property_ && !other.as_property_) {
      return true;
    }
    if (!this->as_property_ && other.as_property_) {
      return false;
    }
    if (this->as_property_) {
      return this->to_property() < other.to_property();
    }
    return this->index_ < other.index_;
  }

private:
  // We need this as a member for making WeakPointer work
  inline static const Value::String DEFAULT_PROPERTY{};
  inline static const Hash HASHER;

  bool as_property_;
  Property property_;
  Hash::HashType hash_;
  Index index_;
};

} // namespace sourcemeta::core

#endif
