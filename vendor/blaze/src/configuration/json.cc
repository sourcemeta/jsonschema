#include <sourcemeta/blaze/configuration.h>

#include <sourcemeta/core/json.h>

#include <algorithm> // std::ranges::sort
#include <string>    // std::string
#include <vector>    // std::vector

namespace sourcemeta::blaze {

auto Configuration::to_json() const -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};

  if (this->title.has_value()) {
    result.assign("title", sourcemeta::core::JSON{this->title.value()});
  }

  if (this->description.has_value()) {
    result.assign("description",
                  sourcemeta::core::JSON{this->description.value()});
  }

  if (this->email.has_value()) {
    result.assign("email", sourcemeta::core::JSON{this->email.value()});
  }

  if (this->github.has_value()) {
    result.assign("github", sourcemeta::core::JSON{this->github.value()});
  }

  if (this->website.has_value()) {
    result.assign("website", sourcemeta::core::JSON{this->website.value()});
  }

  if (!this->absolute_path.empty()) {
    result.assign("path",
                  sourcemeta::core::JSON{this->absolute_path.generic_string()});
  }

  if (!this->base.empty()) {
    result.assign("baseUri", sourcemeta::core::JSON{this->base});
  }

  if (this->default_dialect.has_value()) {
    result.assign("defaultDialect",
                  sourcemeta::core::JSON{this->default_dialect.value()});
  }

  static const Configuration defaults;
  if (!this->extension.empty() && this->extension != defaults.extension) {
    auto extension_array{sourcemeta::core::JSON::make_array()};
    // Sort for deterministic output
    std::vector<std::string> sorted_extensions{this->extension.cbegin(),
                                               this->extension.cend()};
    std::ranges::sort(sorted_extensions);
    for (const auto &entry : sorted_extensions) {
      extension_array.push_back(sourcemeta::core::JSON{entry});
    }

    result.assign("extension", std::move(extension_array));
  }

  if (!this->resolve.empty()) {
    auto resolve_object{sourcemeta::core::JSON::make_object()};
    for (const auto &pair : this->resolve) {
      resolve_object.assign(pair.first, sourcemeta::core::JSON{pair.second});
    }

    result.assign("resolve", std::move(resolve_object));
  }

  if (!this->dependencies.empty()) {
    auto dependencies_object{sourcemeta::core::JSON::make_object()};
    for (const auto &pair : this->dependencies) {
      dependencies_object.assign(
          pair.first, sourcemeta::core::JSON{[&]() -> std::string {
            auto relative_path =
                std::filesystem::relative(pair.second, this->absolute_path)
                    .generic_string();
            if (relative_path.starts_with("..")) {
              return relative_path;
            }

            return "./" + relative_path;
          }()});
    }

    result.assign("dependencies", std::move(dependencies_object));
  }

  for (const auto &pair : this->extra.as_object()) {
    result.assign(pair.first, pair.second);
  }

  return result;
}

} // namespace sourcemeta::blaze
