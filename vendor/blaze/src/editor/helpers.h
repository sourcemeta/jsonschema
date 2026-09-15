#ifndef SOURCEMETA_BLAZE_EDITOR_HELPERS_H
#define SOURCEMETA_BLAZE_EDITOR_HELPERS_H

#include <sourcemeta/core/jsonschema.h>

#include <cassert>     // assert
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

inline auto id_keyword(const sourcemeta::core::SchemaBaseDialect base_dialect)
    -> std::string_view {
  switch (base_dialect) {
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
      return "$id";
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_2_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_1_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_0_HYPER:
      return "id";
  }

  assert(false);
  return "$id";
}

inline auto anonymize(sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaBaseDialect base_dialect)
    -> void {
  if (schema.is_object()) {
    schema.erase(id_keyword(base_dialect));
  }
}

} // namespace sourcemeta::blaze

#endif
