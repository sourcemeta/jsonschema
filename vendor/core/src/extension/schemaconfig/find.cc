#include <sourcemeta/core/io.h>
#include <sourcemeta/core/schemaconfig.h>

#include <cassert> // assert

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

} // namespace sourcemeta::core
