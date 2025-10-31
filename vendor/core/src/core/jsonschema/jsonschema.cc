#include <sourcemeta/core/jsonschema.h>

#include <cassert>       // assert
#include <cstdint>       // std::uint64_t
#include <limits>        // std::numeric_limits
#include <numeric>       // std::accumulate
#include <sstream>       // std::ostringstream
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

auto id_keyword_guess(const sourcemeta::core::JSON &schema)
    -> std::optional<std::string> {
  if (schema.defines("$id") && schema.at("$id").is_string()) {
    if (!schema.defines("id") ||
        (schema.defines("id") && (!schema.at("id").is_string() ||
                                  schema.at("$id") == schema.at("id")))) {
      return "$id";
    }
  } else if (schema.defines("id") && schema.at("id").is_string()) {
    return "id";
  }

  return std::nullopt;
}

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

  std::ostringstream error;
  error << "Unrecognized base dialect: " << base_dialect;
  throw sourcemeta::core::SchemaError(error.str());
}

} // namespace

auto sourcemeta::core::identify(
    const sourcemeta::core::JSON &schema, const SchemaResolver &resolver,
    const SchemaIdentificationStrategy strategy,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id)
    -> std::optional<std::string> {
  std::optional<std::string> maybe_base_dialect;

  // TODO: Can we avoid a C++ exception as the potential normal way of
  // operation?
  try {
    maybe_base_dialect =
        sourcemeta::core::base_dialect(schema, resolver, default_dialect);
  } catch (const SchemaResolutionError &) {
    // Attempt to play a heuristic guessing game before giving up
    if (strategy == SchemaIdentificationStrategy::Loose && schema.is_object()) {
      const auto keyword{id_keyword_guess(schema)};
      if (keyword.has_value()) {
        return schema.at(keyword.value()).to_string();
      } else {
        return std::nullopt;
      }
    }

    throw;
  }

  if (!maybe_base_dialect.has_value()) {
    // Attempt to play a heuristic guessing game before giving up
    if (strategy == SchemaIdentificationStrategy::Loose && schema.is_object()) {
      const auto keyword{id_keyword_guess(schema)};
      if (keyword.has_value()) {
        return schema.at(keyword.value()).to_string();
      } else {
        return std::nullopt;
      }
    }

    return default_id;
  }

  return identify(schema, maybe_base_dialect.value(), strategy, default_id);
}

