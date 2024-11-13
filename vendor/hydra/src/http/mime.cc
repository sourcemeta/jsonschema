#include <sourcemeta/hydra/http_mime.h>

#include <algorithm> // std::transform
#include <cctype>    // std::tolower
#include <map>       // std::map

namespace sourcemeta::hydra {

// See https://www.iana.org/assignments/media-types/media-types.xhtml
auto mime_type(const std::filesystem::path &file_path) -> std::string {
  // TODO: More exhaustively define all known types
  static const std::map<std::string, std::string> MIME_TYPES{
      {".css", "text/css"},
      {".png", "image/png"},
      {".webp", "image/webp"},
      {".ico", "image/vnd.microsoft.icon"},
      {".svg", "image/svg+xml"},
      {".webmanifest", "application/manifest+json"},
      {".json", "application/json"},
      {".woff", "font/woff"},
      {".woff2", "font/woff2"},
  };

  std::string extension{file_path.extension().string()};
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](const auto character) {
                   return static_cast<std::string::value_type>(
                       std::tolower(character));
                 });

  const auto result{MIME_TYPES.find(extension)};
  return result == MIME_TYPES.cend() ? "application/octet-stream"
                                     : result->second;
}

} // namespace sourcemeta::hydra
