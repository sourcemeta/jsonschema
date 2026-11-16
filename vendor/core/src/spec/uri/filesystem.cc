#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <algorithm>   // std::ranges::equal, std::ranges::replace
#include <cassert>     // assert
#include <cctype>      // std::tolower
#include <filesystem>  // std::filesystem
#include <iterator>    // std::advance, std::next
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

auto is_localhost_host(const std::string_view host) -> bool {
  constexpr std::string_view LOCALHOST{"localhost"};
  return std::ranges::equal(
      host, LOCALHOST, [](const char left, const char right) -> bool {
        return std::tolower(static_cast<unsigned char>(left)) == right;
      });
}

auto append_raw_segment(std::optional<std::string> &path,
                        const std::string_view segment) -> void {
  if (!path.has_value()) {
    path = std::string{segment};
    return;
  }
  auto &current = path.value();
  // Leading separators are stripped before any of this is walked, so the only
  // segment that spells out to a separator is the one written for a position
  // the path holds nothing at, and such a position is the last one there is
  assert(!current.ends_with('/'));
  if (!segment.starts_with('/')) {
    current += '/';
  }

  current.append(segment);
}

} // namespace

namespace sourcemeta::core {

auto URI::to_path() const -> std::filesystem::path {
  std::string path{this->path().value_or("")};

  // For non-file URIs, just return the path as-is
  if (!this->is_file()) {
    return path;
  }

  // RFC 8089: a non-empty, non-localhost host on a file URI denotes a UNC
  // server. The "localhost" host is equivalent to no host
  const auto host_value = this->host();
  const auto is_unc = host_value.has_value() && !host_value->empty() &&
                      !is_localhost_host(host_value.value());
  if (is_unc) {
    if (!path.empty()) {
      // RFC 3986 Section 3.3 requires the path of a URI that carries an
      // authority to be empty or to begin with a slash
      assert(path.front() == '/');
      path.erase(0, 1);
    }
    std::ranges::replace(path, '/', '\\');
    uri_unescape_all_inplace(path);
    std::string unc{"\\\\"};
    unc.append(host_value.value());
    if (!path.empty()) {
      unc.push_back('\\');
      unc.append(path);
    }
    return unc;
  }

  // RFC 8089 Appendix E.2 spells a drive out as a letter and a colon, which
  // the path carries below the root the authority names
  const auto is_windows_absolute =
      path.size() >= 3 && path[0] == '/' && is_alpha(path[1]) && path[2] == ':';
  if (is_windows_absolute) {
    // Remove leading slash
    path.erase(0, 1);
    // Convert to Windows separators
    std::ranges::replace(path, '/', '\\');
  }

  uri_unescape_all_inplace(path);
  return path;
}

auto URI::from_path(const std::filesystem::path &path) -> URI {
  // Not every standard library recognises the leading pair of separators as a
  // UNC root name, and those that do not collapse it while normalizing, so the
  // form is detected on the path as it was given
  const auto is_unc = path.string().starts_with("\\\\");
  auto normalized = path.lexically_normal().string();

  // Convert backslashes to forward slashes
  std::ranges::replace(normalized, '\\', '/');
  const auto is_unix_absolute = normalized.starts_with("/");

  // RFC 8089 Appendix E.2 spells a drive out as a letter and a colon, and only
  // a separator after the colon makes what follows absolute, as a drive on its
  // own names whichever directory is current on it. A UNC path reads as the
  // first form once its separators are turned around above
  const auto is_windows_absolute = normalized.size() >= 3 &&
                                   is_alpha(normalized[0]) &&
                                   normalized[1] == ':' && normalized[2] == '/';

  // Only absolute paths can be converted to file:// URIs
  if (!is_unix_absolute && !is_windows_absolute) {
    throw URIError(
        "It is not valid to construct a file:// URI out of a relative path");
  }

  // Remove leading slashes for processing
  normalized.erase(0, normalized.find_first_not_of('/'));
  const std::filesystem::path final_path{normalized};

  URI result{"file://"};
  auto iterator = final_path.begin();

  // For UNC paths, the first segment is the hostname, which a root made of
  // nothing but separators does not have
  if (is_unc && iterator != final_path.end()) {
    result.host_ = iterator->string();
    std::advance(iterator, 1);
  }

  // A path made of nothing but separators is the root itself, and the strip
  // above leaves no segment for the loop to walk. RFC 8089 spells the root as
  // an empty authority followed by "/", so it is set here rather than lost
  if (normalized.empty()) {
    result.path_ = "/";
  }

  // Process remaining path segments
  for (; iterator != final_path.end(); ++iterator) {
    if (iterator->empty()) {
      append_raw_segment(result.path_, "/");
      // A drive is a root name of its own on Windows, which leaves the
      // separator following it as a segment rather than as the empty one that
      // trails such a path everywhere else. Leading separators are stripped
      // above, so nowhere else does a segment spell out to one
#if defined(_WIN32)
    } else if (*iterator == "/") {
      if (std::next(iterator) == final_path.end()) {
        append_raw_segment(result.path_, "/");
      }
#endif
    } else {
      // Store raw segment - escaping will happen during recompose()
      const auto segment = iterator->string();

      if (result.path_.has_value()) {
        append_raw_segment(result.path_, segment);
      } else {
        // First segment: file:// URIs need leading slash
        result.path_ = "/" + segment;
      }
    }
  }

  return result;
}

} // namespace sourcemeta::core
