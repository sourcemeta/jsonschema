#ifndef SOURCEMETA_CORE_URI_NORMALIZE_H_
#define SOURCEMETA_CORE_URI_NORMALIZE_H_

#include <string> // std::string

namespace sourcemeta::core {

// Normalize a URI path by removing "." and ".." segments
// Updates the path in-place according to RFC 3986 path segment normalization
// Handles:
// - Removal of "." segments
// - Resolution of ".." segments with proper backtracking
// - Preservation of leading ".." for relative paths
// - Preservation of trailing slashes
// - Preservation of empty segments (consecutive slashes)
inline auto normalize_path(std::string &path) -> void {
  if (path.empty() || path == "/") {
    return;
  }

  std::string canonical_path;
  const auto had_leading_slash = path.starts_with("/");
  const auto had_trailing_slash = path.ends_with('/') && path != "/";
  bool last_segment_was_dot_or_dotdot{false};
  canonical_path.reserve(path.size());
  if (had_leading_slash) {
    canonical_path = "/";
  }

  std::string::size_type minimum_position = had_leading_slash ? 1 : 0;
  std::string::size_type read_position = had_leading_slash ? 1 : 0;
  std::string::size_type segment_start = read_position;

  if (!had_leading_slash && read_position < path.size() &&
      path[read_position] == '.') {
    if (read_position + 1 < path.size() && path[read_position + 1] == '/') {
      read_position += 2;
      segment_start = read_position;
    }
  }

  while (read_position <= path.size()) {
    if (read_position == path.size() || path[read_position] == '/') {
      const auto segment_length = read_position - segment_start;
      if (segment_length == 0 && read_position == path.size() &&
          had_trailing_slash) {
        break;
      }

      if (segment_length == 2 && path[segment_start] == '.' &&
          path[segment_start + 1] == '.') {
        last_segment_was_dot_or_dotdot = true;
        if (canonical_path.size() > minimum_position) {
          if (!canonical_path.empty() && canonical_path.back() == '/' &&
              (canonical_path.size() < 2 ||
               canonical_path[canonical_path.size() - 2] != '/')) {
            canonical_path.pop_back();
          }

          while (canonical_path.size() > minimum_position &&
                 canonical_path.back() != '/') {
            canonical_path.pop_back();
          }

          if (!canonical_path.empty() && canonical_path.back() == '/' &&
              canonical_path.size() > minimum_position) {
            canonical_path.pop_back();
          }
        } else {
          if (!had_leading_slash) {
            if (canonical_path.size() > 0) {
              canonical_path += '/';
            }

            canonical_path.append("..");
            minimum_position = canonical_path.size();
          }
        }
      } else if (segment_length == 1 && path[segment_start] == '.') {
        last_segment_was_dot_or_dotdot = true;
      } else if (segment_length == 0) {
        last_segment_was_dot_or_dotdot = false;
        if (canonical_path.size() >= minimum_position) {
          canonical_path += '/';
        }
      } else {
        last_segment_was_dot_or_dotdot = false;
        if (canonical_path.size() > 0 &&
            (canonical_path.size() > minimum_position || !had_leading_slash)) {
          canonical_path += '/';
        }
        canonical_path.append(path, segment_start, segment_length);
      }

      ++read_position;
      segment_start = read_position;
    } else {
      ++read_position;
    }
  }

  if ((had_trailing_slash || last_segment_was_dot_or_dotdot) &&
      !canonical_path.empty() && canonical_path != "/" &&
      !canonical_path.ends_with('/')) {
    canonical_path += '/';
  }

  path = std::move(canonical_path);
}

} // namespace sourcemeta::core

#endif
