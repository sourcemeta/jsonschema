#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::transform
#include <cassert>   // assert
#include <cctype>    // std::tolower
#include <sstream>   // std::ostringstream

namespace sourcemeta::core {

SchemaMapResolver::SchemaMapResolver() {}

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

SchemaFlatFileResolver::SchemaFlatFileResolver() {}

SchemaFlatFileResolver::SchemaFlatFileResolver(const SchemaResolver &resolver)
    : default_resolver{resolver} {}

static auto to_lowercase(const std::string_view input) -> std::string {
  std::string result{input};
  std::transform(result.cbegin(), result.cend(), result.begin(),
                 [](const auto character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return result;
}

auto SchemaFlatFileResolver::add(
    const std::filesystem::path &path,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id, const Reader &reader,
    SchemaVisitorReference &&reference_visitor) -> const std::string & {
  const auto canonical{std::filesystem::weakly_canonical(path)};
  const auto schema{reader(canonical)};
  assert(sourcemeta::core::is_schema(schema));
  const auto identifier{sourcemeta::core::identify(
      schema, *this, SchemaIdentificationStrategy::Loose, default_dialect,
      default_id)};
  if (!identifier.has_value() && !default_id.has_value()) {
    std::ostringstream error;
    error << "Cannot identify schema: " << canonical.string();
    throw SchemaError(error.str());
  }

  // Filesystems behave differently with regards to casing. To unify
  // them, assume they are case-insensitive.
  const auto effective_identifier{to_lowercase(
      default_id.has_value() ? identifier.value_or(default_id.value())
                             : identifier.value())};

  const auto result{this->schemas.emplace(
      effective_identifier,
      Entry{canonical, default_dialect, effective_identifier, reader,
            reference_visitor ? std::move(reference_visitor)
                              : reference_visitor_relativize})};
  if (!result.second && result.first->second.path != canonical) {
    std::ostringstream error;
    error << "Cannot register the same identifier twice: "
          << effective_identifier;
    throw SchemaError(error.str());
  }

  return result.first->first;
}

auto SchemaFlatFileResolver::reidentify(const std::string &schema,
                                        const std::string &new_identifier)
    -> void {
  const auto result{this->schemas.find(to_lowercase(schema))};
  assert(result != this->schemas.cend());
  this->schemas.insert_or_assign(to_lowercase(new_identifier),
                                 std::move(result->second));
  this->schemas.erase(result);
}

auto SchemaFlatFileResolver::operator()(std::string_view identifier) const
    -> std::optional<JSON> {
  const std::string string_identifier{to_lowercase(identifier)};
  const auto result{this->schemas.find(string_identifier)};
  if (result != this->schemas.cend()) {
    auto schema{result->second.reader(result->second.path)};
    assert(sourcemeta::core::is_schema(schema));
    if (schema.is_object() && !schema.defines("$schema") &&
        result->second.default_dialect.has_value()) {
      schema.assign("$schema", JSON{result->second.default_dialect.value()});
    }

    sourcemeta::core::reidentify(schema, result->second.original_identifier,
                                 *this, result->second.default_dialect);
    // Because we allow re-identification, we can get into issues unless we
    // always try to relativize references
    sourcemeta::core::reference_visit(
        schema, schema_official_walker, *this, result->second.reference_visitor,
        result->second.default_dialect, result->second.original_identifier);
    sourcemeta::core::reidentify(schema, result->first, *this,
                                 result->second.default_dialect);

    return schema;
  }

  if (this->default_resolver) {
    return this->default_resolver(identifier);
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
