#ifndef SOURCEMETA_CORE_JSONPOINTER_TEMPLATE_H_
#define SOURCEMETA_CORE_JSONPOINTER_TEMPLATE_H_

#include <sourcemeta/core/regex.h>

#include <algorithm>        // std::copy, std::all_of
#include <cassert>          // assert
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::back_inserter
#include <optional>         // std::optional, std::nullopt
#include <type_traits>      // std::is_convertible_v, std::is_null_pointer_v
#include <utility>          // std::forward
#include <variant>          // std::variant, std::holds_alternative, std::get
#include <vector>           // std::vector

namespace sourcemeta::core {

/// @ingroup jsonpointer
template <typename PointerT> class GenericPointerTemplate {
public:
  enum class Wildcard : std::uint8_t { Property, Item, Key };
  struct Condition {
    auto operator==(const Condition &) const noexcept -> bool = default;
    auto operator<(const Condition &) const noexcept -> bool { return false; }
    std::optional<typename PointerT::Value::String> suffix = std::nullopt;
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

  /// This constructor creates a JSON Pointer template from properties or
  /// indexes. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::PointerTemplate pointer_1{"foo", "bar", "baz"};
  /// assert(pointer_1.size() == 3);
  /// const sourcemeta::core::PointerTemplate pointer_2{"foo", 1, "bar"};
  /// assert(pointer_2.size() == 3);
  /// ```
  template <typename... Args>
    requires(sizeof...(Args) > 0 &&
             ((!std::is_null_pointer_v<std::remove_cvref_t<Args>> &&
               (std::is_convertible_v<Args, const char *> ||
                std::is_integral_v<std::remove_cvref_t<Args>>)) &&
              ...))
  GenericPointerTemplate(Args &&...args)
      : data{Token{std::forward<Args>(args)}...} {}

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
  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    return this->data.begin();
  }
  /// Get a constant end iterator on the pointer
  [[nodiscard]] auto end() const noexcept -> const_iterator {
    return this->data.end();
  }
  /// Get a constant begin iterator on the pointer
  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return this->data.cbegin();
  }
  /// Get a constant end iterator on the pointer
  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return this->data.cend();
  }
  /// Get a mutable reverse begin iterator on the pointer
  auto rbegin() noexcept -> reverse_iterator { return this->data.rbegin(); }
  /// Get a mutable reverse end iterator on the pointer
  auto rend() noexcept -> reverse_iterator { return this->data.rend(); }
  /// Get a constant reverse begin iterator on the pointer
  [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
    return this->data.rbegin();
  }
  /// Get a constant reverse end iterator on the pointer
  [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
    return this->data.rend();
  }
  /// Get a constant reverse begin iterator on the pointer
  [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
    return this->data.crbegin();
  }
  /// Get a constant reverse end iterator on the pointer
  [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
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
    return this->data.emplace_back(std::forward<Args>(args)...);
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
  [[nodiscard]] auto
  concat(const GenericPointerTemplate<PointerT> &&other) const
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

  /// Get the size of the JSON Pointer template. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer base{"foo", "bar"};
  /// const sourcemeta::core::PointerTemplate pointer{base};
  /// assert(pointer.size() == 2);
  /// ```
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->data.size();
  }

  /// Check if a JSON Pointer template only consists in normal non-templated
  /// tokens. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::PointerTemplate pointer;
  /// pointer.emplace_back(sourcemeta::core::PointerTemplate::Wildcard::Property);
  /// pointer.emplace_back(sourcemeta::core::Pointer::Token{"foo"});
  /// assert(!pointer.trivial());
  /// ```
  [[nodiscard]] auto trivial() const noexcept -> bool {
    return std::all_of(
        this->data.cbegin(), this->data.cend(),
        [](const auto &token) { return std::holds_alternative<Token>(token); });
  }

  /// Check if a JSON Pointer template matches another JSON Pointer template.
  /// For example:
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
  /// assert(left.matches(right));
  /// assert(right.matches(left));
  /// ```
  [[nodiscard]] auto
  matches(const GenericPointerTemplate<PointerT> &other) const noexcept
      -> bool {
    // TODO: Find a way to simplify this long method
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
        break;
      } else if (*iterator_this != *iterator_that) {
        // Handle regular expressions
        if (std::holds_alternative<Token>(*iterator_this) &&
            std::holds_alternative<Regex>(*iterator_that)) {
          const auto &token{std::get<Token>(*iterator_this)};
          if (!token.is_property() ||
              !sourcemeta::core::matches_if_valid(
                  std::get<Regex>(*iterator_that), token.to_property())) {
            return false;
          }
        } else if (std::holds_alternative<Regex>(*iterator_this) &&
                   std::holds_alternative<Token>(*iterator_that)) {
          const auto &token{std::get<Token>(*iterator_that)};
          if (!token.is_property() ||
              !sourcemeta::core::matches_if_valid(
                  std::get<Regex>(*iterator_this), token.to_property())) {
            return false;
          }

          // Handle wildcards
        } else if (std::holds_alternative<Wildcard>(*iterator_this) &&
                   std::holds_alternative<Token>(*iterator_that)) {
          const auto &token{std::get<Token>(*iterator_that)};
          const auto wildcard{std::get<Wildcard>(*iterator_this)};
          if (wildcard == Wildcard::Key ||
              (wildcard == Wildcard::Property && !token.is_property()) ||
              (wildcard == Wildcard::Item && !token.is_index())) {
            return false;
          }
        } else if (std::holds_alternative<Token>(*iterator_this) &&
                   std::holds_alternative<Wildcard>(*iterator_that)) {
          const auto &token{std::get<Token>(*iterator_this)};
          const auto wildcard{std::get<Wildcard>(*iterator_that)};
          if (wildcard == Wildcard::Key ||
              (wildcard == Wildcard::Property && !token.is_property()) ||
              (wildcard == Wildcard::Item && !token.is_index())) {
            return false;
          }
        } else if (std::holds_alternative<Regex>(*iterator_this) &&
                   std::holds_alternative<Wildcard>(*iterator_that)) {
          if (std::get<Wildcard>(*iterator_that) != Wildcard::Property) {
            return false;
          }
        } else if (std::holds_alternative<Wildcard>(*iterator_this) &&
                   std::holds_alternative<Regex>(*iterator_that)) {
          if (std::get<Wildcard>(*iterator_this) != Wildcard::Property) {
            return false;
          }
        } else {
          return false;
        }
      }

      iterator_this += 1;
      iterator_that += 1;
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
