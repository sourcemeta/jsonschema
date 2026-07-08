#include <sourcemeta/blaze/foundation.h>

#include "helpers.h"

#include <cassert>       // assert
#include <sstream>       // std::ostringstream
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::to_underlying

auto sourcemeta::blaze::is_schema(const sourcemeta::core::JSON &schema)
    -> bool {
  return schema.is_object() || schema.is_boolean();
}

// TODO: Make this function detect schemas only using identifier/comment
// keywords, etc
auto sourcemeta::blaze::is_empty_schema(const sourcemeta::core::JSON &schema)
    -> bool {
  return (schema.is_boolean() && schema.to_boolean()) ||
         (schema.is_object() && schema.empty());
}

auto sourcemeta::blaze::to_string(const SchemaBaseDialect base_dialect)
    -> std::string_view {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_2020_12:
      return "https://json-schema.org/draft/2020-12/schema";
    case SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
      return "https://json-schema.org/draft/2020-12/hyper-schema";
    case SchemaBaseDialect::JSON_Schema_2019_09:
      return "https://json-schema.org/draft/2019-09/schema";
    case SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      return "https://json-schema.org/draft/2019-09/hyper-schema";
    case SchemaBaseDialect::JSON_Schema_Draft_7:
      return "http://json-schema.org/draft-07/schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
      return "http://json-schema.org/draft-07/hyper-schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_6:
      return "http://json-schema.org/draft-06/schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
      return "http://json-schema.org/draft-06/hyper-schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_4:
      return "http://json-schema.org/draft-04/schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
      return "http://json-schema.org/draft-04/hyper-schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_3:
      return "http://json-schema.org/draft-03/schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
      return "http://json-schema.org/draft-03/hyper-schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_2_Hyper:
      return "http://json-schema.org/draft-02/hyper-schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_1_Hyper:
      return "http://json-schema.org/draft-01/hyper-schema#";
    case SchemaBaseDialect::JSON_Schema_Draft_0_Hyper:
      return "http://json-schema.org/draft-00/hyper-schema#";
  }

  assert(false);
  return {};
}

auto sourcemeta::blaze::to_base_dialect(const std::string_view base_dialect)
    -> std::optional<SchemaBaseDialect> {
  if (base_dialect == "https://json-schema.org/draft/2020-12/schema" ||
      base_dialect == "http://json-schema.org/draft/2020-12/schema") {
    return SchemaBaseDialect::JSON_Schema_2020_12;
  } else if (base_dialect ==
                 "https://json-schema.org/draft/2020-12/hyper-schema" ||
             base_dialect ==
                 "http://json-schema.org/draft/2020-12/hyper-schema") {
    return SchemaBaseDialect::JSON_Schema_2020_12_Hyper;
  } else if (base_dialect == "https://json-schema.org/draft/2019-09/schema" ||
             base_dialect == "http://json-schema.org/draft/2019-09/schema") {
    return SchemaBaseDialect::JSON_Schema_2019_09;
  } else if (base_dialect ==
                 "https://json-schema.org/draft/2019-09/hyper-schema" ||
             base_dialect ==
                 "http://json-schema.org/draft/2019-09/hyper-schema") {
    return SchemaBaseDialect::JSON_Schema_2019_09_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-07/schema#" ||
             base_dialect == "https://json-schema.org/draft-07/schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_7;
  } else if (base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-07/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_7_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-06/schema#" ||
             base_dialect == "https://json-schema.org/draft-06/schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_6;
  } else if (base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-06/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_6_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-04/schema#" ||
             base_dialect == "https://json-schema.org/draft-04/schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_4;
  } else if (base_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-04/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_4_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-03/schema#" ||
             base_dialect == "https://json-schema.org/draft-03/schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_3;
  } else if (base_dialect == "http://json-schema.org/draft-03/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-03/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_3_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-02/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-02/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_2_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-01/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-01/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_1_Hyper;
  } else if (base_dialect == "http://json-schema.org/draft-00/hyper-schema#" ||
             base_dialect == "https://json-schema.org/draft-00/hyper-schema#") {
    return SchemaBaseDialect::JSON_Schema_Draft_0_Hyper;
  }

  return std::nullopt;
}

