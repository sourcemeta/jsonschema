#ifndef SOURCEMETA_CORE_JSONPOINTER_WALKER_H_
#define SOURCEMETA_CORE_JSONPOINTER_WALKER_H_

#include <sourcemeta/core/json.h>

#include <cstddef> // std::size_t
#include <vector>  // std::vector

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// A walker to get every JSON Pointer in a JSON document. Note that no specific
/// ordering is guaranteed. If you expect any ordering, sort afterwards.
template <typename PointerT> class GenericPointerWalker {
private:
  using internal = typename std::vector<PointerT>;

public:
  GenericPointerWalker(const JSON &document) {
    PointerT accumulator;
    this->walk(document, accumulator);
  }

  using const_iterator = typename internal::const_iterator;
  [[nodiscard]] auto begin() const -> const_iterator {
    return this->pointers.begin();
  };
  [[nodiscard]] auto end() const -> const_iterator {
    return this->pointers.end();
  };
  [[nodiscard]] auto cbegin() const -> const_iterator {
    return this->pointers.cbegin();
  };
  [[nodiscard]] auto cend() const -> const_iterator {
    return this->pointers.cend();
  };

private:
  auto walk(const JSON &document, PointerT &pointer) -> void {
    this->pointers.push_back(pointer);
    if (document.is_array()) {
      for (std::size_t index = 0; index < document.size(); index++) {
        pointer.emplace_back(index);
        this->walk(document.at(index), pointer);
        pointer.pop_back();
      }
    } else if (document.is_object()) {
      for (const auto &pair : document.as_object()) {
        pointer.emplace_back(pair.first);
        this->walk(pair.second, pointer);
        pointer.pop_back();
      }
    }
  }

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  internal pointers;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
