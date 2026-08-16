#ifndef SOURCEMETA_CORE_DIFF_STRINGIFY_H_
#define SOURCEMETA_CORE_DIFF_STRINGIFY_H_

#include <sourcemeta/core/diff.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::min
#include <cstddef>     // std::size_t
#include <ios>         // std::streamsize
#include <ostream>     // std::ostream
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core::internal {

inline auto write_diff_text(std::ostream &stream, const std::string_view value)
    -> void {
  stream.write(value.data(), static_cast<std::streamsize>(value.size()));
}

inline auto write_diff_range(std::ostream &stream, const std::size_t start,
                             const std::size_t count) -> void {
  digits_write(stream, count == 0 ? start : start + 1);
  if (count != 1) {
    stream.put(',');
    digits_write(stream, count);
  }
}

inline auto write_diff_line(std::ostream &stream, const char prefix,
                            const std::vector<std::string_view> &lines,
                            const std::size_t index,
                            const bool ends_with_newline) -> void {
  stream.put(prefix);
  write_diff_text(stream, lines[index]);
  stream.put('\n');
  if (!ends_with_newline && index + 1 == lines.size()) {
    write_diff_text(stream, "\\ No newline at end of file\n");
  }
}

inline auto stringify_diff_unified(const Diff &document, std::ostream &stream,
                                   const Diff::FormatOptions &options) -> void {
  const auto &operations{document.operations};
  const auto size{operations.size()};
  const auto context{options.context};
  bool wrote_header{false};
  std::size_t index{0};

  while (index < size) {
    while (index < size &&
           operations[index].type == Diff::Operation::Type::Equal) {
      index += 1;
    }

    if (index == size) {
      break;
    }

    const auto change_begin{index};
    auto change_end{index + 1};
    auto cursor{index + 1};

    while (cursor < size) {
      if (operations[cursor].type != Diff::Operation::Type::Equal) {
        change_end = cursor + 1;
        cursor += 1;
        continue;
      }

      const auto length{operations[cursor].original_end -
                        operations[cursor].original_start};
      if (cursor + 1 < size && length <= 2 * context) {
        cursor += 1;
        continue;
      }

      break;
    }

    const auto leading{
        change_begin == 0
            ? std::size_t{0}
            : std::min(context,
                       operations[change_begin - 1].original_end -
                           operations[change_begin - 1].original_start)};
    const auto trailing{
        change_end == size
            ? std::size_t{0}
            : std::min(context, operations[change_end].original_end -
                                    operations[change_end].original_start)};

    const auto original_start{operations[change_begin].original_start -
                              leading};
    const auto modified_start{operations[change_begin].modified_start -
                              leading};
    const auto original_end{operations[change_end - 1].original_end + trailing};
    const auto modified_end{operations[change_end - 1].modified_end + trailing};

    if (!wrote_header) {
      write_diff_text(stream, "--- ");
      write_diff_text(stream, options.original_label);
      stream.put('\n');
      write_diff_text(stream, "+++ ");
      write_diff_text(stream, options.modified_label);
      stream.put('\n');
      wrote_header = true;
    }

    write_diff_text(stream, "@@ -");
    write_diff_range(stream, original_start, original_end - original_start);
    write_diff_text(stream, " +");
    write_diff_range(stream, modified_start, modified_end - modified_start);
    write_diff_text(stream, " @@\n");

    for (auto line{original_start};
         line < operations[change_begin].original_start; ++line) {
      write_diff_line(stream, ' ', document.original, line,
                      document.original_ends_with_newline);
    }

    for (auto position{change_begin}; position < change_end; ++position) {
      const auto &operation{operations[position]};
      switch (operation.type) {
        case Diff::Operation::Type::Equal:
          for (auto line{operation.original_start};
               line < operation.original_end; ++line) {
            write_diff_line(stream, ' ', document.original, line,
                            document.original_ends_with_newline);
          }

          break;
        case Diff::Operation::Type::Delete:
          for (auto line{operation.original_start};
               line < operation.original_end; ++line) {
            write_diff_line(stream, '-', document.original, line,
                            document.original_ends_with_newline);
          }

          break;
        case Diff::Operation::Type::Insert:
          for (auto line{operation.modified_start};
               line < operation.modified_end; ++line) {
            write_diff_line(stream, '+', document.modified, line,
                            document.modified_ends_with_newline);
          }

          break;
      }
    }

    for (auto line{operations[change_end - 1].original_end};
         line < original_end; ++line) {
      write_diff_line(stream, ' ', document.original, line,
                      document.original_ends_with_newline);
    }

    index = change_end;
  }
}

} // namespace sourcemeta::core::internal

#endif
