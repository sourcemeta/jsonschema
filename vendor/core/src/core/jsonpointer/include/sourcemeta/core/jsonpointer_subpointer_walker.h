#ifndef SOURCEMETA_CORE_JSONPOINTER_SUBPOINTER_WALKER_H_
#define SOURCEMETA_CORE_JSONPOINTER_SUBPOINTER_WALKER_H_

#include <cstddef>  // std::ptrdiff_t
#include <iterator> // std::forward_iterator_tag

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// A forward iterator to traverse all subpointers of a given JSON Pointer.
template <typename PointerT> class GenericSubPointerIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = PointerT;
  using pointer = value_type *;
  using reference = value_type &;

  /// Construct a subpointer iterator from a pointer to a JSON Pointer
  GenericSubPointerIterator(pointer input) : data{input} {}

  /// Get the current subpointer as a reference
  auto operator*() const -> reference { return *(this->data); }

  /// Get the current subpointer as a pointer
  auto operator->() -> pointer { return this->data; }

  /// Advance the iterator to the next subpointer
  auto operator++() -> GenericSubPointerIterator & {
    if (this->data->empty()) {
      // Turn the instance into the impossible subpointer iterator
      this->data = nullptr;
    } else {
      this->data->pop_back();
    }

    return *this;
  }

  friend auto operator==(const GenericSubPointerIterator &left,
                         const GenericSubPointerIterator &right) -> bool {
    return (!left.data && !right.data) ||
           (left.data && right.data && *(left.data) == *(right.data));
  };

private:
  pointer data;
};

/// @ingroup jsonpointer
/// A walker to traverse all subpointers of a given JSON Pointer.
template <typename PointerT> class GenericSubPointerWalker {
public:
  using const_iterator = GenericSubPointerIterator<PointerT>;
  GenericSubPointerWalker(PointerT pointer) : data{std::move(pointer)} {}

  auto begin() -> const_iterator { return {&this->data}; }
  auto end() -> const_iterator { return {nullptr}; }
  auto cbegin() -> const_iterator { return {&this->data}; }
  auto cend() -> const_iterator { return {nullptr}; }

private:
  PointerT data;
};

} // namespace sourcemeta::core

#endif
