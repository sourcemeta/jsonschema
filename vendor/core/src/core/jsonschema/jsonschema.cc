#include <sourcemeta/core/jsonschema.h>

#include <cassert>       // assert
#include <cstdint>       // std::uint64_t
#include <limits>        // std::numeric_limits
#include <numeric>       // std::accumulate
#include <type_traits>   // std::remove_reference_t
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move

auto sourcemeta::core::is_schema(const sourcemeta::core::JSON &schema) -> bool {
  return schema.is_object() || schema.is_boolean();
}

// TODO: Make this function detect schemas only using identifier/comment
// keywords, etc
auto sourcemeta::core::is_empty_schema(const sourcemeta::core::JSON &schema)
    -> bool {
  return (schema.is_boolean() && schema.to_boolean()) ||
         (schema.is_object() && schema.empty());
}

namespace {

static auto id_keyword(const std::string &base_dialect) -> std::string {
  if (base_dialect == "https://json-schema.org/draft/2020-12/schema" ||
      base_dialect == "https://json-schema.org/draft/2020-12/hyper-schema" ||
      base_dialect == "https://json-schema.org/draft/2019-09/schema" ||
      base_dialect == "https://json-schema.org/draft/2019-09/hyper-schema" ||
      base_dialect == "http://json-schema.org/draft-07/schema#" ||
      base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-06/schema#" ||
      base_dialect == "http://json-schema.org/draft-06/hyper-schema#") {
    return "$id";
  }

  if (base_dialect == "http://json-schema.org/draft-04/schema#" ||
      base_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-03/schema#" ||
      base_dialect == "http://json-schema.org/draft-03/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-02/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-01/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-00/hyper-schema#") {
    return "id";
  }

  throw sourcemeta::core::SchemaBaseDialectError(base_dialect);
}

} // namespace

auto sourcemeta::core::identify(
    const sourcemeta::core::JSON &schema, const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id)
    -> std::optional<std::string> {
  try {
    const auto maybe_base_dialect{
        sourcemeta::core::base_dialect(schema, resolver, default_dialect)};
    if (maybe_base_dialect.has_value()) {
      return identify(schema, maybe_base_dialect.value(), default_id);
    } else {
      return default_id;
    }
  } catch (const SchemaResolutionError &) {
    if (default_id.has_value()) {
      return default_id;
    } else {
      throw;
    }
  }
}

auto sourcemeta::core::identify(const JSON &schema,
                                const std::string &base_dialect,
                                const std::optional<std::string> &default_id)
    -> std::optional<std::string> {
  if (!schema.is_object()) {
    return default_id;
  }

  const auto keyword{id_keyword(base_dialect)};

  if (!schema.defines(keyword)) {
    return default_id;
  }

  const auto &identifier{schema.at(keyword)};
  if (!identifier.is_string() || identifier.empty()) {
    throw sourcemeta::core::SchemaError(
        "The schema identifier property is invalid");
  }

  // In older drafts, the presence of `$ref` would override any sibling
  // keywords. Note that `$ref` was first introduced in Draft 3, so we
  // don't check for base dialects lower than that.
  // See
  // https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
  if (schema.defines("$ref") &&
      (base_dialect == "http://json-schema.org/draft-07/schema#" ||
       base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
       base_dialect == "http://json-schema.org/draft-06/schema#" ||
       base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
       base_dialect == "http://json-schema.org/draft-04/schema#" ||
       base_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
       base_dialect == "http://json-schema.org/draft-03/schema#" ||
       base_dialect == "http://json-schema.org/draft-03/hyper-schema#")) {
    return default_id;
  }

  return identifier.to_string();
}

auto sourcemeta::core::anonymize(JSON &schema, const std::string &base_dialect)
    -> void {
  if (schema.is_object()) {
    schema.erase(id_keyword(base_dialect));
  }
}

auto sourcemeta::core::reidentify(
    JSON &schema, const std::string &new_identifier,
    const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) -> void {
  const auto base_dialect{
      sourcemeta::core::base_dialect(schema, resolver, default_dialect)};
  if (!base_dialect.has_value()) {
    throw sourcemeta::core::SchemaUnknownBaseDialectError();
  }

  reidentify(schema, new_identifier, base_dialect.value());
}