auto sourcemeta::blaze::identify(const sourcemeta::core::JSON &schema,
                                 const SchemaResolver &resolver,
                                 std::string_view default_dialect,
                                 std::string_view default_id,
                                 const bool allow_dialect_override)
    -> std::string_view {
  try {
    const auto maybe_base_dialect{sourcemeta::blaze::base_dialect(
        schema, resolver, default_dialect, allow_dialect_override)};
    if (maybe_base_dialect.has_value()) {
      return identify(schema, maybe_base_dialect.value(), default_id);
    }
    return default_id;
  } catch (const SchemaResolutionError &) {
    if (!default_id.empty()) {
      return default_id;
    }
    throw;
  }
}

auto sourcemeta::blaze::identify(const sourcemeta::core::JSON &schema,
                                 const SchemaBaseDialect base_dialect,
                                 std::string_view default_id)
    -> std::string_view {
  if (!schema.is_object()) {
    return default_id;
  }

  const std::string keyword{sourcemeta::blaze::id_keyword(base_dialect)};

  if (!schema.defines(keyword)) {
    return default_id;
  }

  const auto &identifier{schema.at(keyword)};
  if (!identifier.is_string()) {
    std::ostringstream value;
    sourcemeta::core::stringify(identifier, value);
    throw sourcemeta::blaze::SchemaKeywordError(
        keyword, value.str(), "The schema identifier is invalid");
  }

  // In older drafts, the presence of `$ref` would override any sibling
  // keywords. Note that `$ref` was first introduced in Draft 3, so we
  // don't check for base dialects lower than that.
  // See
  // https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
  if (schema.defines("$ref") &&
      (base_dialect == SchemaBaseDialect::JSON_Schema_Draft_7 ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_7_Hyper ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_6 ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_6_Hyper ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_4 ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_4_Hyper ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_3 ||
       base_dialect == SchemaBaseDialect::JSON_Schema_Draft_3_Hyper)) {
    return default_id;
  }

  // An empty string identifier and an identifier consisting solely of the
  // empty-fragment marker "#" are both valid URI-references that resolve to
  // the parent base, carrying no information. We treat them as if no
  // identifier was declared at all (i.e. no new schema resource is
  // introduced).
  // See
  // https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.8.2.2
  // See
  // https://json-schema.org/draft/2020-12/draft-bhutton-json-schema-01#section-8.2.1-5
  if (identifier.to_string().empty() || identifier.to_string() == "#") {
    return default_id;
  }

  return identifier.to_string();
}

auto sourcemeta::blaze::anonymize(sourcemeta::core::JSON &schema,
                                  const SchemaBaseDialect base_dialect)
    -> void {
  if (schema.is_object()) {
    schema.erase(sourcemeta::blaze::id_keyword(base_dialect));
  }
}

auto sourcemeta::blaze::reidentify(sourcemeta::core::JSON &schema,
                                   std::string_view new_identifier,
                                   const SchemaResolver &resolver,
                                   std::string_view default_dialect) -> void {
  const auto resolved_base_dialect{
      sourcemeta::blaze::base_dialect(schema, resolver, default_dialect)};
  if (!resolved_base_dialect.has_value()) {
    throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
  }

  reidentify(schema, new_identifier, resolved_base_dialect.value());
}

auto sourcemeta::blaze::reidentify(sourcemeta::core::JSON &schema,
                                   std::string_view new_identifier,
                                   const SchemaBaseDialect base_dialect)
    -> void {
  assert(is_schema(schema));
  assert(schema.is_object());
  schema.assign(sourcemeta::blaze::id_keyword(base_dialect),
                sourcemeta::core::JSON{new_identifier});

  // If we reidentify, and the identifier is still not retrievable, then
  // we are facing the Draft 7 `$ref` sibling edge case, and we cannot
  // really continue
  if (schema.defines("$ref") && identify(schema, base_dialect).empty()) {
    throw SchemaReferenceObjectResourceError(new_identifier);
  }
}

