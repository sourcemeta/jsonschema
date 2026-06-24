#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <algorithm>   // std::ranges::equal, std::ranges::replace
#include <cctype>      // std::tolower
#include <filesystem>  // std::filesystem
#include <iterator>    // std::advance, std::next
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

auto is_localhost_host(const std::string_view host) -> bool {
  constexpr std::string_view localhost{"localhost"};
  return std::ranges::equal(
      host, localhost, [](const char left, const char right) -> bool {
        return std::tolower(static_cast<unsigned char>(left)) == right;
      });
}

auto append_raw_segment(std::optional<std::string> &path,
                        const std::string_view segment) -> void {
  if (segment.empty()) {
    return;
  }
  if (!path.has_value()) {
    path = std::string{segment};
    return;
  }
  auto &current = path.value();
  const bool current_ends_with_slash = current.ends_with('/');
  const bool segment_starts_with_slash = segment.starts_with('/');
  if (current_ends_with_slash && segment_starts_with_slash) {
    current.append(segment, 1);
  } else if (!current_ends_with_slash && !segment_starts_with_slash) {
    current += '/';
    current.append(segment);
  } else {
    current.append(segment);
  }
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
    if (!path.empty() && path.front() == '/') {
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

  // Check for Windows absolute path (e.g., /C:/)
  const auto is_windows_absolute =
      path.size() >= 3 && path[0] == '/' && path[2] == ':';
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
  auto normalized = path.lexically_normal().string();
  const auto is_unc = normalized.starts_with("\\\\");
  const auto is_windows_absolute =
      normalized.size() >= 2 && normalized[1] == ':';

  // Convert backslashes to forward slashes
  std::ranges::replace(normalized, '\\', '/');
  const auto is_unix_absolute = normalized.starts_with("/");

  // Only absolute paths can be converted to file:// URIs
  if (!is_unix_absolute && !is_windows_absolute && !is_unc) {
    throw URIError(
        "It is not valid to construct a file:// URI out of a relative path");
  }

  // Remove leading slashes for processing
  normalized.erase(0, normalized.find_first_not_of('/'));
  const std::filesystem::path final_path{normalized};

  URI result{"file://"};
  auto iterator = final_path.begin();

  // For UNC paths, the first segment is the hostname
  if (is_unc) {
    result.host_ = iterator->string();
    std::advance(iterator, 1);
  }

  // Process remaining path segments
  for (; iterator != final_path.end(); ++iterator) {
    if (iterator->empty()) {
      append_raw_segment(result.path_, "/");
    } else if (*iterator == "/") {
      if (std::next(iterator) == final_path.end()) {
        append_raw_segment(result.path_, "/");
      }
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
