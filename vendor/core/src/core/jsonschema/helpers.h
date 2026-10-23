#ifndef SOURCEMETA_CORE_JSONSCHEMA_HELPERS_H
#define SOURCEMETA_CORE_JSONSCHEMA_HELPERS_H

#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/uri.h>

#include <array>         // std::array
#include <deque>         // std::deque
#include <optional>      // std::optional
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::pair, std::move, std::unreachable
#include <vector>        // std::vector

namespace sourcemeta::core {

using namespace std::string_view_literals;

constexpr auto JSONSCHEMA_HASH_ID{JSON::Object::hash("$id"sv)};
constexpr auto JSONSCHEMA_HASH_LEGACY_ID{JSON::Object::hash("id"sv)};
constexpr auto JSONSCHEMA_HASH_SCHEMA{JSON::Object::hash("$schema"sv)};
constexpr auto JSONSCHEMA_HASH_REF{JSON::Object::hash("$ref"sv)};
constexpr auto JSONSCHEMA_HASH_RECURSIVE_REF{
    JSON::Object::hash("$recursiveRef"sv)};
constexpr auto JSONSCHEMA_HASH_DYNAMIC_REF{JSON::Object::hash("$dynamicRef"sv)};
constexpr auto JSONSCHEMA_HASH_ANCHOR{JSON::Object::hash("$anchor"sv)};
constexpr auto JSONSCHEMA_HASH_DYNAMIC_ANCHOR{
    JSON::Object::hash("$dynamicAnchor"sv)};
constexpr auto JSONSCHEMA_HASH_RECURSIVE_ANCHOR{
    JSON::Object::hash("$recursiveAnchor"sv)};
constexpr auto JSONSCHEMA_HASH_VOCABULARY{JSON::Object::hash("$vocabulary"sv)};
constexpr auto JSONSCHEMA_HASH_DEFS{JSON::Object::hash("$defs"sv)};
constexpr auto JSONSCHEMA_HASH_DEFINITIONS{JSON::Object::hash("definitions"sv)};
constexpr auto EXTENSION_DEFINITIONS_KEYWORD{"x-definitions"sv};
constexpr auto DIALECT_OVERRIDE_KEYWORD{
    "x-sourcemeta-dialect-override-subschema"sv};
constexpr auto JSONSCHEMA_HASH_DIALECT_OVERRIDE{
    JSON::Object::hash(DIALECT_OVERRIDE_KEYWORD)};

/// A keyword whose name is only known once the base dialect is, paired with
/// the hash of that name so that looking it up does not have to hash it again
struct SchemaKeyword {
  JSON::StringView name;
  JSON::Object::hash_type hash;
};

auto base_dialect_uri(const SchemaBaseDialect base_dialect) -> std::string_view;

auto identify(const sourcemeta::core::JSON &schema,
              const SchemaResolver &resolver,
              std::string_view default_dialect = "",
              bool allow_dialect_override = true) -> std::string_view;
auto identify(const sourcemeta::core::JSON &schema,
              const SchemaBaseDialect base_dialect,
              std::string_view default_id = "") -> std::string_view;

auto base_dialect(const sourcemeta::core::JSON &schema,
                  const SchemaResolver &resolver,
                  std::string_view default_dialect = "",
                  bool allow_dialect_override = true)
    -> std::optional<SchemaBaseDialect>;

auto dialect(const sourcemeta::core::JSON &schema,
             std::string_view default_dialect = "",
             bool allow_dialect_override = true) -> std::string_view;

auto vocabularies(const SchemaResolver &resolver,
                  const SchemaBaseDialect base_dialect,
                  std::string_view dialect) -> SchemaVocabularies;
auto to_base_dialect(const std::string_view base_dialect)
    -> std::optional<SchemaBaseDialect>;
auto metaschema_try_embedded(const sourcemeta::core::JSON &schema,
                             std::string_view identifier,
                             const SchemaResolver &resolver)
    -> const sourcemeta::core::JSON *;
auto vocabulary_uri(SchemaVocabularies::Known vocabulary) -> std::string_view;
auto vocabulary_uri(const SchemaVocabularies::URI &vocabulary)
    -> std::string_view;

inline auto id_keyword(const SchemaBaseDialect base_dialect) -> SchemaKeyword {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_SCHEMA_2020_12:
    case SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_2019_09:
    case SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
      return {.name = "$id"sv, .hash = JSONSCHEMA_HASH_ID};
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_2_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_1_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_0_HYPER:
      return {.name = "id"sv, .hash = JSONSCHEMA_HASH_LEGACY_ID};
  }

