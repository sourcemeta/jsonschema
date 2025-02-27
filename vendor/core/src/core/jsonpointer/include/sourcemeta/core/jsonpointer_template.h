#ifndef SOURCEMETA_CORE_JSONPOINTER_TEMPLATE_H_
#define SOURCEMETA_CORE_JSONPOINTER_TEMPLATE_H_

#include <sourcemeta/core/jsonpointer_pointer.h>
#include <sourcemeta/core/jsonpointer_token.h>

#include <algorithm> // std::copy
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <variant>   // std::variant, std::holds_alternative
#include <vector>    // std::vector

namespace sourcemeta::core {

/// @ingroup jsonpointer
template <typename PointerT> class GenericPointerTemplate {
public:
  enum class Wildcard { Property, Item, Key };
  struct Condition {
    auto operator==(const Condition &) const noexcept -> bool = default;
    auto operator<(const Condition &) const noexcept -> bool { return false; }
  };
  struct Negation {
    auto operator==(const Negation &) const noexcept -> bool = default;
    auto operator<(const Negation &) const noexcept -> bool { return false; }
  };
  using Regex = typename PointerT::Value::String;
  using Token = typename PointerT::Token;
  using Container =
      std::vector<std::variant<Wildcard, Condition, Negation, Regex, Token>>;

  /// This constructor creates an empty JSON Pointer template. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  ///
  /// const sourcemeta::core::PointerTemplate pointer;
  /// ```
  GenericPointerTemplate() : data{} {}

  /// This constructor is the preferred way of creating a pointer template.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::PointerTemplate pointer{
  ///   "foo",
  ///   sourcemeta::core::PointerTemplate::Wildcard::Property};
  /// ```
  GenericPointerTemplate(
      std::initializer_list<typename Container::value_type> tokens)
      : data{std::move(tokens)} {}

  /// This constructor creates a JSON Pointer template from an existing JSON
  /// Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  ///
  /// const sourcemeta::core::Pointer base{"foo", "bar"};
  /// const sourcemeta::core::PointerTemplate pointer{base};
  /// ```
  GenericPointerTemplate(const PointerT &other) { this->push_back(other); }

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

  /// Get a mutable begin iterator on the pointer
  auto begin() noexcept -> iterator { return this->data.begin(); }
  /// Get a mutable end iterator on the pointer
  auto end() noexcept -> iterator { return this->data.end(); }
  /// Get a constant begin iterator on the pointer
  auto begin() const noexcept -> const_iterator { return this->data.begin(); }
  /// Get a constant end iterator on the pointer
  auto end() const noexcept -> const_iterator { return this->data.end(); }
  /// Get a constant begin iterator on the pointer
  auto cbegin() const noexcept -> const_iterator { return this->data.cbegin(); }
  /// Get a constant end iterator on the pointer
  auto cend() const noexcept -> const_iterator { return this->data.cend(); }
  /// Get a mutable reverse begin iterator on the pointer
  auto rbegin() noexcept -> reverse_iterator { return this->data.rbegin(); }
  /// Get a mutable reverse end iterator on the pointer
  auto rend() noexcept -> reverse_iterator { return this->data.rend(); }
  /// Get a constant reverse begin iterator on the pointer
  auto rbegin() const noexcept -> const_reverse_iterator {
    return this->data.rbegin();
  }
  /// Get a constant reverse end iterator on the pointer
  auto rend() const noexcept -> const_reverse_iterator {
    return this->data.rend();
  }
  /// Get a constant reverse begin iterator on the pointer
  auto crbegin() const noexcept -> const_reverse_iterator {
    return this->data.crbegin();
  }
  /// Get a constant reverse end iterator on the pointer
  auto crend() const noexcept -> const_reverse_iterator {
    return this->data.crend();
  }

  /// Emplace a token or wildcard into the back of a JSON Pointer template. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  ///
  /// sourcemeta::core::PointerTemplate pointer;
  /// pointer.emplace_back(sourcemeta::core::PointerTemplate::Wildcard::Property);
  /// ```
  template <class... Args> auto emplace_back(Args &&...args) -> reference {
    return this->data.emplace_back(args...);
  }

  /// Push a copy of a JSON Pointer into the back of a JSON Pointer template.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  ///
  /// sourcemeta::core::PointerTemplate result;
  /// const sourcemeta::core::Pointer pointer{"bar", "baz"};
  /// result.push_back(pointer);
  /// ```
  auto push_back(const PointerT &other) -> void {
    this->data.reserve(this->data.size() + other.size());
    std::copy(other.cbegin(), other.cend(), std::back_inserter(this->data));
  }

  /// Remove the last token of a JSON Pointer template. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  ///
  /// const sourcemeta::core::Pointer base{"bar", "baz"};
  /// sourcemeta::core::PointerTemplate pointer{base};
  /// pointer.pop_back();
  /// ```
  auto pop_back() -> void {
    assert(!this->empty());
    this->data.pop_back();
  }

  /// Concatenate a JSON Pointer template with another JSON Pointer template,
  /// getting a new pointer template as a result. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer_left{"foo"};
  /// const sourcemeta::core::Pointer pointer_right{"bar", "baz"};
  /// const sourcemeta::core::Pointer pointer_expected{"foo", "bar", "baz"};
  ///
  /// const sourcemeta::core::PointerTemplate left{pointer_left};
  /// const sourcemeta::core::PointerTemplate right{pointer_right};
  /// const sourcemeta::core::PointerTemplate expected{pointer_expected};
  ///
  /// assert(left.concat(right) == expected);
  /// ```
  auto concat(const GenericPointerTemplate<PointerT> &&other) const
      -> GenericPointerTemplate<PointerT> {
    GenericPointerTemplate<PointerT> result{*this};
    result.data.reserve(result.data.size() + other.data.size());
    for (auto &&token : other) {
      result.emplace_back(std::move(token));
    }

    return result;
  }

  /// Check if a JSON Pointer template is empty.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::PointerTemplate empty_pointer;
  /// assert(empty_pointer.empty());
  /// ```
  [[nodiscard]] auto empty() const noexcept -> bool {
    return this->data.empty();
  }

  /// Check if a JSON Pointer template is equal to another JSON Pointer template
  /// when not taking into account condition tokens. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::PointerTemplate left{
  ///     sourcemeta::core::PointerTemplate::Condition{},
  ///     sourcemeta::core::Pointer::Token{"foo"}};
  /// const sourcemeta::core::PointerTemplate right{
  ///     sourcemeta::core::Pointer::Token{"foo"}};
  ///
  /// assert(left.conditional_of(right));
  /// assert(right.conditional_of(left));
  /// ```
  [[nodiscard]] auto
  conditional_of(const GenericPointerTemplate<PointerT> &other) const noexcept
      -> bool {
    auto iterator_this = this->data.cbegin();
    auto iterator_that = other.data.cbegin();

    while (iterator_this != this->data.cend() &&
           iterator_that != other.data.cend()) {
      while (iterator_this != this->data.cend() &&
             std::holds_alternative<Condition>(*iterator_this)) {
        iterator_this += 1;
      }

      while (iterator_that != other.data.cend() &&
             std::holds_alternative<Condition>(*iterator_that)) {
        iterator_that += 1;
      }

      if (iterator_this == this->data.cend() ||
          iterator_that == other.data.cend()) {
        return iterator_this == this->data.cend() &&
               iterator_that == other.data.cend();
      } else if (*iterator_this != *iterator_that) {
        return false;
      } else {
        iterator_this += 1;
        iterator_that += 1;
      }
    }

    return iterator_this == this->data.cend() &&
           iterator_that == other.data.cend();
  }

  /// Compare JSON Pointer template instances
  auto operator==(const GenericPointerTemplate<PointerT> &other) const noexcept
      -> bool {
    return this->data == other.data;
  }

  /// Overload to support ordering of JSON Pointer templates. Typically for
  /// sorting reasons.
  auto operator<(const GenericPointerTemplate<PointerT> &other) const noexcept
      -> bool {
    return this->data < other.data;
  }

private:
  Container data;
};

} // namespace sourcemeta::core

#endif