auto sourcemeta::blaze::dialect(const sourcemeta::core::JSON &schema,
                                std::string_view default_dialect,
                                const bool allow_dialect_override)
    -> std::string_view {
  assert(sourcemeta::blaze::is_schema(schema));

  if (allow_dialect_override && schema.is_object()) {
    const auto *override_value{
        schema.try_at("x-sourcemeta-dialect-override-subschema")};
    if (override_value && override_value->is_string() &&
        !override_value->to_string().empty()) {
      return override_value->to_string();
    }
  }

  if (schema.is_boolean() || !schema.defines("$schema")) {
    return default_dialect;
  }

  const auto &dialect_value{schema.at("$schema")};
  if (!dialect_value.is_string()) {
    std::ostringstream value;
    sourcemeta::core::stringify(dialect_value, value);
    throw sourcemeta::blaze::SchemaKeywordError("$schema", value.str(),
                                                "The dialect value is invalid");
  }

  return dialect_value.to_string();
}

// A meta-schema that is not known to the resolver may still be embedded in
// the document itself. Across every official base dialect, the only
// containers that can hold embedded resources are `$defs` and `definitions`,
// which no custom dialect can redefine away. A candidate only counts if its
// entire meta-schema chain terminates at an official base dialect and every
// embedded link declares its identifier and sits in a container in a way
// that is valid for such base dialect
auto sourcemeta::blaze::metaschema_try_embedded(
    const sourcemeta::core::JSON &schema, const std::string_view identifier,
    const SchemaResolver &resolver) -> const sourcemeta::core::JSON * {
  // Relative or invalid meta-schema references are not acceptable
  // according to the JSON Schema specifications
  if (!sourcemeta::core::URI::is_uri(identifier)) {
    return nullptr;
  }

  const auto candidate{
      sourcemeta::blaze::embedded_metaschema_candidate(schema, identifier)};
  if (!candidate.first) {
    return nullptr;
  }

  std::unordered_set<std::string_view> visited;
  std::vector<sourcemeta::blaze::EmbeddedMetaschemaLink> links{
      {.schema = candidate.first,
       .identifier = identifier,
       .container = candidate.second}};
  // Chain links that the resolver knows about are returned by value, so we
  // keep them alive while we walk the chain, in a container that never
  // relocates its elements, as we hold views into them
  std::deque<sourcemeta::core::JSON> resolved;
  const auto *current{candidate.first};
  std::string_view current_identifier{identifier};
  std::optional<SchemaBaseDialect> terminal;

  while (true) {
    // The meta-schema is present, but its chain can never terminate at an
    // official base dialect, just like a self-descriptive or cyclic
    // meta-schema that the resolver knows about
    if (!visited.emplace(current_identifier).second) {
      throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
    }

    if (!current->is_object()) {
      throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
    }

    const auto *metaschema_dialect{current->try_at("$schema")};
    if (!metaschema_dialect || !metaschema_dialect->is_string()) {
      throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
    }

    const auto &dialect_uri{metaschema_dialect->to_string()};
    const auto known{sourcemeta::blaze::to_base_dialect(dialect_uri)};
    if (known.has_value()) {
      terminal = known;
      break;
    }

    auto remote{resolver(dialect_uri)};
    if (remote.has_value()) {
      resolved.push_back(std::move(remote).value());
      current = &resolved.back();
      current_identifier = dialect_uri;
      continue;
    }

    if (!sourcemeta::core::URI::is_uri(dialect_uri)) {
      return nullptr;
    }

    const auto next{
        sourcemeta::blaze::embedded_metaschema_candidate(schema, dialect_uri)};
    if (!next.first) {
      return nullptr;
    }

    links.push_back({.schema = next.first,
                     .identifier = dialect_uri,
                     .container = next.second});
    current = next.first;
    current_identifier = dialect_uri;
  }

  assert(terminal.has_value());
  for (const auto &link : links) {
    if (!sourcemeta::blaze::embedded_metaschema_link_valid(
            *(link.schema), link.identifier, link.container,
            terminal.value())) {
      return nullptr;
    }
  }

  return candidate.first;
}

