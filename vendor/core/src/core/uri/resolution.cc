#include <sourcemeta/core/uri.h>

#include <cassert>  // assert
#include <optional> // std::optional
#include <string>   // std::string

namespace {

// RFC 3986 Section 5.2.4: Remove Dot Segments
// This algorithm removes the special "." and ".." segments from a path
auto remove_dot_segments(const std::string &path) -> std::string {
  std::string input = path;
  std::string output;

  while (!input.empty()) {
    // A: If the input buffer begins with a prefix of "../" or "./"
    if (input.starts_with("../")) {
      input = input.substr(3);
    } else if (input.starts_with("./")) {
      input = input.substr(2);
    }
    // B: If the input buffer begins with a prefix of "/./" or "/."
    else if (input.starts_with("/./")) {
      input = "/" + input.substr(3);
    } else if (input == "/.") {
      input = "/";
    }
    // C: If the input buffer begins with a prefix of "/../" or "/.."
    else if (input.starts_with("/../")) {
      input = "/" + input.substr(4);
      // Remove the last segment from output
      const auto last_slash = output.rfind('/');
      if (last_slash != std::string::npos) {
        output = output.substr(0, last_slash);
      } else {
        output.clear();
      }
    } else if (input == "/..") {
      input = "/";
      // Remove the last segment from output
      const auto last_slash = output.rfind('/');
      if (last_slash != std::string::npos) {
        output = output.substr(0, last_slash);
      } else {
        output.clear();
      }
    }
    // D: If the input buffer consists only of "." or ".."
    else if (input == "." || input == "..") {
      input.clear();
    }
    // E: Move the first path segment to the end of output
    else {
      std::string::size_type next_slash;
      if (input.starts_with('/')) {
        next_slash = input.find('/', 1);
      } else {
        next_slash = input.find('/');
      }

      if (next_slash == std::string::npos) {
        output += input;
        input.clear();
      } else {
        output += input.substr(0, next_slash);
        input = input.substr(next_slash);
      }
    }
  }

  return output;
}

// Merge paths according to RFC 3986 Section 5.2.3
auto merge_paths(const std::string &base_path, const std::string &ref_path,
                 bool base_has_authority) -> std::string {
  // If base has authority and empty path, prepend "/"
  if (base_has_authority && base_path.empty()) {
    return "/" + ref_path;
  }

  // Otherwise, merge by removing everything after the last "/" in base
  const auto last_slash = base_path.rfind('/');
  if (last_slash == std::string::npos) {
    return ref_path;
  }

  return base_path.substr(0, last_slash + 1) + ref_path;
}

} // namespace

