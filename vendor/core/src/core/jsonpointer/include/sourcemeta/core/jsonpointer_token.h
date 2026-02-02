#ifndef SOURCEMETA_CORE_JSONPOINTER_TOKEN_H_
#define SOURCEMETA_CORE_JSONPOINTER_TOKEN_H_

#include <sourcemeta/core/json.h>

#include <cassert> // assert

namespace sourcemeta::core {

/// @ingroup jsonpointer
template <typename PropertyT, typename Hash> class GenericToken {
public:
  using Value = JSON;
  using Property = PropertyT;
  using Index = typename Value::Array::size_type;

  /// This constructor creates an JSON Pointer token from a string given its
  /// precomputed hash. This is advanced functionality that should be used with
  /// care.
  GenericToken(Property value, const typename Hash::hash_type property_hash)
      : as_property{true}, property{std::move(value)}, hash{property_hash},
        index{0} {}

  /// This constructor creates an JSON Pointer token from a string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer::Token token{"foo"};
  /// ```
  GenericToken(const Property &value) : GenericToken{value, hasher(value)} {}

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
      : GenericToken{value, hasher(value)} {}

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
      : GenericToken{Property{value}, hasher(Property{value})} {}

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
      : as_property{false}, property{DEFAULT_PROPERTY}, hash{0}, index{value} {}

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
      : as_property{false}, property{DEFAULT_PROPERTY}, hash{0},
        index{static_cast<Index>(value)} {}

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
      : as_property{false}, property{DEFAULT_PROPERTY}, hash{0}, index{value} {}
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
    return this->as_property;
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
    return this->as_property && this->property.size() == 1 &&
           this->property.front() == '\u002D';
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
    return !this->as_property;
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
    if constexpr (requires { this->property.get(); }) {
      return this->property.get();
    } else {
      return this->property;
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
  [[nodiscard]] auto property_hash() const noexcept ->
      typename Hash::hash_type {
    assert(this->is_property());
    return this->hash;
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
    if constexpr (requires { this->property.get(); }) {
      return this->property.get();
    } else {
      return this->property;
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
    return this->index;
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
    } else {
      return JSON{this->to_index()};
    }
  }

  /// Compare JSON Pointer tokens
  auto operator==(const GenericToken<PropertyT, Hash> &other) const noexcept
      -> bool {
    if (this->as_property != other.as_property) {
      return false;
    } else if (this->as_property) {
      if constexpr (requires { hasher.is_perfect(this->hash); }) {
        if (hasher.is_perfect(this->hash) && hasher.is_perfect(other.hash)) {
          return this->hash == other.hash;
        }
      }

      return this->hash == other.hash &&
             this->to_property() == other.to_property();
    } else {
      return this->index == other.index;
    }
  }

  /// Overload to support ordering of JSON Pointer tokens. Typically for sorting
  /// reasons.
  auto operator<(const GenericToken<PropertyT, Hash> &other) const noexcept
      -> bool {
    if (this->as_property && !other.as_property) {
      return true;
    } else if (!this->as_property && other.as_property) {
      return false;
    } else if (this->as_property) {
      return this->to_property() < other.to_property();
    } else {
      return this->index < other.index;
    }
  }

private:
  // We need this as a member for making WeakPointer work
  inline static const Value::String DEFAULT_PROPERTY = "";
  inline static const Hash hasher;

  bool as_property;
  Property property;
  typename Hash::hash_type hash;
  Index index;
};

} // namespace sourcemeta::core

#endif
