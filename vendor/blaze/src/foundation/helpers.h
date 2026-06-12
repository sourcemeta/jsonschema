#ifndef SOURCEMETA_BLAZE_FOUNDATION_HELPERS_H
#define SOURCEMETA_BLAZE_FOUNDATION_HELPERS_H

#include <sourcemeta/blaze/foundation.h>

#include <sourcemeta/core/uri.h>

#include <cassert>          // assert
#include <deque>            // std::deque
#include <initializer_list> // std::initializer_list
#include <optional>         // std::optional
#include <string_view>      // std::string_view
#include <unordered_set>    // std::unordered_set
#include <utility>          // std::pair, std::move
#include <vector>           // std::vector

namespace sourcemeta::blaze {

inline auto id_keyword(const SchemaBaseDialect base_dialect)
    -> std::string_view {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_2020_12:
    case SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
    case SchemaBaseDialect::JSON_Schema_2019_09:
    case SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_7:
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_6:
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
      return "$id";
    case SchemaBaseDialect::JSON_Schema_Draft_4:
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_3:
    case SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_2_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_1_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_0_Hyper:
      return "id";
  }

  assert(false);
  return "$id";
}

inline auto definitions_keyword(const SchemaBaseDialect base_dialect)
    -> std::string_view {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_2020_12:
    case SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
    case SchemaBaseDialect::JSON_Schema_2019_09:
    case SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      return "$defs";
    case SchemaBaseDialect::JSON_Schema_Draft_7:
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_6:
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_4:
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_3:
    case SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
      return "definitions";
    case SchemaBaseDialect::JSON_Schema_Draft_2_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_1_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_0_Hyper:
      return "";
  }

  assert(false);
  return "$defs";
}

// In older drafts, the presence of `$ref` would override any sibling keywords
// See
// https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
inline auto
ref_overrides_adjacent_keywords(const SchemaBaseDialect base_dialect) -> bool {
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

inline auto embedded_metaschema_identifier_matches(
    const sourcemeta::core::JSON &candidate, const std::string_view keyword,
    const std::string_view identifier,
    const std::optional<sourcemeta::core::JSON::String> &canonical) -> bool {
  const auto *value{
      candidate.try_at(sourcemeta::core::JSON::StringView{keyword})};
  if (!value || !value->is_string()) {
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

  for (const auto *const keyword : {"$id", "id"}) {
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

  std::optional<sourcemeta::core::JSON::String> canonical;
  try {
    canonical = sourcemeta::core::URI::canonicalize(identifier);
  } catch (const sourcemeta::core::URIParseError &) {
    canonical = std::nullopt;
  }

  for (const auto *const container : {"$defs", "definitions"}) {
    const auto *entries{document.try_at(container)};
    if (!entries || !entries->is_object()) {
      continue;
    }

    const auto *direct{
        entries->try_at(sourcemeta::core::JSON::StringView{identifier})};
    if (direct && embedded_metaschema_matches(*direct, identifier, canonical)) {
      return {direct, container};
    }

    for (const auto &entry : entries->as_object()) {
      if (embedded_metaschema_matches(entry.second, identifier, canonical)) {
        return {&entry.second, container};
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
  // In 2019-09 and 2020-12, `definitions` is still supported
  // for backwards compatibility
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_2020_12:
    case SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
    case SchemaBaseDialect::JSON_Schema_2019_09:
    case SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      if (container != "$defs" && container != "definitions") {
        return false;
      }

      break;
    default:
      if (container != definitions_keyword(base_dialect)) {
        return false;
      }
  }

  std::optional<sourcemeta::core::JSON::String> canonical;
  try {
    canonical = sourcemeta::core::URI::canonicalize(identifier);
  } catch (const sourcemeta::core::URIParseError &) {
    canonical = std::nullopt;
  }

  return embedded_metaschema_identifier_matches(link, id_keyword(base_dialect),
                                                identifier, canonical);
}

struct EmbeddedMetaschemaLink {
  const sourcemeta::core::JSON *schema;
  sourcemeta::core::JSON::StringView identifier;
  std::string_view container;
};

} // namespace sourcemeta::blaze

#endif
