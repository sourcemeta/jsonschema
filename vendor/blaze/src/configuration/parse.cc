#include <sourcemeta/blaze/configuration.h>

#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <cassert>       // assert
#include <unordered_set> // std::unordered_set

namespace sourcemeta::blaze {

auto Configuration::from_json(const sourcemeta::core::JSON &value,
                              const std::filesystem::path &base_path)
    -> Configuration {
  assert(base_path.is_absolute());
  Configuration result;

#define CONFIGURATION_ENSURE(condition, message, location)                     \
  if (!(condition)) {                                                          \
    throw ConfigurationParseError((message),                                   \
                                  sourcemeta::core::Pointer(location));        \
  }

  // Just to keep this decoupled from actual JSON Schema validator, which we
  // don't do in this repository
  CONFIGURATION_ENSURE(value.is_object(), "The configuration must be an object",
                       {});
  CONFIGURATION_ENSURE(!value.defines("title") || value.at("title").is_string(),
                       "The title property must be a string", {"title"});
  CONFIGURATION_ENSURE(
      !value.defines("description") || value.at("description").is_string(),
      "The description property must be a string", {"description"});
  CONFIGURATION_ENSURE(!value.defines("email") || value.at("email").is_string(),
                       "The email property must be a string", {"email"});
  CONFIGURATION_ENSURE(!value.defines("github") ||
                           value.at("github").is_string(),
                       "The github property must be a string", {"github"});
  CONFIGURATION_ENSURE(!value.defines("website") ||
                           value.at("website").is_string(),
                       "The website property must be a string", {"website"});
  CONFIGURATION_ENSURE(!value.defines("path") || value.at("path").is_string(),
                       "The path property must be a string", {"path"});
  CONFIGURATION_ENSURE(!value.defines("baseUri") ||
                           value.at("baseUri").is_string(),
                       "The baseUri property must be a string", {"baseUri"});
  CONFIGURATION_ENSURE(!value.defines("defaultDialect") ||
                           value.at("defaultDialect").is_string(),
                       "The defaultDialect property must be a string",
                       {"defaultDialect"});
  CONFIGURATION_ENSURE(
      !value.defines("extension") || value.at("extension").is_array() ||
          value.at("extension").is_string(),
      "The extension property must be a string or an array", {"extension"});
  CONFIGURATION_ENSURE(!value.defines("resolve") ||
                           value.at("resolve").is_object(),
                       "The resolve property must be an object", {"resolve"});
  CONFIGURATION_ENSURE(
      !value.defines("dependencies") || value.at("dependencies").is_object(),
      "The dependencies property must be an object", {"dependencies"});

  result.title =
      sourcemeta::core::from_json<decltype(result.title)::value_type>(
          value.at_or("title", sourcemeta::core::JSON{nullptr}));
  result.description =
      sourcemeta::core::from_json<decltype(result.description)::value_type>(
          value.at_or("description", sourcemeta::core::JSON{nullptr}));
  result.email =
      sourcemeta::core::from_json<decltype(result.email)::value_type>(
          value.at_or("email", sourcemeta::core::JSON{nullptr}));
  result.github =
      sourcemeta::core::from_json<decltype(result.github)::value_type>(
          value.at_or("github", sourcemeta::core::JSON{nullptr}));
  result.website =
      sourcemeta::core::from_json<decltype(result.website)::value_type>(
          value.at_or("website", sourcemeta::core::JSON{nullptr}));

  if (value.defines("path")) {
    const std::filesystem::path path{value.at("path").to_string()};
    if (path.is_absolute()) {
      result.absolute_path = std::filesystem::weakly_canonical(path);
    } else {
      result.absolute_path =
          std::filesystem::weakly_canonical(base_path / path);
    }
  } else {
    result.absolute_path = std::filesystem::weakly_canonical(base_path);
  }

  assert(result.absolute_path.is_absolute());

  if (value.defines("baseUri")) {
    try {
      sourcemeta::core::URI base{value.at("baseUri").to_string()};
      base.canonicalize();
      if (!base.is_absolute()) {
        CONFIGURATION_ENSURE(
            false, "The baseUri property must be an absolute URI", {"baseUri"});
      }

      result.base = base.recompose();
    } catch (const sourcemeta::core::URIParseError &) {
      CONFIGURATION_ENSURE(false,
                           "The baseUri property must represent a valid URI",
                           {"baseUri"});
    }
  } else {
    // Otherwise the base is the directory
    result.base =
        sourcemeta::core::URI::from_path(result.absolute_path).recompose();
  }

  result.default_dialect =
      sourcemeta::core::from_json<decltype(result.default_dialect)::value_type>(
          value.at_or("defaultDialect", sourcemeta::core::JSON{nullptr}));

  if (value.defines("extension")) {
    const auto &extension_value{value.at("extension")};
    if (extension_value.is_string()) {
      auto extension_string{extension_value.to_string()};
      if (!extension_string.empty() && extension_string.front() != '.') {
        extension_string.insert(extension_string.begin(), '.');
      }

      result.extension.emplace(std::move(extension_string));
    } else {
      std::size_t index{0};
      for (const auto &element : extension_value.as_array()) {
        CONFIGURATION_ENSURE(
            element.is_string(),
            "The values in the extension array must be strings",
            sourcemeta::core::Pointer({"extension", index}));

        auto extension_string{element.to_string()};
        if (!extension_string.empty() && extension_string.front() != '.') {
          extension_string.insert(extension_string.begin(), '.');
        }

        result.extension.emplace(std::move(extension_string));
        index += 1;
      }
    }
  } else {
    result.extension = {".json", ".yml", ".yaml"};
  }

  if (value.defines("resolve")) {
    for (const auto &pair : value.at("resolve").as_object()) {
      CONFIGURATION_ENSURE(pair.second.is_string(),
                           "The values in the resolve object must be strings",
                           sourcemeta::core::Pointer({"resolve", pair.first}));

      try {
        result.resolve.emplace(pair.first, sourcemeta::core::URI::canonicalize(
                                               pair.second.to_string()));
      } catch (const sourcemeta::core::URIParseError &) {
        CONFIGURATION_ENSURE(
            false, "The values in the resolve object must represent valid URIs",
            sourcemeta::core::Pointer({"resolve", pair.first}));
      }
    }
  }

  if (value.defines("dependencies")) {
    std::unordered_set<std::filesystem::path> seen_paths;
    for (const auto &pair : value.at("dependencies").as_object()) {
      CONFIGURATION_ENSURE(
          pair.second.is_string(),
          "The values in the dependencies object must be strings",
          sourcemeta::core::Pointer({"dependencies", pair.first}));

      const std::filesystem::path dependency_path{pair.second.to_string()};
      const auto absolute_dependency_path =
          dependency_path.is_absolute()
              ? std::filesystem::weakly_canonical(dependency_path)
              : std::filesystem::weakly_canonical(base_path / dependency_path);
      assert(absolute_dependency_path.is_absolute());
      CONFIGURATION_ENSURE(
          !seen_paths.contains(absolute_dependency_path),
          "Multiple dependencies cannot point to the same path",
          sourcemeta::core::Pointer({"dependencies", pair.first}));
      seen_paths.insert(absolute_dependency_path);
      result.dependencies.emplace(pair.first, absolute_dependency_path);
    }
  }

  assert(result.extra.is_object());
  for (const auto &subentry : value.as_object()) {
    if (subentry.first.starts_with("x-")) {
      result.extra.assign(subentry.first, subentry.second);
    }
  }

#undef CONFIGURATION_ENSURE
  return result;
}

auto Configuration::read_json(const std::filesystem::path &path,
                              const Configuration::ReadCallback &reader)
    -> Configuration {
  assert(path.is_absolute());
  const auto value{sourcemeta::core::parse_json(reader(path))};
  return from_json(value, path.parent_path());
}

} // namespace sourcemeta::blaze
