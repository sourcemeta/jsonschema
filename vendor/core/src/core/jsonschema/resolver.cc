#include <sourcemeta/core/jsonschema.h>

#include <cassert> // assert
#include <sstream> // std::ostringstream

namespace sourcemeta::core {

SchemaMapResolver::SchemaMapResolver() = default;

SchemaMapResolver::SchemaMapResolver(const SchemaResolver &resolver)
    : default_resolver{resolver} {}

auto SchemaMapResolver::add(
    const JSON &schema, const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id,
    const std::function<void(const JSON::String &)> &callback) -> bool {
  assert(sourcemeta::core::is_schema(schema));

  // Registering the top-level schema is not enough. We need to check
  // and register every embedded schema resource too
  SchemaFrame frame{SchemaFrame::Mode::References};
  frame.analyse(schema, schema_official_walker, *this, default_dialect,
                default_id);

  bool added_any_schema{false};
  for (const auto &[key, entry] : frame.locations()) {
    if (entry.type != SchemaFrame::LocationType::Resource) {
      continue;
    }

    auto subschema{get(schema, entry.pointer)};
    const auto subschema_vocabularies{frame.vocabularies(entry, *this)};

    // Given we might be resolving embedded resources, we fully
    // resolve their dialect and identifiers, otherwise the
    // consumer might have no idea what to do with them
    subschema.assign("$schema", JSON{entry.dialect});
    // TODO: De-duplicate this id-set functionality from bundle.cc too
    if (subschema_vocabularies.contains(
            "http://json-schema.org/draft-04/schema#") ||
        subschema_vocabularies.contains(
            "http://json-schema.org/draft-03/schema#") ||
        subschema_vocabularies.contains(
            "http://json-schema.org/draft-02/schema#") ||
        subschema_vocabularies.contains(
            "http://json-schema.org/draft-01/schema#") ||
        subschema_vocabularies.contains(
            "http://json-schema.org/draft-00/schema#")) {
      subschema.assign("id", JSON{key.second});
    } else {
      subschema.assign("$id", JSON{key.second});
    }

    const auto result{this->schemas.emplace(key.second, subschema)};
    if (!result.second && result.first->second != schema) {
      std::ostringstream error;
      error << "Cannot register the same identifier twice: " << key.second;
      throw SchemaError(error.str());
    }

    if (callback) {
      callback(key.second);
    }

    added_any_schema = true;
  }

  return added_any_schema;
}

auto SchemaMapResolver::operator()(std::string_view identifier) const
    -> std::optional<JSON> {
  const std::string string_identifier{identifier};
  if (this->schemas.contains(string_identifier)) {
    return this->schemas.at(string_identifier);
  }

  if (this->default_resolver) {
    return this->default_resolver(identifier);
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