auto sourcemeta::core::reidentify(JSON &schema,
                                  const std::string &new_identifier,
                                  const std::string &base_dialect) -> void {
  assert(is_schema(schema));
  assert(schema.is_object());
  schema.assign(id_keyword(base_dialect), JSON{new_identifier});

  // If we reidentify, and the identifier is still not retrievable, then
  // we are facing the Draft 7 `$ref` sibling edge case, and we cannot
  // really continue
  if (schema.defines("$ref") && !identify(schema, base_dialect).has_value()) {
    throw SchemaReferenceObjectResourceError(new_identifier);
  }
}

auto sourcemeta::core::dialect(
    const sourcemeta::core::JSON &schema,
    const std::optional<std::string> &default_dialect)
    -> std::optional<std::string> {
  assert(sourcemeta::core::is_schema(schema));
  if (schema.is_boolean() || !schema.defines("$schema")) {
    return default_dialect;
  }

  const sourcemeta::core::JSON &dialect{schema.at("$schema")};
  assert(dialect.is_string() && !dialect.empty());
  return dialect.to_string();
}

auto sourcemeta::core::metaschema(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) -> JSON {
  const auto maybe_dialect{sourcemeta::core::dialect(schema, default_dialect)};
  if (!maybe_dialect.has_value()) {
    throw sourcemeta::core::SchemaUnknownDialectError();
  }

  const auto maybe_metaschema{resolver(maybe_dialect.value())};
  if (!maybe_metaschema.has_value()) {
    // Relative meta-schema references are invalid according to the
    // JSON Schema specifications. They must be absolute ones
    const URI effective_dialect_uri{maybe_dialect.value()};
    if (effective_dialect_uri.is_relative()) {
      throw sourcemeta::core::SchemaRelativeMetaschemaResolutionError(
          maybe_dialect.value());
    } else {
      throw sourcemeta::core::SchemaResolutionError(
          maybe_dialect.value(),
          "Could not resolve the metaschema of the schema");
    }
  }

  return maybe_metaschema.value();
}

auto sourcemeta::core::base_dialect(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect)
    -> std::optional<std::string> {
  assert(sourcemeta::core::is_schema(schema));
  const std::optional<std::string> dialect{
      sourcemeta::core::dialect(schema, default_dialect)};

  // There is no metaschema information whatsoever
  // Nothing we can do at this point
  if (!dialect.has_value()) {
    return std::nullopt;
  }

  const std::string &effective_dialect{dialect.value()};

  // As a performance optimization shortcut
  if (effective_dialect == "https://json-schema.org/draft/2020-12/schema" ||
      effective_dialect == "https://json-schema.org/draft/2019-09/schema" ||
      effective_dialect == "http://json-schema.org/draft-07/schema#" ||
      effective_dialect == "http://json-schema.org/draft-06/schema#") {
    return effective_dialect;
  }

  // For compatibility with older JSON Schema drafts that didn't support $id nor
  // $vocabulary
  if (
      // In Draft 0, 1, and 2, the official metaschema is defined on top of
      // the official hyper-schema metaschema. See
      // http://json-schema.org/draft-00/schema#
      effective_dialect == "http://json-schema.org/draft-00/hyper-schema#" ||
      effective_dialect == "http://json-schema.org/draft-01/hyper-schema#" ||
      effective_dialect == "http://json-schema.org/draft-02/hyper-schema#" ||

      // Draft 3 and 4 have both schema and hyper-schema dialects
      effective_dialect == "http://json-schema.org/draft-03/hyper-schema#" ||
      effective_dialect == "http://json-schema.org/draft-03/schema#" ||
      effective_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
      effective_dialect == "http://json-schema.org/draft-04/schema#") {
    return effective_dialect;
  }

  // If we reach the bottom of the metaschema hierarchy, where the schema
  // defines itself, then we got to the base dialect
  if (schema.is_object() && schema.defines("$id")) {
    assert(schema.at("$id").is_string());
    if (schema.at("$id").to_string() == effective_dialect) {
      return schema.at("$id").to_string();
    }
  }

  // Otherwise, traverse the metaschema hierarchy up
  const std::optional<sourcemeta::core::JSON> metaschema{
      resolver(effective_dialect)};
  if (!metaschema.has_value()) {
    // Relative meta-schema references are invalid according to the
    // JSON Schema specifications. They must be absolute ones
    const URI effective_dialect_uri{effective_dialect};
    if (effective_dialect_uri.is_relative()) {
      throw sourcemeta::core::SchemaRelativeMetaschemaResolutionError(
          effective_dialect);
    } else {
      throw sourcemeta::core::SchemaResolutionError(
          effective_dialect, "Could not resolve the metaschema of the schema");
    }
  }

  return base_dialect(metaschema.value(), resolver, effective_dialect);
}

