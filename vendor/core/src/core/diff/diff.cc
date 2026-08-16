#include <sourcemeta/core/diff.h>
#include <sourcemeta/core/text.h>

#include "myers.h"
#include "stringify.h"

#include <algorithm>     // std::count
#include <cassert>       // assert
#include <cstddef>       // std::size_t, std::ptrdiff_t
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace {

// A terminator at the very end closes the last line rather than opening an
// empty one, and whether it was there is what the incomplete-line marker
// reports
auto tokenise_lines(const std::string_view input,
                    std::vector<std::string_view> &lines) -> bool {
  if (input.empty()) {
    return true;
  }

  const auto ends_with_newline{input.back() == '\n'};
  const auto terminators{
      static_cast<std::size_t>(std::count(input.cbegin(), input.cend(), '\n'))};
  lines.reserve(ends_with_newline ? terminators : terminators + 1);
  sourcemeta::core::split(
      input, '\n',
      [&lines](const std::string_view line) -> void { lines.push_back(line); });
  if (ends_with_newline) {
    lines.pop_back();
  }

  return ends_with_newline;
}

// Whether the given lines of each input are the same line. The final line of an
// input without a terminator is never the same as that text followed by one
auto lines_match(const sourcemeta::core::Diff &result,
                 const std::size_t original_index,
                 const std::size_t modified_index) -> bool {
  const auto original_incomplete{!result.original_ends_with_newline &&
                                 original_index + 1 == result.original.size()};
  const auto modified_incomplete{!result.modified_ends_with_newline &&
                                 modified_index + 1 == result.modified.size()};
  return original_incomplete == modified_incomplete &&
         result.original[original_index] == result.modified[modified_index];
}

// A line that closes an input without a terminator can never equal the same
// text followed by one, so it draws its identity from a separate table
auto to_identities(
    const std::vector<std::string_view> &lines, const bool ends_with_newline,
    const std::size_t begin, const std::size_t end,
    std::unordered_map<std::string_view, std::size_t> &complete,
    std::unordered_map<std::string_view, std::size_t> &incomplete,
    std::size_t &counter, std::vector<std::size_t> &identities) -> void {
  for (auto index{begin}; index < end; ++index) {
    const auto is_incomplete{!ends_with_newline && index + 1 == lines.size()};
    auto &table{is_incomplete ? incomplete : complete};
    const auto match{table.emplace(lines[index], counter)};
    if (match.second) {
      counter += 1;
    }

    identities[index] = match.first->second;
  }
}

// Slide every run of changed lines as far towards the end of the input as the
// surrounding lines allow, which is what makes paired changes line up
auto compact_changes(const std::vector<std::string_view> &lines,
                     const bool ends_with_newline, std::vector<bool> &changed)
    -> void {
  const auto size{lines.size()};
  std::size_t index{0};

  while (index < size) {
    if (!changed[index]) {
      index += 1;
      continue;
    }

    auto group_start{index};
    auto group_end{index};
    while (group_end < size && changed[group_end]) {
      group_end += 1;
    }

    while (group_end < size && !changed[group_end] &&
           (ends_with_newline || group_end + 1 != size) &&
           lines[group_start] == lines[group_end]) {
      changed[group_start] = false;
      changed[group_end] = true;
      group_start += 1;
      group_end += 1;
    }

    index = group_end;
  }
}

auto to_operations(const std::vector<bool> &original_changed,
                   const std::vector<bool> &modified_changed)
    -> std::vector<sourcemeta::core::Diff::Operation> {
  using Type = sourcemeta::core::Diff::Operation::Type;
  std::vector<sourcemeta::core::Diff::Operation> operations;
  const auto original_size{original_changed.size()};
  const auto modified_size{modified_changed.size()};
  std::size_t original_index{0};
  std::size_t modified_index{0};

  while (original_index < original_size || modified_index < modified_size) {
    if (original_index < original_size && modified_index < modified_size &&
        !original_changed[original_index] &&
        !modified_changed[modified_index]) {
      const auto original_begin{original_index};
      const auto modified_begin{modified_index};
      while (original_index < original_size && modified_index < modified_size &&
             !original_changed[original_index] &&
             !modified_changed[modified_index]) {
        original_index += 1;
        modified_index += 1;
      }

      operations.push_back({.type = Type::Equal,
                            .original_start = original_begin,
                            .original_end = original_index,
                            .modified_start = modified_begin,
                            .modified_end = modified_index});
      continue;
    }

    const auto original_begin{original_index};
    while (original_index < original_size && original_changed[original_index]) {
      original_index += 1;
    }

    const auto modified_begin{modified_index};
    while (modified_index < modified_size && modified_changed[modified_index]) {
      modified_index += 1;
    }

    // Every unchanged line of one input pairs with an unchanged line of the
    // other, so neither cursor can stall here
    assert(original_index > original_begin || modified_index > modified_begin);

    if (original_index > original_begin) {
      operations.push_back({.type = Type::Delete,
                            .original_start = original_begin,
                            .original_end = original_index,
                            .modified_start = modified_begin,
                            .modified_end = modified_begin});
    }

    if (modified_index > modified_begin) {
      operations.push_back({.type = Type::Insert,
                            .original_start = original_index,
                            .original_end = original_index,
                            .modified_start = modified_begin,
                            .modified_end = modified_index});
    }
  }

  return operations;
}

} // namespace