auto sourcemeta::blaze::metaschema(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaResolver &resolver,
    std::string_view default_dialect) -> sourcemeta::core::JSON {
  const auto effective_dialect{
      sourcemeta::blaze::dialect(schema, default_dialect)};
  if (effective_dialect.empty()) {
    throw sourcemeta::blaze::SchemaUnknownDialectError();
  }

  // A meta-schema that is embedded in the schema itself takes precedence
  // over what the resolver knows about, as the schema pins the exact
  // meta-schema it is described by
  const auto *embedded{sourcemeta::blaze::metaschema_try_embedded(
      schema, effective_dialect, resolver)};
  if (embedded) {
    return *embedded;
  }

  const auto maybe_metaschema{resolver(effective_dialect)};
  if (!maybe_metaschema.has_value()) {
    // Relative meta-schema references are invalid according to the
    // JSON Schema specifications. They must be absolute ones
    const sourcemeta::core::URI effective_dialect_uri{effective_dialect};
    if (effective_dialect_uri.is_relative()) {
      throw sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError(
          effective_dialect);
    } else {
      throw sourcemeta::blaze::SchemaResolutionError(
          effective_dialect, "Could not resolve the metaschema of the schema");
    }
  }

  return maybe_metaschema.value();
}

static auto
base_dialect_with_visited(const sourcemeta::core::JSON &schema,
                          const sourcemeta::blaze::SchemaResolver &resolver,
                          std::string_view default_dialect,
                          std::unordered_set<std::string_view> &visited,
                          const bool allow_dialect_override,
                          const sourcemeta::core::JSON &document)
    -> std::optional<sourcemeta::blaze::SchemaBaseDialect> {
  assert(sourcemeta::blaze::is_schema(schema));
  const std::string_view effective_dialect{sourcemeta::blaze::dialect(
      schema, default_dialect, allow_dialect_override)};

  // There is no metaschema information whatsoever
  // Nothing we can do at this point
  if (effective_dialect.empty()) {
    return std::nullopt;
  }

  // Check for known base dialects
  const auto result{sourcemeta::blaze::to_base_dialect(effective_dialect)};
  if (result.has_value()) {
    return result;
  }

  // Detect cycles in the metaschema chain
  if (!visited.emplace(effective_dialect).second) {
    throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
  }

  // A meta-schema that is embedded in the original document itself takes
  // precedence over what the resolver knows about, as the document pins
  // the exact meta-schema it is described by
  const auto *embedded{sourcemeta::blaze::metaschema_try_embedded(
      document, effective_dialect, resolver)};
  if (embedded) {
    const std::string_view embedded_dialect{sourcemeta::blaze::dialect(
        *embedded, effective_dialect, allow_dialect_override)};
    if (embedded_dialect == effective_dialect) {
      throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
    }

    return base_dialect_with_visited(*embedded, resolver, effective_dialect,
                                     visited, allow_dialect_override, document);
  }

  // Otherwise, traverse the metaschema hierarchy up
  const std::optional<sourcemeta::core::JSON> metaschema{
      resolver(effective_dialect)};
  if (!metaschema.has_value()) {
    sourcemeta::core::URI effective_dialect_uri;
    try {
      effective_dialect_uri = sourcemeta::core::URI{effective_dialect};
    } catch (const sourcemeta::core::URIParseError &) {
      throw sourcemeta::blaze::SchemaKeywordError(
          "$schema", effective_dialect, "The dialect is not a valid URI");
    }

    // Relative meta-schema references are invalid according to the
    // JSON Schema specifications. They must be absolute ones
    if (effective_dialect_uri.is_relative()) {
      throw sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError(
          effective_dialect);
    } else {
      throw sourcemeta::blaze::SchemaResolutionError(
          effective_dialect, "Could not resolve the metaschema of the schema");
    }
  }

  // If the metaschema declares the same dialect (self-descriptive), and it's
  // not an official dialect, we cannot determine the base dialect
  const std::string_view metaschema_dialect{sourcemeta::blaze::dialect(
      metaschema.value(), effective_dialect, allow_dialect_override)};
  if (metaschema_dialect == effective_dialect) {
    throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
  }

  return base_dialect_with_visited(metaschema.value(), resolver,
                                   effective_dialect, visited,
                                   allow_dialect_override, document);
}

auto sourcemeta::blaze::base_dialect(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaResolver &resolver,
    std::string_view default_dialect, const bool allow_dialect_override)
    -> std::optional<SchemaBaseDialect> {
  std::unordered_set<std::string_view> visited;
  return base_dialect_with_visited(schema, resolver, default_dialect, visited,
                                   allow_dialect_override, schema);
}

