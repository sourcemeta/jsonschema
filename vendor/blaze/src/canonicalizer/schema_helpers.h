#ifndef SOURCEMETA_BLAZE_CANONICALIZER_SCHEMA_HELPERS_H
#define SOURCEMETA_BLAZE_CANONICALIZER_SCHEMA_HELPERS_H

#include <sourcemeta/core/json.h>

#include <algorithm>   // std::ranges::all_of
#include <string_view> // std::string_view
#include <utility>     // std::to_underlying

namespace sourcemeta::blaze {

// TODO: Unify these with the equivalent helpers in the sibling modules

inline auto is_empty_schema(const sourcemeta::core::JSON &schema) -> bool {
  return (schema.is_boolean() && schema.to_boolean()) ||
         (schema.is_object() && schema.empty());
}

inline auto parse_schema_type_string(const sourcemeta::core::JSON::String &type,
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

inline auto parse_schema_type(const sourcemeta::core::JSON &type)
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

// The dialect a schema declares, honouring the marker that the upgrade rules
// leave behind while they walk a document across drafts
inline auto declared_dialect(const sourcemeta::core::JSON &schema)
    -> std::string_view {
  if (!schema.is_object()) {
    return {};
  }

  const auto *override_value{
      schema.try_at("x-sourcemeta-dialect-override-subschema")};
  if (override_value != nullptr && override_value->is_string()) {
    return override_value->to_string();
  }

  const auto *dialect{schema.try_at("$schema")};
  if (dialect != nullptr && dialect->is_string()) {
    return dialect->to_string();
  }

  return {};
}

// The meta-schema describes `required` and its relatives as an array of unique
// property names. One that does not satisfy that names no property at all, so
// no rule may derive anything from it
inline auto is_property_name_array(const sourcemeta::core::JSON &value)
    -> bool {
  return value.is_array() && value.unique() &&
         std::ranges::all_of(value.as_array(), [](const auto &entry) -> bool {
           return entry.is_string();
         });
}

} // namespace sourcemeta::blaze

#endif