auto sourcemeta::core::identify(const JSON &schema,
                                const std::string &base_dialect,
                                const SchemaIdentificationStrategy strategy,
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
    std::ostringstream error;
    error << "The value of the " << keyword << " property is not valid";
    throw sourcemeta::core::SchemaError(error.str());
  }

  // In older drafts, the presence of `$ref` would override any sibling
  // keywords. Note that `$ref` was first introduced in Draft 3, so we
  // don't check for base dialects lower than that.
  // See
  // https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
  if (strategy == SchemaIdentificationStrategy::Strict &&
      schema.defines("$ref") &&
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
auto core_vocabulary(std::string_view base_dialect) -> std::string {
  if (base_dialect == "https://json-schema.org/draft/2020-12/schema" ||
      base_dialect == "https://json-schema.org/draft/2020-12/hyper-schema") {
    return "https://json-schema.org/draft/2020-12/vocab/core";
  } else if (base_dialect == "https://json-schema.org/draft/2019-09/schema" ||
             base_dialect ==
                 "https://json-schema.org/draft/2019-09/hyper-schema") {
    return "https://json-schema.org/draft/2019-09/vocab/core";
  } else {
    std::ostringstream error;
    error << "Unrecognized base dialect: " << base_dialect;
    throw sourcemeta::core::SchemaError(error.str());
  }
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
      return {{"https://json-schema.org/draft/2020-12/vocab/core", true},
              {"https://json-schema.org/draft/2020-12/vocab/applicator", true},
              {"https://json-schema.org/draft/2020-12/vocab/unevaluated", true},
              {"https://json-schema.org/draft/2020-12/vocab/validation", true},
              {"https://json-schema.org/draft/2020-12/vocab/meta-data", true},
              {"https://json-schema.org/draft/2020-12/vocab/format-annotation",
               true},
              {"https://json-schema.org/draft/2020-12/vocab/content", true}};
    } else if (dialect == "https://json-schema.org/draft/2019-09/schema") {
      return {{"https://json-schema.org/draft/2019-09/vocab/core", true},
              {"https://json-schema.org/draft/2019-09/vocab/applicator", true},
              {"https://json-schema.org/draft/2019-09/vocab/validation", true},
              {"https://json-schema.org/draft/2019-09/vocab/meta-data", true},
              {"https://json-schema.org/draft/2019-09/vocab/format", false},
              {"https://json-schema.org/draft/2019-09/vocab/content", true}};
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
    return {{dialect, true}};
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
    return {{base_dialect, true}};
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
  const std::string core{core_vocabulary(base_dialect)};
  if (schema_dialect.defines("$vocabulary")) {
    const sourcemeta::core::JSON &vocabularies{
        schema_dialect.at("$vocabulary")};
    assert(vocabularies.is_object());
    for (const auto &entry : vocabularies.as_object()) {
      result.insert({entry.first, entry.second.to_boolean()});
    }
  } else {
    result.insert({core, true});
  }

  // The specification recommends these checks
  if (!result.contains(core)) {
    throw sourcemeta::core::SchemaError(
        "The core vocabulary must always be present");
  } else if (!result.at(core)) {
    throw sourcemeta::core::SchemaError(
        "The core vocabulary must always be required");
  }

  return result;
}

auto sourcemeta::core::schema_keyword_priority(
    std::string_view keyword,
    const sourcemeta::core::Vocabularies &vocabularies,
    const sourcemeta::core::SchemaWalker &walker) -> std::uint64_t {
  const auto result{walker(keyword, vocabularies)};
  return std::accumulate(
      result.dependencies.cbegin(), result.dependencies.cend(),
      static_cast<std::uint64_t>(0),
      [&vocabularies, &walker](const auto accumulator, const auto &dependency) {
        return std::max(
            accumulator,
            schema_keyword_priority(dependency, vocabularies, walker) + 1);
      });
}

auto sourcemeta::core::wrap(const sourcemeta::core::JSON::String &identifier)
    -> sourcemeta::core::JSON {
  auto result{JSON::make_object()};
  // JSON Schema 2020-12 is the first dialect that truly supports cross-dialect
  // references In practice, others do, but we can play it safe here
  result.assign("$schema",
                JSON{"https://json-schema.org/draft/2020-12/schema"});
  result.assign("$ref", JSON{identifier});
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
  result.assign("$schema",
                // JSON Schema 2020-12 is the first dialect that truly supports
                // cross-dialect references In practice, others do, but we can
                // play it safe here
                JSON{"https://json-schema.org/draft/2020-12/schema"});
  // We need to make sure the schema we are wrapping always has an identifier,
  // at least an artificial one, otherwise a standalone instance of `$schema`
  // outside of the root of a schema resource is not valid according to
  // JSON Schema
  // However, note that we use a relative URI so that references to
  // other schemas whose top-level identifiers are relative URIs don't
  // get affected. Otherwise, we would cause unintended base resolution.
  constexpr auto WRAPPER_IDENTIFIER{"__sourcemeta-core-wrap__"};
  const auto id{identify(copy, resolver, SchemaIdentificationStrategy::Strict,
                         default_dialect)
                    .value_or(WRAPPER_IDENTIFIER)};
  reidentify(copy, id, resolver, default_dialect);
  result.assign("$defs", JSON::make_object());
  result.at("$defs").assign("schema", std::move(copy));

  // Add a reference to the schema
  URI uri{id};
  if (!uri.fragment().has_value() || uri.fragment().value().empty()) {
    uri.fragment(to_string(pointer));
    result.assign("$ref", JSON{uri.recompose()});
  } else {
    result.assign(
        "$ref",
        JSON{to_uri(Pointer{"$defs", "schema"}.concat(pointer)).recompose()});
  }

  return result;
}

auto sourcemeta::core::parse_schema_type(
    const sourcemeta::core::JSON::String &type,
    const std::function<void(const sourcemeta::core::JSON::Type)> &callback)
    -> void {
  if (type == "null") {
    callback(sourcemeta::core::JSON::Type::Null);
  } else if (type == "boolean") {
    callback(sourcemeta::core::JSON::Type::Boolean);
  } else if (type == "object") {
    callback(sourcemeta::core::JSON::Type::Object);
  } else if (type == "array") {
    callback(sourcemeta::core::JSON::Type::Array);
  } else if (type == "number") {
    callback(sourcemeta::core::JSON::Type::Integer);
    callback(sourcemeta::core::JSON::Type::Real);
  } else if (type == "integer") {
    callback(sourcemeta::core::JSON::Type::Integer);
  } else if (type == "string") {
    callback(sourcemeta::core::JSON::Type::String);
  }
}