namespace {
auto core_vocabulary_known(
    const sourcemeta::blaze::SchemaBaseDialect base_dialect)
    -> sourcemeta::blaze::Vocabularies::Known {
  using sourcemeta::blaze::SchemaBaseDialect;
  using sourcemeta::blaze::Vocabularies;
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_2020_12:
    case SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
      return Vocabularies::Known::JSON_Schema_2020_12_Core;
    case SchemaBaseDialect::JSON_Schema_2019_09:
    case SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      return Vocabularies::Known::JSON_Schema_2019_09_Core;
    default:
      assert(false);
      return Vocabularies::Known::JSON_Schema_2020_12_Core;
  }
}

auto dialect_to_known(const std::string_view dialect)
    -> std::optional<sourcemeta::blaze::Vocabularies::Known> {
  using sourcemeta::blaze::Vocabularies;
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

auto base_dialect_to_known(const sourcemeta::blaze::SchemaBaseDialect dialect)
    -> sourcemeta::blaze::Vocabularies::Known {
  using sourcemeta::blaze::SchemaBaseDialect;
  using sourcemeta::blaze::Vocabularies;
  switch (dialect) {
    case SchemaBaseDialect::JSON_Schema_Draft_7:
      return Vocabularies::Known::JSON_Schema_Draft_7;
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_7_Hyper;
    case SchemaBaseDialect::JSON_Schema_Draft_6:
      return Vocabularies::Known::JSON_Schema_Draft_6;
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_6_Hyper;
    case SchemaBaseDialect::JSON_Schema_Draft_4:
      return Vocabularies::Known::JSON_Schema_Draft_4;
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_4_Hyper;
    case SchemaBaseDialect::JSON_Schema_Draft_3:
      return Vocabularies::Known::JSON_Schema_Draft_3;
    case SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_3_Hyper;
    case SchemaBaseDialect::JSON_Schema_Draft_2_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_2_Hyper;
    case SchemaBaseDialect::JSON_Schema_Draft_1_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_1_Hyper;
    case SchemaBaseDialect::JSON_Schema_Draft_0_Hyper:
      return Vocabularies::Known::JSON_Schema_Draft_0_Hyper;
    default:
      assert(false);
      return Vocabularies::Known::JSON_Schema_Draft_7;
  }
}

auto is_pre_vocabulary_base_dialect(
    const sourcemeta::blaze::SchemaBaseDialect base_dialect) -> bool {
  using sourcemeta::blaze::SchemaBaseDialect;
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_Draft_7:
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_6:
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_4:
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_3:
    case SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_2_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_1_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_0_Hyper:
      return true;
    default:
      return false;
  }
}
} // namespace

auto sourcemeta::blaze::parse_vocabularies(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaBaseDialect base_dialect)
    -> std::optional<sourcemeta::blaze::Vocabularies> {
  if (base_dialect !=
          sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12 &&
      base_dialect !=
          sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12_Hyper &&
      base_dialect !=
          sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09 &&
      base_dialect !=
          sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09_Hyper) {
    return std::nullopt;
  }

  if (!schema.is_object()) {
    return std::nullopt;
  }

  const auto *vocabulary_entry{schema.try_at("$vocabulary")};
  if (!vocabulary_entry) {
    return std::nullopt;
  }

  if (!vocabulary_entry->is_object()) {
    return std::nullopt;
  }

  sourcemeta::blaze::Vocabularies result;
  for (const auto &entry : vocabulary_entry->as_object()) {
    if (!entry.second.is_boolean()) {
      return std::nullopt;
    }

    result.insert(entry.first, entry.second.to_boolean());
  }

  return result;
}

auto sourcemeta::blaze::parse_vocabularies(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaResolver &resolver,
    std::string_view default_dialect)
    -> std::optional<sourcemeta::blaze::Vocabularies> {
  const auto schema_base_dialect{
      sourcemeta::blaze::base_dialect(schema, resolver, default_dialect)};
  if (schema_base_dialect.has_value()) {
    return sourcemeta::blaze::parse_vocabularies(schema,
                                                 schema_base_dialect.value());
  } else {
    return std::nullopt;
  }
}

