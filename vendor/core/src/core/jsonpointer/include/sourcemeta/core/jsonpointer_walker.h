#ifndef SOURCEMETA_CORE_JSONPOINTER_WALKER_H_
#define SOURCEMETA_CORE_JSONPOINTER_WALKER_H_

#include <sourcemeta/core/json.h>

#include <algorithm> // std::reverse
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <utility>   // std::pair, std::move
#include <vector>    // std::vector

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// A walker to get every JSON Pointer in a JSON document. Note that no specific
/// ordering is guaranteed. If you expect any ordering, sort afterwards.
template <typename PointerT> class GenericPointerWalker {
private:
  using internal = typename std::vector<PointerT>;

public:
  /// Construct a walker over every location in a JSON document
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
    // Traversal is iterative with an explicit stack so that a deeply nested
    // document cannot overflow the call stack. The output ordering is
    // unspecified either way
    std::vector<std::pair<const JSON *, PointerT>> pending;
    pending.emplace_back(&document, pointer);
    while (!pending.empty()) {
      auto entry{std::move(pending.back())};
      pending.pop_back();
      const JSON &node{*entry.first};
      // Children are queued then reversed so that they are popped in their
      // natural order, preserving the pre-order traversal of the recursive form
      const auto start{pending.size()};
      if (node.is_array()) {
        for (std::size_t index = 0; index < node.size(); index++) {
          PointerT child{entry.second};
          child.emplace_back(index);
          pending.emplace_back(&node.at(index), std::move(child));
        }
      } else if (node.is_object()) {
        for (const auto &pair : node.as_object()) {
          PointerT child{entry.second};
          child.emplace_back(pair.first);
          pending.emplace_back(&pair.second, std::move(child));
        }
      }

      std::reverse(pending.begin() + static_cast<std::ptrdiff_t>(start),
                   pending.end());
      this->pointers.push_back(std::move(entry.second));
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