namespace sourcemeta::core {

auto URI::resolve_from(const URI &base) -> URI & {
  // RFC 3986 Section 5.2.2: Transform References

  // Check if this is a dot reference ("." or "./") before we modify the path
  const bool was_dot_reference =
      this->path_.has_value() &&
      (this->path_.value() == "." || this->path_.value() == "./");

  // Reference has a scheme - use as-is (already absolute)
  if (this->scheme_.has_value()) {
    if (this->path_.has_value()) {
      this->path_ = remove_dot_segments(this->path_.value());
    }
    return *this;
  }

  // NON-STANDARD EXTENSION: Handle relative-to-relative resolution
  // RFC 3986 requires base to be absolute, but we extend to support
  // specific relative base cases
  if (!base.is_absolute()) {
    if (this->is_fragment_only()) {
      // Fragment-only reference: add fragment to base
      this->path_ = base.path_;
      this->query_ = base.query_;
      return *this;
    }

    // Simple relative-to-relative case: merge paths directly
    // Example: "foo/bar/baz" + "qux" = "foo/bar/qux"
    const auto base_path = base.path_.value_or("");
    if (!base_path.starts_with('/') && this->path_.has_value()) {
      const auto &ref_path = this->path_.value();
      if (!ref_path.starts_with('/') &&
          ref_path.find('/') == std::string::npos) {
        auto merged = merge_paths(base_path, ref_path, false);
        this->path_ = remove_dot_segments(merged);
        return *this;
      }
    }

    // Otherwise, leave unchanged
    return *this;
  }

  // From here on, reference has no scheme and base is absolute

  // Reference has authority - use reference authority and path
  if (this->host_.has_value()) {
    this->scheme_ = base.scheme_;
    if (this->path_.has_value()) {
      this->path_ = remove_dot_segments(this->path_.value());
    }
    return *this;
  }

  // From here on, reference has no authority, so use base authority
  this->scheme_ = base.scheme_;
  this->userinfo_ = base.userinfo_;
  this->host_ = base.host_;
  this->port_ = base.port_;

  // Reference has empty path
  if (!this->path_.has_value() || this->path_.value().empty()) {
    // Special case: "." or "./" resolves to the containing directory
    if (was_dot_reference) {
      const auto base_path = base.path_.value_or("");
      const auto last_slash = base_path.rfind('/');
      if (last_slash != std::string::npos) {
        this->path_ = base_path.substr(0, last_slash + 1);
      } else {
        this->path_ = std::nullopt;
      }
      return *this;
    }

    // Empty path with query or fragment means use base path
    this->path_ = base.path_;
    if (!this->query_.has_value()) {
      this->query_ = base.query_;
    }
    return *this;
  }

  // Reference path starts with "/" - use reference path as-is
  if (this->path_.value().starts_with('/')) {
    this->path_ = remove_dot_segments(this->path_.value());
    return *this;
  }

  // Reference path is relative - merge with base path
  const auto base_path = base.path_.value_or("");
  const bool base_has_authority = base.host_.has_value();
  auto merged_path =
      merge_paths(base_path, this->path_.value(), base_has_authority);
  this->path_ = remove_dot_segments(merged_path);

  return *this;
}

auto URI::relative_to(const URI &base) -> URI & {
  // Only works if both URIs are absolute
  if (!this->is_absolute() || !base.is_absolute()) {
    return *this;
  }

  // Schemes must match
  if (this->scheme_ != base.scheme_) {
    return *this;
  }

  // Hosts must match (but both can be null for URNs)
  if (this->host_ != base.host_) {
    return *this;
  }

  // Special case: both URIs are exactly the same
  if (this->path_ == base.path_ && this->query_ == base.query_ &&
      this->fragment_ == base.fragment_) {
    // Clear all components to make it empty relative URI
    this->scheme_.reset();
    this->userinfo_.reset();
    this->host_.reset();
    this->port_.reset();
    this->path_.reset();
    this->query_.reset();
    this->fragment_.reset();
    return *this;
  }

  // If this URI doesn't have a path, we can't make it relative
  if (!this->path_.has_value()) {
    return *this;
  }

  const auto &this_path = this->path_.value();

  const auto &base_path = base.path_.value_or("");

  // Case 1: Check if this_path starts with base_path followed by "/"
  // This handles: base="/foo" and this="/foo/bar" = "bar"
  // But NOT: base="/spec" and this="/spec/" (different resources)
  const std::string base_with_slash =
      base_path.ends_with('/') ? base_path : base_path + "/";
  if (this_path.starts_with(base_with_slash) &&
      this_path.length() > base_with_slash.length()) {
    auto relative_path = this_path.substr(base_with_slash.length());

    this->scheme_.reset();
    this->userinfo_.reset();
    this->host_.reset();
    this->port_.reset();
    this->path_ = relative_path.empty()
                      ? std::nullopt
                      : std::optional<std::string>{relative_path};

    return *this;
  }

  // Find last slash positions (needed for multiple cases below)
  const auto base_last_slash = base_path.rfind('/');
  const auto this_last_slash = this_path.rfind('/');

  // Case 2: Check if both paths share the same parent directory (siblings)
  // This handles: base="/test/bar.json" and this="/test/foo.json" =
  // "foo.json"
  if (base_last_slash != std::string::npos &&
      this_last_slash != std::string::npos) {
    const auto base_parent = base_path.substr(0, base_last_slash + 1);
    const auto this_parent = this_path.substr(0, this_last_slash + 1);

    if (base_parent == this_parent) {
      auto relative_path = this_path.substr(this_last_slash + 1);

      this->scheme_.reset();
      this->userinfo_.reset();
      this->host_.reset();
      this->port_.reset();
      this->path_ = relative_path.empty()
                        ? std::nullopt
                        : std::optional<std::string>{relative_path};

      return *this;
    }
  }

  // Case 3: Base has no path or empty path
  // Examples: "https://example.com" or "schema:"
  // Strip leading slash and make relative
  if (base_path.empty()) {
    auto relative_path = this_path;

    if (relative_path.starts_with('/')) {
      relative_path = relative_path.substr(1);
    }

    this->scheme_.reset();
    this->userinfo_.reset();
    this->host_.reset();
    this->port_.reset();
    this->path_ = relative_path.empty()
                      ? std::nullopt
                      : std::optional<std::string>{relative_path};

    return *this;
  }

  // Case 4: General case - compute relative path using .. segments
  // This handles cases like: base="/schemas/foo.json" and this="/bundling/bar"
  // Result should be "../bundling/bar"
  // Note: We don't make URIs relative if the target is just a shallow path
  // like "/foo" (only one level deep) as that's not meaningfully navigable
  const auto base_parent = base_last_slash != std::string::npos
                               ? base_path.substr(0, base_last_slash + 1)
                               : base_path;

  std::string relative_path;
  std::string current_base_parent{base_parent};

  while (!current_base_parent.empty() && current_base_parent != "/") {
    if (this_path.starts_with(current_base_parent)) {
      const auto remainder{this_path.substr(current_base_parent.length())};
      if (!remainder.empty()) {
        // Check if the target is just the base path plus a trailing slash
        // e.g., base="/foo/bar" and target="/foo/bar/"
        // These should stay absolute as they represent different resources
        if (current_base_parent == base_parent &&
            this_path == base_path + "/") {
          return *this;
        }

        relative_path += remainder;
      }

      this->scheme_.reset();
      this->userinfo_.reset();
      this->host_.reset();
      this->port_.reset();
      this->path_ = relative_path.empty()
                        ? std::nullopt
                        : std::optional<std::string>{relative_path};

      return *this;
    }

    relative_path += "../";
    const auto parent_slash{
        current_base_parent.rfind('/', current_base_parent.length() - 2)};
    if (parent_slash == std::string::npos) {
      break;
    }
    current_base_parent = current_base_parent.substr(0, parent_slash + 1);
  }

  // If we reached the root, we can make it relative unless the target path
  // is ambiguous (i.e., it's a prefix of the base parent directory)
  // This handles: "/a/b/c.json" vs "/d.json" -> "../../d.json"
  // And: "/foo/bar" vs "/baz/qux" -> "../../baz/qux"
  // But NOT: "/foo/bar" vs "/foo" (ambiguous: is /foo a file or directory?)
  if (current_base_parent == "/" && this_path.starts_with('/')) {
    // Check if target path is a prefix of the original base parent
    // If so, it's ambiguous and we should stay absolute
    const bool is_prefix_of_base_parent =
        base_parent.starts_with(this_path) &&
        base_parent.length() > this_path.length() &&
        (base_parent[this_path.length()] == '/');

    if (!is_prefix_of_base_parent) {
      relative_path += this_path.substr(1);

      this->scheme_.reset();
      this->userinfo_.reset();
      this->host_.reset();
      this->port_.reset();
      this->path_ = relative_path.empty()
                        ? std::nullopt
                        : std::optional<std::string>{relative_path};

      return *this;
    }
  }

  // If we can't make it relative, return unchanged
  return *this;
}

auto URI::rebase(const URI &base, const URI &new_base) -> URI & {
  this->relative_to(base);
  if (!this->is_relative()) {
    return *this;
  }

  // Save the relative path before copying new_base components
  auto saved_path = std::move(this->path_);
  auto saved_fragment = std::move(this->fragment_);
  auto saved_query = std::move(this->query_);

  // Copy all components from new_base except path/query/fragment
  this->scheme_ = new_base.scheme_;
  this->userinfo_ = new_base.userinfo_;
  this->host_ = new_base.host_;
  this->port_ = new_base.port_;

  if (new_base.path_.has_value() && saved_path.has_value()) {
    const auto &base_path = new_base.path_.value();
    const auto &relative_path = saved_path.value();
    const auto base_ends_with_slash = base_path.ends_with('/');
    const auto relative_starts_with_slash = relative_path.starts_with('/');
    if (base_ends_with_slash && relative_starts_with_slash) {
      this->path_ = base_path + relative_path.substr(1);
    } else if (!base_ends_with_slash && !relative_starts_with_slash) {
      this->path_ = base_path + '/' + relative_path;
    } else {
      this->path_ = base_path + relative_path;
    }
  } else if (new_base.path_.has_value()) {
    this->path_ = new_base.path_;
  } else {
    this->path_ = std::move(saved_path);
  }

  // Restore fragment and query from the relative URI
  this->fragment_ = std::move(saved_fragment);
  this->query_ = std::move(saved_query);

  return *this;
}

} // namespace sourcemeta::core