auto sourcemeta::blaze::vocabularies(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaResolver &resolver,
    std::string_view default_dialect) -> sourcemeta::blaze::Vocabularies {
  const auto resolved_base_dialect{
      sourcemeta::blaze::base_dialect(schema, resolver, default_dialect)};
  if (!resolved_base_dialect.has_value()) {
    throw sourcemeta::blaze::SchemaUnknownBaseDialectError();
  }

  const std::string_view resolved_dialect{
      sourcemeta::blaze::dialect(schema, default_dialect)};
  if (resolved_dialect.empty()) {
    // If the schema has no declared metaschema and the user didn't
    // provide a explicit default, then we cannot do anything.
    // Better to abort instead of trying to guess.
    throw sourcemeta::blaze::SchemaUnknownDialectError();
  }

  // A meta-schema that is embedded in the schema itself takes precedence
  // over what the resolver knows about, as the schema pins the exact
  // meta-schema it is described by
  return vocabularies(
      [&schema, &resolver](const std::string_view identifier)
          -> std::optional<sourcemeta::core::JSON> {
        const auto *embedded{sourcemeta::blaze::metaschema_try_embedded(
            schema, identifier, resolver)};
        if (embedded) {
          return *embedded;
        }

        return resolver(identifier);
      },
      resolved_base_dialect.value(), resolved_dialect);
}

auto sourcemeta::blaze::vocabularies(const SchemaResolver &resolver,
                                     const SchemaBaseDialect base_dialect,
                                     std::string_view dialect)
    -> sourcemeta::blaze::Vocabularies {
  const auto base_dialect_string{to_string(base_dialect)};
  // As a performance optimization shortcut
  if (base_dialect_string == dialect ||
      to_base_dialect(dialect) == base_dialect) {
    if (base_dialect == SchemaBaseDialect::JSON_Schema_2020_12) {
      return Vocabularies{
          {Vocabularies::Known::JSON_Schema_2020_12_Core, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Applicator, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Validation, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Format_Annotation, true},
          {Vocabularies::Known::JSON_Schema_2020_12_Content, true}};
    } else if (base_dialect == SchemaBaseDialect::JSON_Schema_2019_09) {
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
    return Vocabularies{{std::string{dialect}, true}};
  }

  /*
   * (2) If the base dialect is pre-vocabularies, then the
   * base dialect itself is conceptually the only vocabulary
   */

  if (is_pre_vocabulary_base_dialect(base_dialect)) {
    return Vocabularies{{base_dialect_to_known(base_dialect), true}};
  }

  /*
   * (3) If the dialect is vocabulary aware, then fetch such dialect
   */

  const std::optional<sourcemeta::core::JSON> maybe_schema_dialect{
      resolver(dialect)};
  if (!maybe_schema_dialect.has_value()) {
    throw sourcemeta::blaze::SchemaResolutionError(
        dialect, "Could not resolve the metaschema of the schema");
  }
  const sourcemeta::core::JSON &schema_dialect{maybe_schema_dialect.value()};
  // At this point we are sure that the dialect is vocabulary aware and the
  // identifier keyword is indeed `$id`, so we can avoid the added
  // complexity of the generic `id` function.
  assert(schema_dialect.defines("$id") && schema_dialect.at("$id").is_string());

  /*
   * (4) Retrieve the vocabularies explicitly or implicitly declared by the
   * dialect
   */

  const auto core{core_vocabulary_known(base_dialect)};
  auto result{parse_vocabularies(schema_dialect, base_dialect)
                  .value_or(Vocabularies{})};
  if (result.empty()) {
    result.insert(core, true);
  }

  // The specification recommends these checks
  if (!result.contains(core)) {
    throw sourcemeta::blaze::SchemaError(
        "The core vocabulary must always be present");
  } else {
    const auto core_status{result.get(core)};
    if (core_status.has_value() && !core_status.value()) {
      throw sourcemeta::blaze::SchemaError(
          "The core vocabulary must always be required");
    }
  }

  return result;
}

static auto parse_schema_type_string(const sourcemeta::core::JSON::String &type,
                                     sourcemeta::core::JSON::TypeSet &result)
    -> void {
  if (type == "null") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Null));
  } else if (type == "boolean") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Boolean));
  } else if (type == "object") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Object));
  } else if (type == "array") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Array));
  } else if (type == "number") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
  } else if (type == "integer") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
  } else if (type == "string") {
    result.set(std::to_underlying(sourcemeta::core::JSON::Type::String));
  }
}

auto sourcemeta::blaze::parse_schema_type(const sourcemeta::core::JSON &type)
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
