#ifndef SOURCEMETA_CORE_JSONPOINTER_POINTER_H_
#define SOURCEMETA_CORE_JSONPOINTER_POINTER_H_

#include <sourcemeta/core/jsonpointer_token.h>

#include <algorithm>        // std::copy, std::equal
#include <cassert>          // assert
#include <cstddef>          // std::size_t
#include <functional>       // std::reference_wrapper
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::advance, std::back_inserter
#include <type_traits>      // std::enable_if_t, std::is_same_v, std::false_type
#include <utility>          // std::move
#include <vector>           // std::vector

namespace sourcemeta::core {

/// @ingroup jsonpointer
template <typename PropertyT, typename Hash> class GenericPointer {
public:
  using Token = GenericToken<PropertyT, Hash>;
  using Value = typename Token::Value;
  using Container = std::vector<Token>;
  // We manually provide a JSON transformer
  using json_auto = std::false_type;

  /// This constructor creates an empty JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer;
  /// assert(pointer.empty());
  /// ```
  GenericPointer() : data{} {}

  /// This constructor is the preferred way of creating a pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// // Equivalent to /foo/bar/1
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", 1};
  /// assert(pointer.size() == 3);
  /// ```
  GenericPointer(std::initializer_list<Token> tokens)
      : data{std::move(tokens)} {}

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

  /// Access a token in a JSON Pointer at a given index.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", 1};
  /// assert(pointer.at(1).is_property());
  /// assert(pointer.at(1).to_property() == "bar");
  /// ```
  [[nodiscard]] auto at(const size_type index) const -> const_reference {
    assert(this->size() > index);
    return this->data[index];
  }

  /// Access the last token in a JSON Pointer
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", 1};
  /// assert(pointer.back().is_property());
  /// assert(pointer.back().to_property() == "bar");
  /// ```
  [[nodiscard]] auto back() const -> const_reference {
    assert(!this->empty());
    return this->data.back();
  }

  /// Get the number of tokens in a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar"};
  /// assert(pointer.size() == 2);
  /// ```
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->data.size();
  }

  /// Check if a JSON Pointer is the empty pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer empty_pointer;
  /// const sourcemeta::core::Pointer non_empty_pointer{"foo", "bar"};
  /// assert(empty_pointer.empty());
  /// assert(!non_empty_pointer.empty());
  /// ```
  [[nodiscard]] auto empty() const noexcept -> bool {
    return this->data.empty();
  }

  /// Emplace a token into the back of a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer;
  /// assert(pointer.empty());
  /// auto &token{pointer.emplace_back("foo")};
  /// assert(!pointer.empty());
  /// assert(token.is_property());
  /// ```
  template <class... Args> auto emplace_back(Args &&...args) -> reference {
    return this->data.emplace_back(std::forward<Args>(args)...);
  }

  /// Reserve capacity for a JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  ///
  /// sourcemeta::core::Pointer pointer;
  /// pointer.reserve(1024);
  /// ```
  auto reserve(const typename Container::size_type capacity) -> void {
    this->data.reserve(capacity);
  }