namespace {
auto core_vocabulary_known(std::string_view base_dialect)
    -> sourcemeta::core::Vocabularies::Known {
  if (base_dialect == "https://json-schema.org/draft/2020-12/schema" ||
      base_dialect == "https://json-schema.org/draft/2020-12/hyper-schema") {
    return sourcemeta::core::Vocabularies::Known::JSON_Schema_2020_12_Core;
  } else if (base_dialect == "https://json-schema.org/draft/2019-09/schema" ||
             base_dialect ==
                 "https://json-schema.org/draft/2019-09/hyper-schema") {
    return sourcemeta::core::Vocabularies::Known::JSON_Schema_2019_09_Core;
  } else {
    throw sourcemeta::core::SchemaBaseDialectError(std::string{base_dialect});
  }
}

auto dialect_to_known(std::string_view dialect)
    -> std::optional<sourcemeta::core::Vocabularies::Known> {
  using sourcemeta::core::Vocabularies;
  if (dialect == "http://json-schema.org/draft-07/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_7;
  }
  if (dialect == "http://json-schema.org/draft-07/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_7_Hyper;
  }
  if (dialect == "http://json-schema.org/draft-06/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_6;
  }
  if (dialect == "http://json-schema.org/draft-06/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_6_Hyper;
  }
  if (dialect == "http://json-schema.org/draft-04/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_4;
  }
  if (dialect == "http://json-schema.org/draft-04/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_4_Hyper;
  }
  if (dialect == "http://json-schema.org/draft-03/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_3;
  }
  if (dialect == "http://json-schema.org/draft-03/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_3_Hyper;
  }
  if (dialect == "http://json-schema.org/draft-02/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_2;
  }
  if (dialect == "http://json-schema.org/draft-02/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_2_Hyper;
  }
  if (dialect == "http://json-schema.org/draft-01/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_1;
  }
  if (dialect == "http://json-schema.org/draft-01/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_1_Hyper;
  }
  if (dialect == "http://json-schema.org/draft-00/schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_0;
  }
  if (dialect == "http://json-schema.org/draft-00/hyper-schema#") {
    return Vocabularies::Known::JSON_Schema_Draft_0_Hyper;
  }
  return std::nullopt;
}
} // namespace

