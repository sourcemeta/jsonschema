#ifndef SOURCEMETA_JSONTOOLKIT_JSONPOINTER_SUBPOINTER_WALKER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONPOINTER_SUBPOINTER_WALKER_H_

#include <cstddef>  // std::ptrdiff_t
#include <iterator> // std::forward_iterator_tag

#include <sourcemeta/jsontoolkit/jsonpointer_pointer.h>

namespace sourcemeta::jsontoolkit {

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
  reference operator*() const { return *(this->data); }

  /// Get the current subpointer as a pointer
  pointer operator->() { return this->data; }

  /// Advance the iterator to the next subpointer
  GenericSubPointerIterator &operator++() {
    if (this->data->empty()) {
      // Turn the instance into the impossible subpointer iterator
      this->data = nullptr;
    } else {
      this->data->pop_back();
    }

    return *this;
  }

  friend bool operator==(const GenericSubPointerIterator &left,
                         const GenericSubPointerIterator &right) {
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
  GenericSubPointerWalker(const PointerT &pointer) : data{pointer} {}

  const_iterator begin() { return {&this->data}; }
  const_iterator end() { return {nullptr}; }
  const_iterator cbegin() { return {&this->data}; }
  const_iterator cend() { return {nullptr}; }

private:
  PointerT data;
};

} // namespace sourcemeta::jsontoolkit

#endif
