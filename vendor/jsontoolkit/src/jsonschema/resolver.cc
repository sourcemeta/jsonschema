#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>

#include <cassert> // assert
#include <sstream> // std::ostringstream

namespace sourcemeta::jsontoolkit {

MapSchemaResolver::MapSchemaResolver() {}

MapSchemaResolver::MapSchemaResolver(const SchemaResolver &resolver)
    : default_resolver{resolver} {}

auto MapSchemaResolver::add(const JSON &schema,
                            const std::optional<std::string> &default_dialect,
                            const std::optional<std::string> &default_id)
    -> void {
  assert(sourcemeta::jsontoolkit::is_schema(schema));

  // Registering the top-level schema is not enough. We need to check
  // and register every embedded schema resource too
  ReferenceFrame entries;
  ReferenceMap references;
  frame(schema, entries, references, default_schema_walker, *this,
        default_dialect, default_id);

  for (const auto &[key, entry] : entries) {
    if (entry.type != ReferenceEntryType::Resource) {
      continue;
    }

    auto subschema{get(schema, entry.pointer)};
    // TODO: Set the base dialect in the frame entries
    const auto subschema_base_dialect{
        base_dialect(subschema, *this, entry.dialect)};
    assert(subschema_base_dialect.has_value());
    const auto subschema_vocabularies{
        vocabularies(*this, subschema_base_dialect.value(), entry.dialect)};

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
  }
}

auto MapSchemaResolver::operator()(std::string_view identifier) const
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

} // namespace sourcemeta::jsontoolkit
