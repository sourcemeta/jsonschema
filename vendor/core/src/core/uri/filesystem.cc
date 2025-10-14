#include <sourcemeta/core/uri.h>

#include <algorithm>  // std::ranges::replace
#include <filesystem> // std::filesystem
#include <iterator>   // std::advance, std::next
#include <string>     // std::string

namespace sourcemeta::core {

auto URI::to_path() const -> std::filesystem::path {
  auto path = this->path().value_or("");

  // For non-file URIs, just return the path as-is
  if (!this->is_file()) {
    return path;
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

  // Path is already fully decoded, just return it
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
      result.append_path("/");
    } else if (*iterator == "/") {
      if (std::next(iterator) == final_path.end()) {
        result.append_path("/");
      }
    } else {
      // Store raw segment - escaping will happen during recompose()
      const auto segment = iterator->string();

      if (result.path_.has_value()) {
        result.append_path(segment);
      } else {
        // First segment: file:// URIs need leading slash
        result.path_ = "/" + segment;
      }
    }
  }

  return result;
}

} // namespace sourcemeta::core
