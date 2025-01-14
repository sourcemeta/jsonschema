#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>

#include <algorithm> // std::transform
#include <cassert>   // assert
#include <cctype>    // std::tolower
#include <sstream>   // std::ostringstream

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
  Frame frame;
  frame.analyse(schema, default_schema_walker, *this, default_dialect,
                default_id);

  for (const auto &[key, entry] : frame.locations()) {
    if (entry.type != Frame::LocationType::Resource) {
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

FlatFileSchemaResolver::FlatFileSchemaResolver() {}

FlatFileSchemaResolver::FlatFileSchemaResolver(const SchemaResolver &resolver)
    : default_resolver{resolver} {}

static auto to_lowercase(const std::string_view input) -> std::string {
  std::string result{input};
  std::transform(result.cbegin(), result.cend(), result.begin(),
                 [](const auto character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return result;
}

auto FlatFileSchemaResolver::add(
    const std::filesystem::path &path,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id) -> const std::string & {
  const auto canonical{std::filesystem::canonical(path)};
  const auto schema{sourcemeta::jsontoolkit::from_file(canonical)};
  assert(sourcemeta::jsontoolkit::is_schema(schema));
  const auto identifier{sourcemeta::jsontoolkit::identify(
      schema, *this, IdentificationStrategy::Loose, default_dialect,
      default_id)};
  if (!identifier.has_value()) {
    std::ostringstream error;
    error << "Cannot identify schema: " << canonical.string();
    throw SchemaError(error.str());
  }

  // Filesystems behave differently with regards to casing. To unify
  // them, assume they are case-insensitive.
  const auto effective_identifier{to_lowercase(identifier.value())};

  const auto result{this->schemas.emplace(
      effective_identifier,
      Entry{canonical, default_dialect, effective_identifier})};
  if (!result.second && result.first->second.path != canonical) {
    std::ostringstream error;
    error << "Cannot register the same identifier twice: "
          << identifier.value();
    throw SchemaError(error.str());
  }

  return result.first->first;
}

auto FlatFileSchemaResolver::reidentify(const std::string &schema,
                                        const std::string &new_identifier)
    -> void {
  const auto result{this->schemas.find(to_lowercase(schema))};
  assert(result != this->schemas.cend());
  this->schemas.insert_or_assign(to_lowercase(new_identifier),
                                 std::move(result->second));
  this->schemas.erase(result);
}

auto FlatFileSchemaResolver::operator()(std::string_view identifier) const
    -> std::optional<JSON> {
  const std::string string_identifier{to_lowercase(identifier)};
  const auto result{this->schemas.find(string_identifier)};
  if (result != this->schemas.cend()) {
    auto schema{sourcemeta::jsontoolkit::from_file(result->second.path)};
    assert(sourcemeta::jsontoolkit::is_schema(schema));
    if (schema.is_object() && !schema.defines("$schema") &&
        result->second.default_dialect.has_value()) {
      schema.assign("$schema", JSON{result->second.default_dialect.value()});
    }

    sourcemeta::jsontoolkit::reidentify(schema,
                                        result->second.original_identifier,
                                        *this, result->second.default_dialect);
    // Because we allow re-identification, we can get into issues unless we
    // always try to relativize references
    sourcemeta::jsontoolkit::relativize(schema, default_schema_walker, *this,
                                        result->second.default_dialect,
                                        result->second.original_identifier);
    sourcemeta::jsontoolkit::reidentify(schema, result->first, *this,
                                        result->second.default_dialect);

    return schema;
  }

  if (this->default_resolver) {
    return this->default_resolver(identifier);
  }

  return std::nullopt;
}

} // namespace sourcemeta::jsontoolkit
