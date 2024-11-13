#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cassert>     // assert
#include <cstdint>     // std::uint64_t
#include <functional>  // std::less
#include <limits>      // std::numeric_limits
#include <sstream>     // std::ostringstream
#include <type_traits> // std::remove_reference_t
#include <utility>     // std::move

auto sourcemeta::jsontoolkit::is_schema(
    const sourcemeta::jsontoolkit::JSON &schema) -> bool {
  return schema.is_object() || schema.is_boolean();
}

static auto id_keyword_guess(const sourcemeta::jsontoolkit::JSON &schema)
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
  throw sourcemeta::jsontoolkit::SchemaError(error.str());
}

auto sourcemeta::jsontoolkit::identify(
    const sourcemeta::jsontoolkit::JSON &schema, const SchemaResolver &resolver,
    const IdentificationStrategy strategy,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id)
    -> std::optional<std::string> {
  std::optional<std::string> maybe_base_dialect;

  // TODO: Can we avoid a C++ exception as the potential normal way of
  // operation?
  try {
    maybe_base_dialect = sourcemeta::jsontoolkit::base_dialect(schema, resolver,
                                                               default_dialect);
  } catch (const SchemaResolutionError &) {
    // Attempt to play a heuristic guessing game before giving up
    if (strategy == IdentificationStrategy::Loose && schema.is_object()) {
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
    if (strategy == IdentificationStrategy::Loose && schema.is_object()) {
      const auto keyword{id_keyword_guess(schema)};
      if (keyword.has_value()) {
        return schema.at(keyword.value()).to_string();
      } else {
        return std::nullopt;
      }
    }

    return default_id;
  }

  return identify(schema, maybe_base_dialect.value(), default_id);
}

auto sourcemeta::jsontoolkit::identify(
    const JSON &schema, const std::string &base_dialect,
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
    throw sourcemeta::jsontoolkit::SchemaError(error.str());
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
    return std::nullopt;
  }

  return identifier.to_string();
}

auto sourcemeta::jsontoolkit::anonymize(JSON &schema,
                                        const std::string &base_dialect)
    -> void {
  if (schema.is_object()) {
    schema.erase(id_keyword(base_dialect));
  }
}

auto sourcemeta::jsontoolkit::reidentify(
    JSON &schema, const std::string &new_identifier,
    const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) -> void {
  const auto base_dialect{
      sourcemeta::jsontoolkit::base_dialect(schema, resolver, default_dialect)};
  if (!base_dialect.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaError("Cannot determine base dialect");
  }

  reidentify(schema, new_identifier, base_dialect.value());
}

auto sourcemeta::jsontoolkit::reidentify(JSON &schema,
                                         const std::string &new_identifier,
                                         const std::string &base_dialect)
    -> void {
  assert(is_schema(schema));
  assert(schema.is_object());
  schema.assign(id_keyword(base_dialect), JSON{new_identifier});
  assert(identify(schema, base_dialect).has_value());
}

auto sourcemeta::jsontoolkit::dialect(
    const sourcemeta::jsontoolkit::JSON &schema,
    const std::optional<std::string> &default_dialect)
    -> std::optional<std::string> {
  assert(sourcemeta::jsontoolkit::is_schema(schema));
  if (schema.is_boolean() || !schema.defines("$schema")) {
    return default_dialect;
  }

  const sourcemeta::jsontoolkit::JSON &dialect{schema.at("$schema")};
  assert(dialect.is_string() && !dialect.empty());
  return dialect.to_string();
}

auto sourcemeta::jsontoolkit::metaschema(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) -> JSON {
  const auto maybe_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  if (!maybe_dialect.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaError(
        "Could not determine dialect of the schema");
  }

  const auto maybe_metaschema{resolver(maybe_dialect.value())};
  if (!maybe_metaschema.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaResolutionError(
        maybe_dialect.value(),
        "Could not resolve the metaschema of the schema");
  }

  return maybe_metaschema.value();
}

auto sourcemeta::jsontoolkit::base_dialect(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect)
    -> std::optional<std::string> {
  assert(sourcemeta::jsontoolkit::is_schema(schema));
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};

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
  const std::optional<sourcemeta::jsontoolkit::JSON> metaschema{
      resolver(effective_dialect)};
  if (!metaschema.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaResolutionError(
        effective_dialect, "Could not resolve the requested schema");
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
    throw sourcemeta::jsontoolkit::SchemaError(error.str());
  }
}
} // namespace

auto sourcemeta::jsontoolkit::vocabularies(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect)
    -> std::map<std::string, bool> {
  const std::optional<std::string> maybe_base_dialect{
      sourcemeta::jsontoolkit::base_dialect(schema, resolver, default_dialect)};
  if (!maybe_base_dialect.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaError(
        "Could not determine base dialect for schema");
  }

  const std::optional<std::string> maybe_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  if (!maybe_dialect.has_value()) {
    // If the schema has no declared metaschema and the user didn't
    // provide a explicit default, then we cannot do anything.
    // Better to abort instead of trying to guess.
    throw sourcemeta::jsontoolkit::SchemaError(
        "Cannot determine the dialect of the schema");
  }

  return vocabularies(resolver, maybe_base_dialect.value(),
                      maybe_dialect.value());
}