    // Clang settles that no other value arrives, so only GCC and MSVC
    // need to be told that the end of this function is never reached
#if !defined(__clang__)
  std::unreachable();
#endif
}

// Whether the dialect in force gives a keyword any meaning at all, which is
// what the vocabularies it draws on say rather than what its base dialect
// happens to name
inline auto dialect_defines(const SchemaWalker &walker,
                            const SchemaVocabularies &vocabularies,
                            const std::string_view keyword) -> bool {
  return walker(keyword, vocabularies).type != SchemaKeywordType::Unknown;
}

// A base dialect names the keyword that introduces an identifier, but a
// dialect built on top of it may leave that keyword out of its own
// vocabularies, in which case nothing the schema writes can introduce one.
// Every dialect the specifications define keeps the two in step, so this only
// parts ways for a custom dialect that drops the keyword
inline auto dialect_defines_identifier(const SchemaWalker &walker,
                                       const SchemaVocabularies &vocabularies,
                                       const SchemaBaseDialect base_dialect)
    -> bool {
  return dialect_defines(walker, vocabularies, id_keyword(base_dialect).name);
}

// A schema may pin a meta-schema that it carries within itself, so looking
// there before the resolver is what keeps a document that describes itself
// from being read against whatever else answers to that name. The result
// borrows both of them, so it must not outlive either
inline auto resolver_with_embedded(const sourcemeta::core::JSON &schema,
                                   const SchemaResolver &resolver)
    -> SchemaResolver {
  return [&schema,
          &resolver](const std::string_view target) -> SchemaResolverResult {
    const auto *embedded{
        sourcemeta::core::metaschema_try_embedded(schema, target, resolver)};
    if (embedded != nullptr) {
      return *embedded;
    }

    return resolver(target);
  };
}

inline auto vocabularies_with_embedded(const sourcemeta::core::JSON &schema,
                                       const SchemaResolver &resolver,
                                       const SchemaBaseDialect base_dialect,
                                       const std::string_view dialect)
    -> SchemaVocabularies {
  return sourcemeta::core::vocabularies(
      resolver_with_embedded(schema, resolver), base_dialect, dialect);
}

// Every name this implementation knows for a location reserved for schema
// definitions, in the order a dialect that reserves more than one prefers them
constexpr auto DEFINITIONS_KEYWORDS{std::to_array<std::string_view>(
    {"$defs", "definitions", EXTENSION_DEFINITIONS_KEYWORD})};

// Which of those names the dialect in force actually reserves. Asking its own
// vocabularies rather than its base dialect is what keeps a dialect that drops
// or renames the location from being handed the one its base dialect would use
inline auto definitions_keyword(const SchemaWalker &walker,
                                const SchemaVocabularies &vocabularies)
    -> std::string_view {
  for (const auto candidate : DEFINITIONS_KEYWORDS) {
    if (walker(candidate, vocabularies).type ==
        SchemaKeywordType::LocationMembers) {
      return candidate;
    }
  }

  return {};
}

inline auto definitions_keyword(const SchemaBaseDialect base_dialect)
    -> std::string_view {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_SCHEMA_2020_12:
    case SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_2019_09:
    case SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
      return "$defs";
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
      return "definitions";
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_2_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_1_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_0_HYPER:
      return "";
  }

    // Clang settles that no other value arrives, so only GCC and MSVC
    // need to be told that the end of this function is never reached
#if !defined(__clang__)
  std::unreachable();
#endif
}

// In older drafts, the presence of `$ref` would override any sibling keywords
// See
// https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
inline auto
ref_overrides_adjacent_keywords(const SchemaBaseDialect base_dialect) -> bool {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_2_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_1_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_0_HYPER:
      return true;
    default:
      return false;
  }
}