auto sourcemeta::core::vocabularies(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::Vocabularies {
  const std::optional<std::string> maybe_base_dialect{
      sourcemeta::core::base_dialect(schema, resolver, default_dialect)};
  if (!maybe_base_dialect.has_value()) {
    throw sourcemeta::core::SchemaUnknownBaseDialectError();
  }

  const std::optional<std::string> maybe_dialect{
      sourcemeta::core::dialect(schema, default_dialect)};
  if (!maybe_dialect.has_value()) {
    // If the schema has no declared metaschema and the user didn't
    // provide a explicit default, then we cannot do anything.
    // Better to abort instead of trying to guess.
    throw sourcemeta::core::SchemaUnknownDialectError();
  }

  return vocabularies(resolver, maybe_base_dialect.value(),
                      maybe_dialect.value());
}

auto sourcemeta::core::vocabularies(const SchemaResolver &resolver,
                                    const std::string &base_dialect,
                                    const std::string &dialect)
    -> sourcemeta::core::Vocabularies {
  // As a performance optimization shortcut
  if (base_dialect == dialect) {
    if (dialect == "https://json-schema.org/draft/2020-12/schema") {
      return Vocabularies{
          {Vocabularies::Known::JSON_Schema_2020_12_Core, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Applicator, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Validation, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Format_Annotation, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Content, true}};
    } else if (dialect == "https://json-schema.org/draft/2019-09/schema") {
      return Vocabularies{
          {Vocabularies::Known::JSON_Schema_2019_09_Core, true},
          {Vocabularies::Known::JSON_Schema_2019_09_Applicator, true},
          {Vocabularies::Known::JSON_Schema_2019_09_Validation, true},
          {Vocabularies::Known::JSON_Schema_2019_09_Meta_Data, true},
          {Vocabularies::Known::JSON_Schema_2019_09_Format, false},
          {Vocabularies::Known::JSON_Schema_2019_09_Content, true}};
    }
  }

  /*
   * (1) If the dialect is pre-vocabularies, then the
   * dialect itself is conceptually the only vocabulary
   */

  // This is an exhaustive list of all official dialects in the pre-vocabulary
  // world
  if (dialect == "http://json-schema.org/draft-07/schema#" ||
      dialect == "http://json-schema.org/draft-06/schema#" ||
      dialect == "http://json-schema.org/draft-04/schema#" ||
      dialect == "http://json-schema.org/draft-03/schema#" ||
      dialect == "http://json-schema.org/draft-02/schema#" ||
      dialect == "http://json-schema.org/draft-01/schema#" ||
      dialect == "http://json-schema.org/draft-00/schema#") {
    const auto known = dialect_to_known(dialect);
    if (known.has_value()) {
      return Vocabularies{{known.value(), true}};
    }
    return Vocabularies{{dialect, true}};
  }

  /*
   * (2) If the base dialect is pre-vocabularies, then the
   * base dialect itself is conceptually the only vocabulary
   */

  // This is an exhaustive list of all base dialects in the pre-vocabulary world
  if (base_dialect == "http://json-schema.org/draft-07/schema#" ||
      base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-06/schema#" ||
      base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-04/schema#" ||
      base_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-03/schema#" ||
      base_dialect == "http://json-schema.org/draft-03/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-02/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-01/hyper-schema#" ||
      base_dialect == "http://json-schema.org/draft-00/hyper-schema#") {
    const auto known = dialect_to_known(base_dialect);
    if (known.has_value()) {
      return Vocabularies{{known.value(), true}};
    }
    return Vocabularies{{base_dialect, true}};
  }

  /*
   * (3) If the dialect is vocabulary aware, then fetch such dialect
   */

  const std::optional<sourcemeta::core::JSON> maybe_schema_dialect{
      resolver(dialect)};
  if (!maybe_schema_dialect.has_value()) {
    throw sourcemeta::core::SchemaResolutionError(
        dialect, "Could not resolve the metaschema of the schema");
  }
  const sourcemeta::core::JSON &schema_dialect{maybe_schema_dialect.value()};
  // At this point we are sure that the dialect is vocabulary aware and the
  // identifier keyword is indeed `$id`, so we can avoid the added
  // complexity of the generic `id` function.
  assert(schema_dialect.defines("$id") &&
         schema_dialect.at("$id").is_string() &&
         URI::canonicalize(schema_dialect.at("$id").to_string()) ==
             URI::canonicalize(dialect));

  /*
   * (4) Retrieve the vocabularies explicitly or implicitly declared by the
   * dialect
   */

  Vocabularies result;
  const auto core{core_vocabulary_known(base_dialect)};
  if (schema_dialect.defines("$vocabulary")) {
    const sourcemeta::core::JSON &vocabularies{
        schema_dialect.at("$vocabulary")};
    assert(vocabularies.is_object());
    for (const auto &entry : vocabularies.as_object()) {
      result.insert(entry.first, entry.second.to_boolean());
    }
  } else {
    result.insert(core, true);
  }

  // The specification recommends these checks
  if (!result.contains(core)) {
    throw sourcemeta::core::SchemaError(
        "The core vocabulary must always be present");
  } else {
    const auto core_status{result.get(core)};
    if (core_status.has_value() && !core_status.value()) {
      throw sourcemeta::core::SchemaError(
          "The core vocabulary must always be required");
    }
  }

  return result;
}

auto sourcemeta::core::schema_keyword_priority(
    std::string_view keyword,
    const sourcemeta::core::Vocabularies &vocabularies,
    const sourcemeta::core::SchemaWalker &walker) -> std::uint64_t {
  const auto &result{walker(keyword, vocabularies)};
  const auto priority_from_dependencies{std::accumulate(
      result.dependencies.cbegin(), result.dependencies.cend(),
      static_cast<std::uint64_t>(0),
      [&vocabularies, &walker](const auto accumulator, const auto &dependency) {
        return std::max(
            accumulator,
            schema_keyword_priority(dependency, vocabularies, walker) + 1);
      })};
  const auto priority_from_order_dependencies{std::accumulate(
      result.order_dependencies.cbegin(), result.order_dependencies.cend(),
      static_cast<std::uint64_t>(0),
      [&vocabularies, &walker](const auto accumulator, const auto &dependency) {
        return std::max(
            accumulator,
            schema_keyword_priority(dependency, vocabularies, walker) + 1);
      })};
  return std::max(priority_from_dependencies, priority_from_order_dependencies);
}

auto sourcemeta::core::wrap(const sourcemeta::core::JSON::String &identifier)
    -> sourcemeta::core::JSON {
  auto result{JSON::make_object()};
  // JSON Schema 2020-12 is the first dialect that truly supports cross-dialect
  // references In practice, others do, but we can play it safe here
  result.assign_assume_new(
      "$schema", JSON{"https://json-schema.org/draft/2020-12/schema"});
  result.assign_assume_new("$ref", JSON{identifier});
  return result;
}

auto sourcemeta::core::wrap(const sourcemeta::core::JSON &schema,
                            const sourcemeta::core::Pointer &pointer,
                            const sourcemeta::core::SchemaResolver &resolver,
                            const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::JSON {
  assert(try_get(schema, pointer));
  if (pointer.empty()) {
    return schema;
  }

  auto copy = schema;
  const auto effective_dialect{dialect(copy, default_dialect)};
  if (effective_dialect.has_value()) {
    copy.assign("$schema", JSON{effective_dialect.value()});
  } else {
    throw SchemaUnknownBaseDialectError();
  }

  auto result{JSON::make_object()};
  // JSON Schema 2020-12 is the first dialect that truly supports
  // cross-dialect references In practice, others do, but we can
  // play it safe here
  result.assign_assume_new(
      "$schema", JSON{"https://json-schema.org/draft/2020-12/schema"});
  // We need to make sure the schema we are wrapping always has an identifier,
  // at least an artificial one, otherwise a standalone instance of `$schema`
  // outside of the root of a schema resource is not valid according to
  // JSON Schema
  // However, note that we use a relative URI so that references to
  // other schemas whose top-level identifiers are relative URIs don't
  // get affected. Otherwise, we would cause unintended base resolution.
  constexpr auto WRAPPER_IDENTIFIER{"__sourcemeta-core-wrap__"};
  const auto id{
      identify(copy, resolver, default_dialect).value_or(WRAPPER_IDENTIFIER)};

  try {
    reidentify(copy, id, resolver, default_dialect);

    // Otherwise we will get an error with the `WRAPPER_IDENTIFIER`, which will
    // be confusing to end users
  } catch (const SchemaReferenceObjectResourceError &) {
    throw SchemaError(
        "Cannot process a JSON Schema Draft 7 or older with a top-level "
        "`$ref` (which overrides sibling keywords) without introducing "
        "undefined behavior");
  }

  result.assign_assume_new("$defs", JSON::make_object());
  result.at("$defs").assign_assume_new("schema", std::move(copy));

  // Add a reference to the schema
  URI uri{id};
  if (!uri.fragment().has_value() || uri.fragment().value().empty()) {
    uri.fragment(to_string(pointer));
    result.assign_assume_new("$ref", JSON{uri.recompose()});
  } else {
    result.assign_assume_new(
        "$ref",
        JSON{to_uri(Pointer{"$defs", "schema"}.concat(pointer)).recompose()});
  }

  return result;
}

static auto parse_schema_type_string(const sourcemeta::core::JSON::String &type,
                                     sourcemeta::core::JSON::TypeSet &result)
    -> void {
  if (type == "null") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Null));
  } else if (type == "boolean") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Boolean));
  } else if (type == "object") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Object));
  } else if (type == "array") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Array));
  } else if (type == "number") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Integer));
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Real));
  } else if (type == "integer") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::Integer));
  } else if (type == "string") {
    result.set(static_cast<std::size_t>(sourcemeta::core::JSON::Type::String));
  }
}

auto sourcemeta::core::parse_schema_type(const sourcemeta::core::JSON &type)
    -> sourcemeta::core::JSON::TypeSet {
  sourcemeta::core::JSON::TypeSet result;
  if (type.is_string()) {
    parse_schema_type_string(type.to_string(), result);
  } else if (type.is_array()) {
    for (const auto &item : type.as_array()) {
      if (item.is_string()) {
        parse_schema_type_string(item.to_string(), result);
      }
    }
  }

  return result;
}