namespace sourcemeta::core {

auto diff(const std::string_view original, const std::string_view modified,
          const Diff::Mode mode, const Diff::Algorithm algorithm) -> Diff {
  Diff result;

  switch (mode) {
    case Diff::Mode::Line:
      result.original_ends_with_newline =
          tokenise_lines(original, result.original);
      result.modified_ends_with_newline =
          tokenise_lines(modified, result.modified);
      break;
  }

  // Lines shared by the head and the tail of both inputs cannot take part in
  // any change, so trimming them first keeps the identity tables and the search
  // workspace proportional to the region that can actually differ rather than
  // to the size of the inputs
  const auto original_size{result.original.size()};
  const auto modified_size{result.modified.size()};
  std::size_t prefix{0};
  while (prefix < original_size && prefix < modified_size &&
         lines_match(result, prefix, prefix)) {
    prefix += 1;
  }

  const auto shortest{original_size < modified_size ? original_size
                                                    : modified_size};
  std::size_t suffix{0};
  while (suffix < shortest - prefix &&
         lines_match(result, original_size - suffix - 1,
                     modified_size - suffix - 1)) {
    suffix += 1;
  }

  const auto original_end{original_size - suffix};
  const auto modified_end{modified_size - suffix};

  std::unordered_map<std::string_view, std::size_t> complete;
  std::unordered_map<std::string_view, std::size_t> incomplete;
  complete.reserve((original_end - prefix) + (modified_end - prefix));
  std::size_t counter{0};
  std::vector<std::size_t> original_identities(original_size, 0);
  std::vector<std::size_t> modified_identities(modified_size, 0);
  to_identities(result.original, result.original_ends_with_newline, prefix,
                original_end, complete, incomplete, counter,
                original_identities);
  to_identities(result.modified, result.modified_ends_with_newline, prefix,
                modified_end, complete, incomplete, counter,
                modified_identities);

  std::vector<bool> original_changed(original_size, false);
  std::vector<bool> modified_changed(modified_size, false);

  switch (algorithm) {
    case Diff::Algorithm::Myers: {
      internal::MyersWorkspace workspace;
      // A search only needs its workspace once both sides have something left,
      // as any other case resolves to a plain insertion or deletion
      if (original_end > prefix && modified_end > prefix) {
        const auto span{(original_end - prefix) + (modified_end - prefix)};
        workspace.forward.assign((2 * span) + 3, 0);
        workspace.backward.assign((2 * span) + 3, 0);
        workspace.offset = static_cast<std::ptrdiff_t>(span + 1);
      }

      internal::myers_compare(original_identities, prefix, original_end,
                              modified_identities, prefix, modified_end,
                              original_changed, modified_changed, workspace);
      break;
    }
  }

  compact_changes(result.original, result.original_ends_with_newline,
                  original_changed);
  compact_changes(result.modified, result.modified_ends_with_newline,
                  modified_changed);

  result.operations = to_operations(original_changed, modified_changed);
  return result;
}

auto stringify(const Diff &document, std::ostream &stream,
               const Diff::Format format, const Diff::FormatOptions &options)
    -> void {
  switch (format) {
    case Diff::Format::Unified:
      internal::stringify_diff_unified(document, stream, options);
      break;
  }
}

} // namespace sourcemeta::core
