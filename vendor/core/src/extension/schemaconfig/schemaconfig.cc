#include <sourcemeta/core/io.h>
#include <sourcemeta/core/schemaconfig.h>

#include <algorithm> // std::ranges::any_of
#include <cassert>   // assert
#include <string>    // std::string

namespace sourcemeta::core {

auto SchemaConfig::find(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  const auto canonical{sourcemeta::core::weakly_canonical(path)};
  assert(canonical.is_absolute());
  auto current = std::filesystem::is_directory(canonical)
                     ? canonical
                     : canonical.parent_path();

  while (!current.empty()) {
    auto candidate = current / "jsonschema.json";
    if (std::filesystem::exists(candidate) &&
        std::filesystem::is_regular_file(candidate)) {
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

auto SchemaConfig::applies_to(const std::filesystem::path &path) const -> bool {
  const std::string filename{path.filename().string()};
  return std::ranges::any_of(this->extension, [&filename](const auto &suffix) {
    return filename.ends_with(suffix);
  });
}

} // namespace sourcemeta::core
