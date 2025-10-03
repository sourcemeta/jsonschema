#include <sourcemeta/core/schemaconfig.h>

#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <cassert> // assert

namespace sourcemeta::core {

auto SchemaConfig::from_json(const JSON &value,
                             const std::filesystem::path &base_path)
    -> SchemaConfig {
  assert(base_path.is_absolute());
  SchemaConfig result;

#define SCHEMACONFIG_ENSURE(condition, message, location)                      \
  if (!(condition)) {                                                          \
    throw SchemaConfigParseError((message), Pointer(location));                \
  }

  // Just to keep this decoupled from actual JSON Schema validator, which we
  // don't do in this repository
  SCHEMACONFIG_ENSURE(value.is_object(), "The configuration must be an object",
                      {});
  SCHEMACONFIG_ENSURE(!value.defines("title") || value.at("title").is_string(),
                      "The title property must be a string", {"title"});
  SCHEMACONFIG_ENSURE(
      !value.defines("description") || value.at("description").is_string(),
      "The description property must be a string", {"description"});
  SCHEMACONFIG_ENSURE(!value.defines("email") || value.at("email").is_string(),
                      "The email property must be a string", {"email"});
  SCHEMACONFIG_ENSURE(!value.defines("github") ||
                          value.at("github").is_string(),
                      "The github property must be a string", {"github"});
  SCHEMACONFIG_ENSURE(!value.defines("website") ||
                          value.at("website").is_string(),
                      "The website property must be a string", {"website"});
  SCHEMACONFIG_ENSURE(!value.defines("path") || value.at("path").is_string(),
                      "The path property must be a string", {"path"});
  SCHEMACONFIG_ENSURE(!value.defines("baseUri") ||
                          value.at("baseUri").is_string(),
                      "The baseUri property must be a string", {"baseUri"});
  SCHEMACONFIG_ENSURE(!value.defines("defaultDialect") ||
                          value.at("defaultDialect").is_string(),
                      "The defaultDialect property must be a string",
                      {"defaultDialect"});
  SCHEMACONFIG_ENSURE(!value.defines("resolve") ||
                          value.at("resolve").is_object(),
                      "The resolve property must be an object", {"resolve"});

  result.title =
      sourcemeta::core::from_json<decltype(result.title)::value_type>(
          value.at_or("title", JSON{nullptr}));
  result.description =
      sourcemeta::core::from_json<decltype(result.description)::value_type>(
          value.at_or("description", JSON{nullptr}));
  result.email =
      sourcemeta::core::from_json<decltype(result.email)::value_type>(
          value.at_or("email", JSON{nullptr}));
  result.github =
      sourcemeta::core::from_json<decltype(result.github)::value_type>(
          value.at_or("github", JSON{nullptr}));
  result.website =
      sourcemeta::core::from_json<decltype(result.website)::value_type>(
          value.at_or("website", JSON{nullptr}));

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
      URI base{value.at("baseUri").to_string()};
      base.canonicalize();
      if (!base.is_absolute()) {
        SCHEMACONFIG_ENSURE(
            false, "The baseUri property must be an absolute URI", {"baseUri"});
      }

      result.base = base.recompose();
    } catch (const URIParseError &) {
      SCHEMACONFIG_ENSURE(false,
                          "The baseUri property must represent a valid URI",
                          {"baseUri"});
    }
  } else {
    // Otherwise the base is the directory
    result.base = URI::from_path(result.absolute_path).recompose();
  }

  result.default_dialect =
      sourcemeta::core::from_json<decltype(result.default_dialect)::value_type>(
          value.at_or("defaultDialect", JSON{nullptr}));

  if (value.defines("resolve")) {
    for (const auto &pair : value.at("resolve").as_object()) {
      SCHEMACONFIG_ENSURE(pair.second.is_string(),
                          "The values in the resolve object must be strings",
                          Pointer({"resolve", pair.first}));

      try {
        result.resolve.emplace(pair.first,
                               URI::canonicalize(pair.second.to_string()));
      } catch (const URIParseError &) {
        SCHEMACONFIG_ENSURE(
            false, "The values in the resolve object must represent valid URIs",
            Pointer({"resolve", pair.first}));
      }
    }
  }

  assert(result.extra.is_object());
  for (const auto &subentry : value.as_object()) {
    if (subentry.first.starts_with("x-")) {
      result.extra.assign(subentry.first, subentry.second);
    }
  }

#undef SCHEMACONFIG_ENSURE
  return result;
}

auto SchemaConfig::read_json(const std::filesystem::path &path)
    -> SchemaConfig {
  assert(path.is_absolute());
  const auto value{sourcemeta::core::read_json(path)};
  return from_json(value, path.parent_path());
}

} // namespace sourcemeta::core
