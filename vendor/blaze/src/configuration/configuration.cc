#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>

#include <algorithm>    // std::ranges::any_of
#include <cassert>      // assert
#include <string>       // std::string
#include <system_error> // std::error_code

namespace sourcemeta::blaze {

auto Configuration::find(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  // Note we use non-throwing overloads of filesystem functions to gracefully
  // handle I/O errors on FUSE and other unusual filesystems
  std::filesystem::path canonical;
  try {
    canonical = sourcemeta::core::weakly_canonical(path);
  } catch (const std::filesystem::filesystem_error &) {
    return std::nullopt;
  }

  assert(canonical.is_absolute());
  std::error_code error;
  const auto is_directory = std::filesystem::is_directory(canonical, error);
  auto current = !error && !is_directory ? canonical.parent_path() : canonical;

  while (!current.empty()) {
    auto candidate = current / "jsonschema.json";
    if (std::filesystem::exists(candidate, error) &&
        std::filesystem::is_regular_file(candidate, error)) {
      return candidate;
    }

    auto parent = current.parent_path();
    if (parent == current) {
      break;
    } else {
      current = parent;
    }
  }

  return std::nullopt;
}

auto Configuration::applies_to(const std::filesystem::path &path) const
    -> bool {
  if (this->extension.empty()) {
    return true;
  }

  const std::string filename{path.filename().string()};
  return std::ranges::any_of(this->extension,
                             [&path, &filename](const auto &suffix) {
                               if (suffix.empty()) {
                                 return path.extension().empty();
                               }

                               return filename.ends_with(suffix);
                             });
}

} // namespace sourcemeta::blaze
