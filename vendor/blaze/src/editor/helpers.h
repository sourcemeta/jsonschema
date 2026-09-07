#ifndef SOURCEMETA_BLAZE_EDITOR_HELPERS_H
#define SOURCEMETA_BLAZE_EDITOR_HELPERS_H

#include <sourcemeta/blaze/foundation.h>

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

inline auto anonymize(sourcemeta::core::JSON &schema,
                      const SchemaBaseDialect base_dialect) -> void {
  if (schema.is_object()) {
    schema.erase(id_keyword(base_dialect));
  }
}

} // namespace sourcemeta::blaze

#endif
