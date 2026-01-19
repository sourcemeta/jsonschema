#include <sourcemeta/codegen/ir.h>

#include <sourcemeta/core/uri.h>

#include <algorithm>  // std::ranges::reverse
#include <cassert>    // assert
#include <filesystem> // std::filesystem::path
#include <sstream>    // std::istringstream
#include <string>     // std::string, std::getline
#include <vector>     // std::vector

namespace {

// Strip all extensions from a filename (e.g., "user.schema.json" -> "user")
auto strip_extensions(const std::string &filename) -> std::string {
  std::filesystem::path path{filename};
  while (path.has_extension()) {
    path = path.stem();
  }
  return path.string();
}

// If the input looks like an absolute URI, extract its path segments.
// For file URIs, only the filename (without extensions) is used.
// For other URIs, all path segments are used with extensions stripped from
// the last segment.
// Otherwise, add the input as a single segment.
// Note: segments are added in reverse order because the caller reverses
// the entire result at the end.
auto push_token_segments(std::vector<std::string> &result,
                         const std::string &value) -> void {
  try {
    const sourcemeta::core::URI uri{value};
    if (uri.is_absolute()) {
      const auto path{uri.path()};
      if (path.has_value() && !path->empty()) {
        std::vector<std::string> segments;
        std::istringstream stream{std::string{path.value()}};
        std::string segment;
        while (std::getline(stream, segment, '/')) {
          if (!segment.empty()) {
            segments.emplace_back(segment);
          }
        }

        if (!segments.empty()) {
          // Strip extensions from the last segment
          segments.back() = strip_extensions(segments.back());

          // For file URIs, only use the filename
          if (uri.is_file()) {
            result.emplace_back(segments.back());
          } else {
            // Reverse segments since the caller will reverse the entire result
            std::ranges::reverse(segments);
            for (const auto &path_segment : segments) {
              result.emplace_back(path_segment);
            }
          }

          return;
        }
      }
    }
    // NOLINTNEXTLINE(bugprone-empty-catch)
  } catch (const sourcemeta::core::URIParseError &) {
    // Not a valid URI, fall through to default behavior
  }

  result.emplace_back(value);
}

} // namespace

namespace sourcemeta::codegen {

auto symbol(const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location)
    -> std::vector<std::string> {
  std::vector<std::string> result;

  auto current_pointer{location.pointer};

  while (true) {
    const auto current_location{frame.traverse(current_pointer)};
    assert(current_location.has_value());

    if (!current_location->get().parent.has_value()) {
      break;
    }

    const auto &parent_pointer{current_location->get().parent.value()};
    const auto segments_skipped{current_pointer.size() - parent_pointer.size()};
    assert(segments_skipped >= 1);

    if (segments_skipped >= 2) {
      const auto &last_token{current_pointer.back()};
      if (last_token.is_property()) {
        push_token_segments(result, last_token.to_property());
      } else {
        result.emplace_back(std::to_string(last_token.to_index()));
      }
    } else {
      const auto &token{current_pointer.back()};
      if (token.is_property()) {
        push_token_segments(result, token.to_property());
      } else {
        result.emplace_back(std::to_string(token.to_index()));
      }
    }

    current_pointer = parent_pointer;
  }

  std::ranges::reverse(result);
  return result;
}

} // namespace sourcemeta::codegen