// An identifier that reads as no URI has no canonical form to be compared
// against, which is as good as not matching whatever it is held up to
inline auto maybe_canonicalize(const std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON::String> {
  try {
    return sourcemeta::core::URI::canonicalize(identifier);
  } catch (const sourcemeta::core::URIParseError &) {
    return std::nullopt;
  }
}

inline auto embedded_metaschema_identifier_matches(
    const sourcemeta::core::JSON &candidate, const SchemaKeyword keyword,
    const std::string_view identifier,
    const std::optional<sourcemeta::core::JSON::String> &canonical) -> bool {
  const auto *value{candidate.try_at(keyword.name, keyword.hash)};
  if ((value == nullptr) || !value->is_string()) {
    return false;
  }

  const auto &current{value->to_string()};
  if (current == identifier) {
    return true;
  }

  if (canonical.has_value()) {
    try {
      return sourcemeta::core::URI::canonicalize(current) == canonical.value();
    } catch (const sourcemeta::core::URIParseError &) {
      return false;
    }
  }

  return false;
}

inline auto embedded_metaschema_matches(
    const sourcemeta::core::JSON &candidate, const std::string_view identifier,
    const std::optional<sourcemeta::core::JSON::String> &canonical) -> bool {
  if (!candidate.is_object()) {
    return false;
  }

  constexpr std::array<SchemaKeyword, 2> KEYWORDS{
      {{.name = "$id"sv, .hash = JSONSCHEMA_HASH_ID},
       {.name = "id"sv, .hash = JSONSCHEMA_HASH_LEGACY_ID}}};
  for (const auto keyword : KEYWORDS) {
    if (embedded_metaschema_identifier_matches(candidate, keyword, identifier,
                                               canonical)) {
      return true;
    }
  }

  return false;
}

inline auto
embedded_metaschema_candidate(const sourcemeta::core::JSON &document,
                              const std::string_view identifier)
    -> std::pair<const sourcemeta::core::JSON *, std::string_view> {
  if (!document.is_object()) {
    return {nullptr, ""};
  }

  const auto canonical{maybe_canonicalize(identifier)};

  constexpr std::array<SchemaKeyword, 2> CONTAINERS{
      {{.name = "$defs"sv, .hash = JSONSCHEMA_HASH_DEFS},
       {.name = "definitions"sv, .hash = JSONSCHEMA_HASH_DEFINITIONS}}};
  for (const auto container : CONTAINERS) {
    const auto *entries{document.try_at(container.name, container.hash)};
    if ((entries == nullptr) || !entries->is_object()) {
      continue;
    }

    const auto *direct{
        entries->try_at(sourcemeta::core::JSON::StringView{identifier})};
    if ((direct != nullptr) &&
        embedded_metaschema_matches(*direct, identifier, canonical)) {
      return {direct, container.name};
    }

    for (const auto &entry : entries->as_object()) {
      if (embedded_metaschema_matches(entry.second, identifier, canonical)) {
        return {&entry.second, container.name};
      }
    }
  }

  return {nullptr, ""};
}

inline auto embedded_metaschema_link_valid(const sourcemeta::core::JSON &link,
                                           const std::string_view identifier,
                                           const std::string_view container,
                                           const SchemaBaseDialect base_dialect)
    -> bool {
  // In 2019-09 and 2020-12, `definitions` is still supported for backwards
  // compatibility, so the older name answers for a dialect that reserves the
  // newer one as well as for the ones that reserve it in their own right
  const auto keyword{definitions_keyword(base_dialect)};
  if (container != keyword &&
      !(keyword == "$defs" && container == "definitions")) {
    return false;
  }

  const auto canonical{maybe_canonicalize(identifier)};

  return embedded_metaschema_identifier_matches(link, id_keyword(base_dialect),
                                                identifier, canonical);
}

struct EmbeddedMetaschemaLink {
  const sourcemeta::core::JSON *schema;
  sourcemeta::core::JSON::StringView identifier;
  std::string_view container;
};

} // namespace sourcemeta::core

#endif