auto sourcemeta::jsontoolkit::vocabularies(const SchemaResolver &resolver,
                                           const std::string &base_dialect,
                                           const std::string &dialect)
    -> std::map<std::string, bool> {
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
   * (1) If the base dialect is pre-vocabularies, then the
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
   * (2) If the dialect is vocabulary aware, then fetch such dialect
   */

  const std::optional<sourcemeta::jsontoolkit::JSON> maybe_schema_dialect{
      resolver(dialect)};
  if (!maybe_schema_dialect.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaResolutionError(
        dialect, "Could not resolve the requested schema");
  }
  const sourcemeta::jsontoolkit::JSON &schema_dialect{
      maybe_schema_dialect.value()};
  // At this point we are sure that the dialect is vocabulary aware and the
  // identifier keyword is indeed `$id`, so we can avoid the added
  // complexity of the generic `id` function.
  assert(schema_dialect.defines("$id") &&
         schema_dialect.at("$id").is_string() &&
         schema_dialect.at("$id").to_string() == dialect);

  /*
   * (3) Retrieve the vocabularies explicitly or implicitly declared by the
   * dialect
   */

  std::map<std::string, bool> result;
  const std::string core{core_vocabulary(base_dialect)};
  if (schema_dialect.defines("$vocabulary")) {
    const sourcemeta::jsontoolkit::JSON &vocabularies{
        schema_dialect.at("$vocabulary")};
    assert(vocabularies.is_object());
    for (const auto &[key, value] : vocabularies.as_object()) {
      result.insert({key, value.to_boolean()});
    }
  } else {
    result.insert({core, true});
  }

  // The specification recommends these checks
  if (!result.contains(core)) {
    throw sourcemeta::jsontoolkit::SchemaError(
        "The core vocabulary must always be present");
  } else if (!result.at(core)) {
    throw sourcemeta::jsontoolkit::SchemaError(
        "The core vocabulary must always be required");
  }

  return result;
}

auto sourcemeta::jsontoolkit::schema_format_compare(
    const sourcemeta::jsontoolkit::JSON::String &left,
    const sourcemeta::jsontoolkit::JSON::String &right) -> bool {
  using Rank =
      std::map<JSON::String, std::uint64_t, std::less<JSON::String>,
               JSON::Allocator<std::pair<const JSON::String, std::uint64_t>>>;
  static Rank rank{// Most core keywords tend to come first
                   {"$schema", 0},
                   {"$id", 1},
                   {"id", 2},
                   {"$vocabulary", 3},
                   {"$anchor", 4},
                   {"$dynamicAnchor", 5},
                   {"$recursiveAnchor", 6},

                   // Then important metadata about the schema
                   {"title", 7},
                   {"description", 8},
                   {"$comment", 10},
                   {"examples", 11},
                   {"deprecated", 12},
                   {"readOnly", 13},
                   {"writeOnly", 14},
                   {"default", 15},

                   // Then references
                   {"$ref", 16},
                   {"$dynamicRef", 17},
                   {"$recursiveRef", 18},

                   // Then keywords that apply to any type
                   {"type", 19},
                   {"disallow", 20},
                   {"extends", 21},
                   {"const", 22},
                   {"enum", 23},
                   {"optional", 0},
                   {"requires", 0},
                   {"allOf", 24},
                   {"anyOf", 25},
                   {"oneOf", 26},
                   {"not", 27},
                   {"if", 28},
                   {"then", 29},
                   {"else", 30},

                   // Then keywords about numbers
                   {"exclusiveMaximum", 31},
                   {"maximum", 32},
                   {"maximumCanEqual", 33},
                   {"exclusiveMinimum", 34},
                   {"minimum", 35},
                   {"minimumCanEqual", 36},
                   {"multipleOf", 37},
                   {"divisibleBy", 38},
                   {"maxDecimal", 39},

                   // Then keywords about strings
                   {"pattern", 40},
                   {"format", 41},
                   {"maxLength", 42},
                   {"minLength", 43},
                   {"contentEncoding", 44},
                   {"contentMediaType", 45},
                   {"contentSchema", 46},

                   // Then keywords about arrays
                   {"maxItems", 47},
                   {"minItems", 48},
                   {"uniqueItems", 49},
                   {"maxContains", 50},
                   {"minContains", 51},
                   {"contains", 52},
                   {"prefixItems", 53},
                   {"items", 54},
                   {"additionalItems", 55},
                   {"unevaluatedItems", 56},

                   // Object
                   {"required", 57},
                   {"maxProperties", 58},
                   {"minProperties", 59},
                   {"propertyNames", 60},
                   {"properties", 61},
                   {"patternProperties", 62},
                   {"additionalProperties", 63},
                   {"unevaluatedProperties", 64},
                   {"dependentRequired", 65},
                   {"dependencies", 66},
                   {"dependentSchemas", 67},

                   // Reusable utilities go last
                   {"$defs", 68},
                   {"definitions", 69}};

  if (rank.contains(left) || rank.contains(right)) {
    constexpr auto DEFAULT{std::numeric_limits<Rank::mapped_type>::max()};
    const auto left_rank{rank.contains(left) ? rank.at(left) : DEFAULT};
    const auto right_rank{rank.contains(right) ? rank.at(right) : DEFAULT};
    // If the ranks are equal, then either the keywords are the same or
    // none of them are recognized keywords.
    assert((left_rank != right_rank) ||
           (left == right || left_rank == DEFAULT));
    return left_rank < right_rank;
  } else {
    // For unknown keywords, go alphabetically
    return left < right;
  }
}
