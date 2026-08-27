#ifndef SOURCEMETA_BLAZE_COMPILER_SCHEMA_HELPERS_H
#define SOURCEMETA_BLAZE_COMPILER_SCHEMA_HELPERS_H

#include <sourcemeta/core/json.h>

#include <utility> // std::to_underlying

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

} // namespace sourcemeta::blaze

#endif
