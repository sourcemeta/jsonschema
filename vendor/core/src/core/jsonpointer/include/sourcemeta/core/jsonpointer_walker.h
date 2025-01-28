#ifndef SOURCEMETA_CORE_JSONPOINTER_WALKER_H_
#define SOURCEMETA_CORE_JSONPOINTER_WALKER_H_

#include <vector> // std::vector

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer_pointer.h>

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// A walker to get every JSON Pointer in a JSON document. Note that no specific
/// ordering is guaranteed. If you expect any ordering, sort afterwards.
template <typename PointerT> class GenericPointerWalker {
private:
  using internal = typename std::vector<PointerT>;

public:
  GenericPointerWalker(const JSON &document) { this->walk(document, {}); }

  using const_iterator = typename internal::const_iterator;
  auto begin() const -> const_iterator { return this->pointers.begin(); };
  auto end() const -> const_iterator { return this->pointers.end(); };
  auto cbegin() const -> const_iterator { return this->pointers.cbegin(); };
  auto cend() const -> const_iterator { return this->pointers.cend(); };

private:
  auto walk(const JSON &document, const PointerT &pointer) -> void {
    this->pointers.push_back(pointer);
    if (document.is_array()) {
      for (std::size_t index = 0; index < document.size(); index++) {
        PointerT subpointer{pointer};
        subpointer.emplace_back(index);
        this->walk(document.at(index), subpointer);
      }
    } else if (document.is_object()) {
      for (const auto &pair : document.as_object()) {
        PointerT subpointer{pointer};
        subpointer.emplace_back(pair.first);
        this->walk(pair.second, subpointer);
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
