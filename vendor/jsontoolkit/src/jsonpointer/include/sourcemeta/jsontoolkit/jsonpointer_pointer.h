#ifndef SOURCEMETA_JSONTOOLKIT_JSONPOINTER_POINTER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONPOINTER_POINTER_H_

#include <sourcemeta/jsontoolkit/jsonpointer_token.h>

#include <algorithm>        // std::copy, std::equal
#include <cassert>          // assert
#include <functional>       // std::reference_wrapper
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::advance, std::back_inserter
#include <sstream>          // std::basic_ostringstream
#include <stdexcept>        // std::runtime_error
#include <type_traits>      // std::enable_if_t, std::is_same_v
#include <utility>          // std::move
#include <vector>           // std::vector

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonpointer
template <typename PropertyT> class GenericPointer {
public:
  using Token = GenericToken<PropertyT, Hash>;
  using Value = typename Token::Value;
  using Container = std::vector<Token>;

  /// This constructor creates an empty JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer;
  /// assert(pointer.empty());
  /// ```
  GenericPointer() : data{} {}

  /// This constructor is the preferred way of creating a pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// // Equivalent to /foo/bar/1
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", 1};
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

  /// Access a token in a JSON Pointer at a given index.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", 1};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", 1};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar"};
  /// assert(pointer.size() == 2);
  /// ```
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->data.size();
  }

  /// Check if a JSON Pointer is the empty pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer empty_pointer;
  /// const sourcemeta::jsontoolkit::Pointer non_empty_pointer{"foo", "bar"};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer;
  /// assert(pointer.empty());
  /// auto &token{pointer.emplace_back("foo")};
  /// assert(!pointer.empty());
  /// assert(token.is_property());
  /// ```
  template <class... Args> auto emplace_back(Args &&...args) -> reference {
    return this->data.emplace_back(args...);
  }

  /// Reserve capacity for a JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer;
  /// pointer.reserve(1024);
  /// ```
  auto reserve(const typename Container::size_type capacity) -> void {
    this->data.reserve(capacity);
  }

  /// Push a copy of a JSON Pointer into the back of a JSON Pointer.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo"};
  /// const sourcemeta::jsontoolkit::Pointer other{"bar", "baz"};
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
  auto push_back(const GenericPointer<PropertyT> &other) -> void {
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  /// #include <utility>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo"};
  /// sourcemeta::jsontoolkit::Pointer other{"bar", "baz"};
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
  auto push_back(GenericPointer<PropertyT> &&other) -> void {
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const std::string foo{"foo"};
  /// sourcemeta::jsontoolkit::WeakPointer pointer{std::cref(foo)};
  /// const sourcemeta::jsontoolkit::Pointer other{"bar", "baz"};
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
  template <typename OtherT,
            typename = std::enable_if_t<std::is_same_v<
                PropertyT, std::reference_wrapper<const OtherT>>>>
  auto push_back(const GenericPointer<OtherT> &other) -> void {
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo"};
  /// const sourcemeta::jsontoolkit::Pointer other{"bar"};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo"};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo"};
  /// const sourcemeta::jsontoolkit::Pointer other{0};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar"};
  /// pointer.pop_back();
  /// assert(pointer.size() == 1);
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(0).to_property() == "foo");
  /// ```
  ///
  /// If the JSON Pointer is empty, this method throws `std::runtime_error`.
  auto pop_back() -> void {
    if (this->empty()) {
      throw std::runtime_error("Cannot pop an empty Pointer");
    }

    this->data.pop_back();
  }

  /// Remove a number of tokens from the back of a JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", "baz"};
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::jsontoolkit::Pointer result{pointer.initial()};
  /// assert(pointer.size() == 2);
  /// assert(pointer.at(0).is_property());
  /// assert(pointer.at(0).to_property() == "foo");
  /// assert(pointer.at(1).is_property());
  /// assert(pointer.at(1).to_property() == "bar");
  /// ```
  [[nodiscard]] auto initial() const -> GenericPointer<PropertyT> {
    assert(!this->empty());
    GenericPointer<PropertyT> result{*this};
    result.pop_back();
    return result;
  }

  /// Concatenate a JSON Pointer with another JSON Pointer, getting a new
  /// pointer as a result. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer left{"foo"};
  /// const sourcemeta::jsontoolkit::Pointer right{"bar", "baz"};
  /// assert(left.concat(right) ==
  ///   sourcemeta::jsontoolkit::Pointer{"foo", "bar", "baz"});
  /// ```
  auto concat(const GenericPointer<PropertyT> &other) const
      -> GenericPointer<PropertyT> {
    GenericPointer<PropertyT> result{*this};
    result.push_back(other);
    return result;
  }

  /// Check whether a JSON Pointer starts with another JSON Pointer. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::jsontoolkit::Pointer prefix{"foo", "bar"};
  /// assert(pointer.starts_with(prefix));
  /// ```
  auto starts_with(const GenericPointer<PropertyT> &other) const -> bool {
    return other.data.size() <= this->data.size() &&
           std::equal(other.data.cbegin(), other.data.cend(),
                      this->data.cbegin());
  }

  /// Check whether a JSON Pointer plus a given tail starts with another JSON
  /// Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar"};
  /// const sourcemeta::jsontoolkit::Pointer::Token tail{"baz"};
  /// const sourcemeta::jsontoolkit::Pointer prefix{"foo", "bar", "baz"};
  /// assert(pointer.starts_with(prefix, tail));
  /// ```
  auto starts_with(const GenericPointer<PropertyT> &other,
                   const Token &tail) const -> bool {
    if (other.size() == this->size() + 1) {
      assert(!other.empty());
      return other.starts_with(*this) && other.back() == tail;
    } else {
      return this->starts_with(other);
    }
  }

  /// Check whether a JSON Pointer starts with the initial part of another JSON
  /// Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::jsontoolkit::Pointer prefix{"foo", "bar", "qux"};
  /// assert(pointer.starts_with_initial(prefix));
  /// ```
  auto starts_with_initial(const GenericPointer<PropertyT> &other) const
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
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar", "baz"};
  /// const sourcemeta::jsontoolkit::Pointer prefix{"foo", "bar"};
  /// const sourcemeta::jsontoolkit::Pointer replacement{"qux"};
  ///
  /// assert(pointer.rebase(prefix, replacement) ==
  ///   sourcemeta::jsontoolkit::Pointer{"qux", "baz"});
  /// ```
  auto rebase(const GenericPointer<PropertyT> &prefix,
              const GenericPointer<PropertyT> &replacement) const
      -> GenericPointer<PropertyT> {
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
    GenericPointer<PropertyT> result{replacement};
    std::copy(new_begin, this->data.cend(), std::back_inserter(result.data));
    return result;
  }

  /// Resolve a JSON Pointer relative to another JSON Pointer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonpointer.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::jsontoolkit::Pointer pointer{"foo", "bar"};
  /// const sourcemeta::jsontoolkit::Pointer base{"foo"};
  /// assert(pointer.resolve_from(base) ==
  ///   sourcemeta::jsontoolkit::Pointer{"bar"});
  /// ```
  ///
  /// If the JSON Pointer is not relative to the base, a copy of the original
  /// input pointer is returned.
  auto resolve_from(const GenericPointer<PropertyT> &base) const
      -> GenericPointer<PropertyT> {
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
    GenericPointer<PropertyT> result;
    std::copy(new_begin, this->data.cend(), std::back_inserter(result.data));
    return result;
  }

  /// Compare JSON Pointer instances
  auto operator==(const GenericPointer<PropertyT> &other) const noexcept
      -> bool {
    return this->data == other.data;
  }

  /// Overload to support ordering of JSON Pointers. Typically for sorting
  /// reasons.
  auto operator<(const GenericPointer<PropertyT> &other) const noexcept
      -> bool {
    return this->data < other.data;
  }

private:
  Container data;
};

} // namespace sourcemeta::jsontoolkit

#endif
