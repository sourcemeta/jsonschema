#include <sourcemeta/core/schemaconfig.h>

namespace sourcemeta::core {

auto SchemaConfig::find(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  auto current =
      std::filesystem::is_directory(path) ? path : path.parent_path();

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

} // namespace sourcemeta::core
