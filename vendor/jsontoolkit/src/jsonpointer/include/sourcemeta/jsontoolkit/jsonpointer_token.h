#ifndef SOURCEMETA_JSONTOOLKIT_JSONPOINTER_TOKEN_H_
#define SOURCEMETA_JSONTOOLKIT_JSONPOINTER_TOKEN_H_

#include <sourcemeta/jsontoolkit/json.h>

#include <cassert> // assert
#include <utility> // std::in_place_type
#include <variant> // std::variant, std::holds_alternative, std::get

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonpointer
template <typename PropertyT> class GenericToken {
public:
  using Value = JSON;
  using Property = PropertyT;
  using Index = typename Value::Array::size_type;

  /// This constructor creates an JSON Pointer token from a string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{"foo"};
  /// ```
  GenericToken(const Property &property)
      : data{std::in_place_type<Property>, property} {}

  /// This constructor creates an JSON Pointer token from a string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{"foo"};
  /// ```
  GenericToken(const JSON::Char *const property)
      : data{std::in_place_type<Property>, property} {}

  /// This constructor creates an JSON Pointer token from a character. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{'a'};
  /// ```
  GenericToken(const JSON::Char character)
      : data{std::in_place_type<Property>, Property{character}} {}

  /// This constructor creates an JSON Pointer token from an item index. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{1};
  /// ```
  GenericToken(const Index index) : data{std::in_place_type<Index>, index} {}

  /// This constructor creates an JSON Pointer token from an item index. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{1};
  /// ```
  GenericToken(const int index) : data{std::in_place_type<Index>, index} {}

#if defined(_MSC_VER)
  /// This constructor creates an JSON Pointer token from an item index. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{1};
  /// ```
  GenericToken(const unsigned long index)
      : data{std::in_place_type<Index>, index} {}
#endif

  /// Check if a JSON Pointer token represents an object property.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// ```
  [[nodiscard]] auto is_property() const noexcept -> bool {
    return std::holds_alternative<Property>(this->data);
  }

  /// Check if a JSON Pointer token represents the hyphen constant
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token_1{"-"};
  /// const sourcemeta::jsontoolkit::Pointer::Token token_2{'-'};
  /// assert(token_1.is_hyphen());
  /// assert(token_2.is_hyphen());
  /// ```
  [[nodiscard]] auto is_hyphen() const noexcept -> bool {
    return this->is_property() && this->to_property().size() == 1 &&
           this->to_property().front() == '\u002D';
  }

  /// Check if a JSON Pointer token represents an array index.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{2};
  /// assert(token.is_index());
  /// ```
  [[nodiscard]] auto is_index() const noexcept -> bool {
    return std::holds_alternative<Index>(this->data);
  }

  /// Get the underlying value of a JSON Pointer object property token (`const`
  /// overload). For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// assert(token.to_property() == "foo");
  /// ```
  [[nodiscard]] auto to_property() const noexcept -> const auto & {
    assert(this->is_property());
    if constexpr (requires { std::get<Property>(this->data).get(); }) {
      return std::get<Property>(this->data).get();
    } else {
      return std::get<Property>(this->data);
    }
  }

  /// Get the underlying value of a JSON Pointer object property token
  /// (non-`const` overload). For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer::Token token{"foo"};
  /// assert(token.is_property());
  /// assert(token.to_property() == "foo");
  /// ```
  auto to_property() noexcept -> auto & {
    assert(this->is_property());
    if constexpr (requires { std::get<Property>(this->data).get(); }) {
      return std::get<Property>(this->data).get();
    } else {
      return std::get<Property>(this->data);
    }
  }

  /// Get the underlying value of a JSON Pointer array index token
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token token{2};
  /// assert(token.is_index());
  /// assert(token.to_index() == 2);
  /// ```
  [[nodiscard]] auto to_index() const noexcept -> Index {
    assert(this->is_index());
    return std::get<Index>(this->data);
  }

  /// Convert a JSON Pointer token into a JSON document, whether it represents a
  /// property or an index. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer::Token index{1};
  /// const sourcemeta::jsontoolkit::Pointer::Token property{"foo"};
  ///
  /// const sourcemeta::jsontoolkit::JSON json_index{index.to_json()};
  /// const sourcemeta::jsontoolkit::JSON json_property{property.to_json()};
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
  auto operator==(const GenericToken<PropertyT> &other) const noexcept -> bool {
    if (this->data.index() != other.data.index()) {
      return false;
    }
    if (this->is_property()) {
      return this->to_property() == other.to_property();
    }
    return this->to_index() == other.to_index();
  }

  /// Overload to support ordering of JSON Pointer token. Typically for sorting
  /// reasons.
  auto operator<(const GenericToken<PropertyT> &other) const noexcept -> bool {
    if (this->data.index() != other.data.index()) {
      return this->data.index() < other.data.index();
    }
    if (this->is_property()) {
      return this->to_property() < other.to_property();
    }
    return this->to_index() < other.to_index();
  }

private:
  std::variant<Property, Index> data;
};

} // namespace sourcemeta::jsontoolkit

#endif
