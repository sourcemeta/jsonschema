#include <sourcemeta/core/uri.h>

#include "normalize.h"

#include <optional> // std::optional
#include <string>   // std::string

namespace {

auto remove_dot_segments(const std::string &path) -> std::string {
  std::string result{path};
  sourcemeta::core::normalize_path(result);
  return result;
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
  // Resolving against an IRI base, or resolving an IRI reference, yields an IRI
  this->iri_ = this->iri_ || base.iri_;

  // RFC 3986 Section 5.2.2: Transform References

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
      if (!ref_path.starts_with('/') && !ref_path.contains('/')) {
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

  // The full authority must match (but components can be null for URNs)
  if (this->userinfo_ != base.userinfo_) {
    return *this;
  }

  if (this->host_ != base.host_) {
    return *this;
  }

  if (this->port_ != base.port_) {
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

  // Both URIs share the same path and differ only in their query or fragment.
  // An empty relative path is safe when this URI carries its own query or the
  // base has none, since resolution then will not re-inherit the base's query.
  // Otherwise a non-empty path is required to reset the query, which is only
  // possible when there is a path to express.
  if (this->path_ == base.path_) {
    const bool empty_path_safe{this->query_.has_value() ||
                               !base.query_.has_value()};
    if (empty_path_safe || this->path_.has_value()) {
      this->scheme_.reset();
      this->userinfo_.reset();
      this->host_.reset();
      this->port_.reset();
      if (empty_path_safe) {
        this->path_.reset();
      } else {
        // Use the last path segment, or "./" when the path has no final segment
        // (it ends in a slash).
        const auto &path{this->path_.value()};
        const auto slash{path.rfind('/')};
        auto segment{slash == std::string::npos ? path
                                                : path.substr(slash + 1)};
        this->path_ = segment.empty() ? std::string{"./"} : std::move(segment);
      }
      return *this;
    }
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

  if (current_base_parent == "/" && this_path.starts_with('/')) {
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

  // If we can't make it relative, return unchanged
  return *this;
}

namespace {

auto merge_new_base_path(std::optional<std::string> &target_path,
                         std::optional<std::string> &&new_base_path,
                         std::optional<std::string> &&saved_path) -> void {
  if (new_base_path.has_value() && saved_path.has_value()) {
    auto merged{std::move(new_base_path.value())};
    const auto &relative_path = saved_path.value();
    const auto base_ends_with_slash = merged.ends_with('/');
    const auto relative_starts_with_slash = relative_path.starts_with('/');
    if (base_ends_with_slash && relative_starts_with_slash) {
      merged.append(relative_path, 1);
    } else if (!base_ends_with_slash && !relative_starts_with_slash) {
      merged += '/';
      merged += relative_path;
    } else {
      merged += relative_path;
    }
    target_path = std::move(merged);
  } else if (new_base_path.has_value()) {
    target_path = std::move(new_base_path);
  } else {
    target_path = std::move(saved_path);
  }
}

} // namespace

auto URI::rebase(const URI &base, const URI &new_base) -> URI & {
  this->relative_to(base);
  if (!this->is_relative()) {
    return *this;
  }

  auto saved_path = std::move(this->path_);
  auto saved_fragment = std::move(this->fragment_);
  auto saved_query = std::move(this->query_);

  this->scheme_ = new_base.scheme_;
  this->userinfo_ = new_base.userinfo_;
  this->host_ = new_base.host_;
  this->port_ = new_base.port_;
  // The new components come from the new base, so the result is an IRI if the
  // new base is one
  this->iri_ = this->iri_ || new_base.iri_;

  std::optional<std::string> new_base_path_copy{new_base.path_};
  merge_new_base_path(this->path_, std::move(new_base_path_copy),
                      std::move(saved_path));

  this->fragment_ = std::move(saved_fragment);
  this->query_ = std::move(saved_query);

  return *this;
}

auto URI::rebase(const URI &base, URI &&new_base) -> URI & {
  this->relative_to(base);
  if (!this->is_relative()) {
    return *this;
  }

  auto saved_path = std::move(this->path_);
  auto saved_fragment = std::move(this->fragment_);
  auto saved_query = std::move(this->query_);

  this->scheme_ = std::move(new_base.scheme_);
  this->userinfo_ = std::move(new_base.userinfo_);
  this->host_ = std::move(new_base.host_);
  this->port_ = new_base.port_;
  // The new components come from the new base, so the result is an IRI if the
  // new base is one
  this->iri_ = this->iri_ || new_base.iri_;

  merge_new_base_path(this->path_, std::move(new_base.path_),
                      std::move(saved_path));

  this->fragment_ = std::move(saved_fragment);
  this->query_ = std::move(saved_query);

  return *this;
}

} // namespace sourcemeta::core
