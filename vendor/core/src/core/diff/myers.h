#ifndef SOURCEMETA_CORE_DIFF_MYERS_H_
#define SOURCEMETA_CORE_DIFF_MYERS_H_

#include <cassert> // assert
#include <cstddef> // std::size_t, std::ptrdiff_t
#include <vector>  // std::vector

namespace sourcemeta::core::internal {

/// The diagonal run of matching tokens at which a forward and a backward
/// search meet, expressed as half-open ranges into both inputs
struct MyersSnake {
  std::size_t original_start;
  std::size_t modified_start;
  std::size_t original_end;
  std::size_t modified_end;
};

/// The furthest reaching endpoints of each search direction, allocated once and
/// reused across every level of the divide and conquer
struct MyersWorkspace {
  std::vector<std::ptrdiff_t> forward;
  std::vector<std::ptrdiff_t> backward;
  std::ptrdiff_t offset{0};
};

inline auto myers_middle_snake(const std::vector<std::size_t> &original,
                               const std::size_t original_start,
                               const std::size_t original_end,
                               const std::vector<std::size_t> &modified,
                               const std::size_t modified_start,
                               const std::size_t modified_end,
                               MyersWorkspace &workspace) -> MyersSnake {
  assert(original_start < original_end);
  assert(modified_start < modified_end);

  const auto original_size{
      static_cast<std::ptrdiff_t>(original_end - original_start)};
  const auto modified_size{
      static_cast<std::ptrdiff_t>(modified_end - modified_start)};
  const auto delta{original_size - modified_size};
  const auto is_odd{(delta % 2) != 0};
  const auto offset{workspace.offset};

  workspace.forward[static_cast<std::size_t>(offset + 1)] = 0;
  workspace.backward[static_cast<std::size_t>(offset + 1)] = 0;

  const auto limit{(original_size + modified_size + 1) / 2};
  for (std::ptrdiff_t depth{0}; depth <= limit; ++depth) {
    for (std::ptrdiff_t diagonal{-depth}; diagonal <= depth; diagonal += 2) {
      const auto lower{
          workspace.forward[static_cast<std::size_t>(offset + diagonal - 1)]};
      const auto upper{
          workspace.forward[static_cast<std::size_t>(offset + diagonal + 1)]};
      auto x{(diagonal == -depth || (diagonal != depth && lower < upper))
                 ? upper
                 : lower + 1};
      auto y{x - diagonal};
      const auto snake_x{x};
      const auto snake_y{y};

      while (x < original_size && y < modified_size &&
             original[original_start + static_cast<std::size_t>(x)] ==
                 modified[modified_start + static_cast<std::size_t>(y)]) {
        x += 1;
        y += 1;
      }

      workspace.forward[static_cast<std::size_t>(offset + diagonal)] = x;

      if (is_odd && delta - diagonal >= -(depth - 1) &&
          delta - diagonal <= depth - 1 &&
          x + workspace.backward[static_cast<std::size_t>(offset + delta -
                                                          diagonal)] >=
              original_size) {
        return {.original_start =
                    original_start + static_cast<std::size_t>(snake_x),
                .modified_start =
                    modified_start + static_cast<std::size_t>(snake_y),
                .original_end = original_start + static_cast<std::size_t>(x),
                .modified_end = modified_start + static_cast<std::size_t>(y)};
      }
    }

    for (std::ptrdiff_t diagonal{-depth}; diagonal <= depth; diagonal += 2) {
      const auto lower{
          workspace.backward[static_cast<std::size_t>(offset + diagonal - 1)]};
      const auto upper{
          workspace.backward[static_cast<std::size_t>(offset + diagonal + 1)]};
      auto x{(diagonal == -depth || (diagonal != depth && lower < upper))
                 ? upper
                 : lower + 1};
      auto y{x - diagonal};
      const auto snake_x{x};
      const auto snake_y{y};

      while (x < original_size && y < modified_size &&
             original[original_start +
                      static_cast<std::size_t>(original_size - x - 1)] ==
                 modified[modified_start +
                          static_cast<std::size_t>(modified_size - y - 1)]) {
        x += 1;
        y += 1;
      }

      workspace.backward[static_cast<std::size_t>(offset + diagonal)] = x;

      if (!is_odd && delta - diagonal >= -depth && delta - diagonal <= depth &&
          x + workspace.forward[static_cast<std::size_t>(offset + delta -
                                                         diagonal)] >=
              original_size) {
        return {.original_start = original_start +
                                  static_cast<std::size_t>(original_size - x),
                .modified_start = modified_start +
                                  static_cast<std::size_t>(modified_size - y),
                .original_end = original_start + static_cast<std::size_t>(
                                                     original_size - snake_x),
                .modified_end = modified_start + static_cast<std::size_t>(
                                                     modified_size - snake_y)};
      }
    }
  }

  assert(false);
  return {.original_start = original_start,
          .modified_start = modified_start,
          .original_end = original_end,
          .modified_end = modified_end};
}

inline auto myers_compare(const std::vector<std::size_t> &original,
                          std::size_t original_start, std::size_t original_end,
                          const std::vector<std::size_t> &modified,
                          std::size_t modified_start, std::size_t modified_end,
                          std::vector<bool> &original_changed,
                          std::vector<bool> &modified_changed,
                          MyersWorkspace &workspace) -> void {
  while (original_start < original_end && modified_start < modified_end &&
         original[original_start] == modified[modified_start]) {
    original_start += 1;
    modified_start += 1;
  }

  while (original_start < original_end && modified_start < modified_end &&
         original[original_end - 1] == modified[modified_end - 1]) {
    original_end -= 1;
    modified_end -= 1;
  }

  if (original_start == original_end) {
    for (auto index{modified_start}; index < modified_end; ++index) {
      modified_changed[index] = true;
    }

    return;
  }

  if (modified_start == modified_end) {
    for (auto index{original_start}; index < original_end; ++index) {
      original_changed[index] = true;
    }

    return;
  }

  const auto snake{myers_middle_snake(original, original_start, original_end,
                                      modified, modified_start, modified_end,
                                      workspace)};

  myers_compare(original, original_start, snake.original_start, modified,
                modified_start, snake.modified_start, original_changed,
                modified_changed, workspace);
  myers_compare(original, snake.original_end, original_end, modified,
                snake.modified_end, modified_end, original_changed,
                modified_changed, workspace);
}

} // namespace sourcemeta::core::internal

#endif
