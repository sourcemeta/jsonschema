#ifndef SOURCEMETA_BLAZE_BUNDLE_HELPERS_H
#define SOURCEMETA_BLAZE_BUNDLE_HELPERS_H

#include <sourcemeta/blaze/foundation.h>

#include <sourcemeta/core/json.h>

#include <cassert>     // assert
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

inline auto id_keyword(const SchemaBaseDialect base_dialect)
    -> std::string_view {
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_SCHEMA_2020_12:
    case SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_2019_09:
    case SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
      return "$id";
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_2_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_1_HYPER:
    case SchemaBaseDialect::JSON_SCHEMA_DRAFT_0_HYPER:
      return "id";
  }

  assert(false);
  return "$id";
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

  assert(false);
  return "$defs";
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

// The dialect a schema declares, falling back to the given default. Unlike
// the equivalent helper in alterschema, this one knows nothing about the
// dialect override marker, as only the upgrade rules ever write one and
// bundling can never observe a document while those are live
inline auto declared_dialect(const sourcemeta::core::JSON &schema,
                             const std::string_view default_dialect)
    -> std::string_view {
  if (!schema.is_object()) {
    return default_dialect;
  }

  const auto *dialect{schema.try_at("$schema")};
  return (dialect != nullptr && dialect->is_string()) ? dialect->to_string()
                                                      : default_dialect;
}

} // namespace sourcemeta::blaze

#endif