  /// Push a copy of a JSON Pointer into the back of a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer{"foo"};
  /// const sourcemeta::core::Pointer other{"bar", "baz"};
  /// pointer.push_back(other);
  /// assert(pointer.size() == 3);
  ///
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(1).is_property());
  /// assert(pointer.at(2).is_property());
  ///
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).to_property() == "bar");
  /// assert(pointer.at(2).to_property() == "baz");
  /// ```
  auto push_back(const GenericPointer<PropertyT, Hash> &other) -> void {
    if (other.empty()) {
      return;
    } else if (other.size() == 1) {
      this->emplace_back(other.back());
      return;
    }

    this->reserve(this->data.size() + other.size());
    std::copy(other.data.cbegin(), other.data.cend(),
              std::back_inserter(this->data));
  }

  /// Move a JSON Pointer into the back of a JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  /// #include <utility>
  ///
  /// sourcemeta::core::Pointer pointer{"foo"};
  /// sourcemeta::core::Pointer other{"bar", "baz"};
  /// pointer.push_back(std::move(other));
  /// assert(pointer.size() == 3);
  ///
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(1).is_property());
  /// assert(pointer.at(2).is_property());
  ///
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).to_property() == "bar");
  /// assert(pointer.at(2).to_property() == "baz");
  /// ```
  auto push_back(GenericPointer<PropertyT, Hash> &&other) -> void {
    if (other.empty()) {
      return;
    } else if (other.size() == 1) {
      this->emplace_back(std::move(other.back()));
      return;
    }

    this->reserve(this->data.size() + other.size());
    std::move(other.data.begin(), other.data.end(),
              std::back_inserter(this->data));
  }

  /// Push a JSON Pointer into the back of a JSON WeakPointer. Make sure that
  /// the pointer you are pushing remains alive for the duration of the
  /// WeakPointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const std::string foo{"foo"};
  /// sourcemeta::core::WeakPointer pointer{std::cref(foo)};
  /// const sourcemeta::core::Pointer other{"bar", "baz"};
  /// pointer.push_back(other);
  /// assert(pointer.size() == 3);
  ///
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(1).is_property());
  /// assert(pointer.at(2).is_property());
  ///
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).to_property() == "bar");
  /// assert(pointer.at(2).to_property() == "baz");
  /// ```
  template <typename OtherT>
  auto push_back(const GenericPointer<OtherT, Hash> &other) -> void
    requires std::is_same_v<PropertyT, std::reference_wrapper<const OtherT>>
  {
    if (other.empty()) {
      return;
    } else if (other.size() == 1) {
      const auto &token{other.back()};
      if (token.is_property()) {
        // We should make sure to re-use the existing hash
        this->data.emplace_back(token.to_property(), token.property_hash());
      } else {
        this->data.emplace_back(token.to_index());
      }
    } else {
      this->reserve(this->data.size() + other.size());
      for (const auto &token : other) {
        if (token.is_property()) {
          // We should make sure to re-use the existing hash
          this->data.emplace_back(token.to_property(), token.property_hash());
        } else {
          this->data.emplace_back(token.to_index());
        }
      }
    }
  }

  /// Push a property token into the back of a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer{"foo"};
  /// const sourcemeta::core::Pointer other{"bar"};
  /// pointer.push_back(other.back().to_property());
  /// assert(pointer.size() == 2);
  ///
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(1).is_property());
  ///
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).to_property() == "bar");
  /// ```
  auto push_back(const typename Token::Property &property) -> void {
    this->data.emplace_back(property);
  }

  /// Move a property token into the back of a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer{"foo"};
  /// pointer.push_back("bar");
  /// assert(pointer.size() == 2);
  ///
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(1).is_property());
  ///
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).to_property() == "bar");
  /// ```
  auto push_back(typename Token::Property &&property) -> void {
    this->data.emplace_back(std::move(property));
  }

  /// Push an index token into the back of a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer{"foo"};
  /// const sourcemeta::core::Pointer other{0};
  /// pointer.push_back(other.back().to_index());
  /// assert(pointer.size() == 2);
  ///
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(1).is_index());
  ///
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).to_index() == 0);
  /// ```
  auto push_back(const typename Token::Index &index) -> void {
    this->data.emplace_back(index);
  }

  /// Remove the last token of a JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer{"foo", "bar"};
  /// pointer.pop_back();
  /// assert(pointer.size() == 1);
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(0).to_property() == "foo");
  /// ```
  auto pop_back() -> void {
    assert(!this->empty());
    this->data.pop_back();
  }

  /// Remove a number of tokens from the back of a JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::Pointer pointer{"foo", "bar", "baz"};
  /// pointer.pop_back(2);
  /// assert(pointer.size() == 1);
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(0).to_property() == "foo");
  /// ```
  auto pop_back(const size_type count) -> void {
    assert(this->size() >= count);
    for (std::size_t index = 0; index < count; index++) {
      this->data.pop_back();
    }
  }

  /// Get a copy of the JSON Pointer including every token except the last. This
  /// method is undefined if the JSON Pointer is empty. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::core::Pointer result{pointer.initial()};
  /// assert(pointer.size() == 2);
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).is_property());
  /// assert(pointer.at(1).to_property() == "bar");
  /// ```
  [[nodiscard]] auto initial() const -> GenericPointer<PropertyT, Hash> {
    assert(!this->empty());
    GenericPointer<PropertyT, Hash> result{*this};
    result.pop_back();
    return result;
  }

  /// Concatenate a JSON Pointer with another JSON Pointer, getting a new
  /// pointer as a result. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer left{"foo"};
  /// const sourcemeta::core::Pointer right{"bar", "baz"};
  /// assert(left.concat(right) ==
  ///   sourcemeta::core::Pointer{"foo", "bar", "baz"});
  /// ```
  [[nodiscard]] auto concat(const GenericPointer<PropertyT, Hash> &other) const
      -> GenericPointer<PropertyT, Hash> {
    GenericPointer<PropertyT, Hash> result{*this};
    result.push_back(other);
    return result;
  }

  /// Check whether a JSON Pointer starts with another JSON Pointer. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::core::Pointer prefix{"foo", "bar"};
  /// assert(pointer.starts_with(prefix));
  /// ```
  [[nodiscard]] auto
  starts_with(const GenericPointer<PropertyT, Hash> &other) const -> bool {
    return other.data.size() <= this->data.size() &&
           std::equal(other.data.cbegin(), other.data.cend(),
                      this->data.cbegin());
  }

  /// Check whether a JSON Pointer plus a given tail starts with another JSON
  /// Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar"};
  /// const sourcemeta::core::Pointer::Token tail{"baz"};
  /// const sourcemeta::core::Pointer prefix{"foo", "bar", "baz"};
  /// assert(pointer.starts_with(prefix, tail));
  /// ```
  [[nodiscard]] auto starts_with(const GenericPointer<PropertyT, Hash> &other,
                                 const Token &tail) const -> bool {
    if (other.size() == this->size() + 1) {
      assert(!other.empty());
      return other.starts_with(*this) && other.back() == tail;
    } else {
      return this->starts_with(other);
    }
  }

  /// Check whether a JSON Pointer starts with another JSON Pointer followed
  /// by a property token. This is useful for checking container membership
  /// without allocating a new pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "$defs", "bar"};
  /// const sourcemeta::core::Pointer prefix{"foo"};
  /// assert(pointer.starts_with(prefix, "$defs"));
  /// assert(!pointer.starts_with(prefix, "other"));
  /// ```
  template <typename StringT>
    requires(!std::is_same_v<std::decay_t<StringT>, Token>)
  [[nodiscard]] auto starts_with(const GenericPointer<PropertyT, Hash> &other,
                                 const StringT &tail) const -> bool {
    const auto prefix_size{other.size()};
    return this->size() > prefix_size && this->starts_with(other) &&
           this->data[prefix_size].is_property() &&
           this->data[prefix_size].to_property() == tail;
  }

  /// Check whether a JSON Pointer starts with another JSON Pointer followed
  /// by two property tokens. This is useful for checking nested container
  /// membership without allocating a new pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "$defs", "bar", "baz"};
  /// const sourcemeta::core::Pointer prefix{"foo"};
  /// assert(pointer.starts_with(prefix, "$defs", "bar"));
  /// assert(!pointer.starts_with(prefix, "$defs", "other"));
  /// ```
  template <typename StringLeftT, typename StringRightT>
    requires(!std::is_same_v<std::decay_t<StringLeftT>, Token> &&
             !std::is_same_v<std::decay_t<StringRightT>, Token>)
  [[nodiscard]] auto starts_with(const GenericPointer<PropertyT, Hash> &other,
                                 const StringLeftT &tail_left,
                                 const StringRightT &tail_right) const -> bool {
    const auto prefix_size{other.size()};
    return this->size() > prefix_size + 1 &&
           this->starts_with(other, tail_left) &&
           this->data[prefix_size + 1].is_property() &&
           this->data[prefix_size + 1].to_property() == tail_right;
  }

  /// Check whether a JSON Pointer starts with the initial part of another JSON
  /// Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::core::Pointer prefix{"foo", "bar", "qux"};
  /// assert(pointer.starts_with_initial(prefix));
  /// ```
  [[nodiscard]] auto
  starts_with_initial(const GenericPointer<PropertyT, Hash> &other) const
      -> bool {
    const auto prefix_size{other.size()};
    if (prefix_size == 0) {
      return true;
    } else if (this->size() < prefix_size - 1) {
      return false;
    }

    for (std::size_t index = 0; index < prefix_size - 1; index++) {
      if (this->data[index] != other.data[index]) {
        return false;
      }
    }

    return true;
  }

  /// Replace a base of a JSON Pointer with another JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::core::Pointer prefix{"foo", "bar"};
  /// const sourcemeta::core::Pointer replacement{"qux"};
  ///
  /// assert(pointer.rebase(prefix, replacement) ==
  ///   sourcemeta::core::Pointer{"qux", "baz"});
  /// ```
  [[nodiscard]] auto
  rebase(const GenericPointer<PropertyT, Hash> &prefix,
         const GenericPointer<PropertyT, Hash> &replacement) const
      -> GenericPointer<PropertyT, Hash> {
    typename Container::size_type index{0};
    while (index < prefix.size()) {
      if (index >= this->size() || prefix.data[index] != this->data[index]) {
        return *this;
      } else {
        index++;
      }
    }

    assert(index == prefix.size());
    assert(this->starts_with(prefix));
    auto new_begin{this->data.cbegin()};
    std::advance(new_begin, index);
    GenericPointer<PropertyT, Hash> result{replacement};
    std::copy(new_begin, this->data.cend(), std::back_inserter(result.data));
    return result;
  }

  /// Resolve a JSON Pointer relative to another JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::Pointer pointer{"foo", "bar"};
  /// const sourcemeta::core::Pointer base{"foo"};
  /// assert(pointer.resolve_from(base) ==
  ///   sourcemeta::core::Pointer{"bar"});
  /// ```
  ///
  /// If the JSON Pointer is not relative to the base, a copy of the original
  /// input pointer is returned.
  [[nodiscard]] auto
  resolve_from(const GenericPointer<PropertyT, Hash> &base) const
      -> GenericPointer<PropertyT, Hash> {
    typename Container::size_type index{0};
    while (index < base.size()) {
      if (index >= this->size() || base.data[index] != this->data[index]) {
        return *this;
      } else {
        index++;
      }
    }

    // Make a pointer from the remaining tokens
    auto new_begin{this->data.cbegin()};
    std::advance(new_begin, index);
    GenericPointer<PropertyT, Hash> result;
    std::copy(new_begin, this->data.cend(), std::back_inserter(result.data));
    return result;
  }

  /// Compare JSON Pointer instances
  [[nodiscard]] auto
  operator==(const GenericPointer<PropertyT, Hash> &other) const noexcept
      -> bool {
    return this->data == other.data;
  }

  /// Overload to support ordering of JSON Pointers. Typically for sorting
  /// reasons.
  [[nodiscard]] auto
  operator<(const GenericPointer<PropertyT, Hash> &other) const noexcept
      -> bool {
    return this->data < other.data;
  }

private:
  Container data;
};

} // namespace sourcemeta::core

#endif
