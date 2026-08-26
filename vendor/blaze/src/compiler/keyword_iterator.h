#ifndef SOURCEMETA_BLAZE_COMPILER_KEYWORD_ITERATOR_H_
#define SOURCEMETA_BLAZE_COMPILER_KEYWORD_ITERATOR_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/blaze/foundation.h>

#include <algorithm>   // std::max, std::ranges::fold_left, std::ranges::sort
#include <cassert>     // assert
#include <cstdint>     // std::uint64_t
#include <functional>  // std::cref
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace sourcemeta::blaze {

/// A top-level keyword as reported by SchemaKeywordIterator
struct KeywordEntry {
  sourcemeta::core::WeakPointer pointer;
};

inline auto schema_keyword_priority(const std::string_view keyword,
                                    const Vocabularies &vocabularies,
                                    const SchemaWalker &walker)
    -> std::uint64_t {
  const auto &result{walker(keyword, vocabularies)};
  const auto priority_from_dependencies{std::ranges::fold_left(
      result.dependencies, static_cast<std::uint64_t>(0),
      [&vocabularies, &walker](const auto accumulator,
                               const auto &dependency) -> std::uint64_t {
        return std::max(
            accumulator,
            schema_keyword_priority(dependency, vocabularies, walker) + 1);
      })};
  const auto priority_from_order_dependencies{std::ranges::fold_left(
      result.order_dependencies, static_cast<std::uint64_t>(0),
      [&vocabularies, &walker](const auto accumulator,
                               const auto &dependency) -> std::uint64_t {
        return std::max(
            accumulator,
            schema_keyword_priority(dependency, vocabularies, walker) + 1);
      })};
  return std::max(priority_from_dependencies, priority_from_order_dependencies);
}

/// Iterate over the top-level keywords of a schema in evaluation order
class SchemaKeywordIterator {
private:
  using internal = typename std::vector<KeywordEntry>;

public:
  using const_iterator = typename internal::const_iterator;
  SchemaKeywordIterator(const sourcemeta::core::JSON &schema,
                        const SchemaWalker &walker,
                        const Vocabularies &vocabularies);
  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

private:
  internal entries{};
};

// TODO: This iterator is not very efficient. It traverses once on
// construction and then the client traverses again.

inline SchemaKeywordIterator::SchemaKeywordIterator(
    const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
    const Vocabularies &vocabularies) {
  assert(is_schema(schema));
  if (schema.is_boolean()) {
    return;
  }

  // TODO: Use std::ranges::to<std::vector>() once libc++ supports it
  // (__cpp_lib_ranges_to_container)
  for (const auto &entry : schema.as_object()) {
    sourcemeta::core::WeakPointer entry_pointer;
    entry_pointer.push_back(std::cref(entry.first));
    KeywordEntry keyword_entry{.pointer = std::move(entry_pointer)};
    this->entries.push_back(std::move(keyword_entry));
  }

  // Sort keywords based on priority for correct evaluation
  std::ranges::sort(
      this->entries,
      [&vocabularies, &walker](const auto &left, const auto &right) -> bool {
        // These cannot be empty or indexes, as we created
        // the entries array from a JSON object
        assert(!left.pointer.empty() && left.pointer.back().is_property());
        assert(!right.pointer.empty() && right.pointer.back().is_property());

        const auto left_priority = schema_keyword_priority(
            left.pointer.back().to_property(), vocabularies, walker);
        const auto right_priority = schema_keyword_priority(
            right.pointer.back().to_property(), vocabularies, walker);

        // Sort first on priority, second on actual keywords. The latter is to
        // make sure different compilers with different STL implementations end
        // up at the exact same result. Not really mandatory, but useful for
        // writing tests on the iterator output.
        if (left_priority != right_priority) {
          return left_priority < right_priority;
        } else {
          return left.pointer < right.pointer;
        }
      });
}

inline auto SchemaKeywordIterator::begin() const -> const_iterator {
  return this->entries.begin();
}
inline auto SchemaKeywordIterator::end() const -> const_iterator {
  return this->entries.end();
}
inline auto SchemaKeywordIterator::cbegin() const -> const_iterator {
  return this->entries.cbegin();
}
inline auto SchemaKeywordIterator::cend() const -> const_iterator {
  return this->entries.cend();
}

} // namespace sourcemeta::blaze

#endif
