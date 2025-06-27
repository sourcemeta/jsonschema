#include <sourcemeta/core/jsonschema.h>

#include <initializer_list> // std::initializer_list

template <typename T>
auto make_set(std::initializer_list<T> types) -> std::set<T> {
  return {types};
}

auto sourcemeta::core::schema_official_walker(
    std::string_view keyword,
    const sourcemeta::core::Vocabularies &vocabularies)
    -> sourcemeta::core::SchemaWalkerResult {
#define TYPES(...) make_set({__VA_ARGS__})
#define WALK(vocabulary, _keyword, _types, strategy, ...)                      \
  if (vocabularies.contains(vocabulary) && keyword == _keyword)                \
    return {sourcemeta::core::SchemaKeywordType::strategy,                     \
            vocabulary,                                                        \
            {__VA_ARGS__},                                                     \
            _types};

#define WALK_ANY(vocabulary_1, vocabulary_2, _keyword, types, strategy, ...)   \
  WALK(vocabulary_1, _keyword, types, strategy, __VA_ARGS__)                   \
  WALK(vocabulary_2, _keyword, types, strategy, __VA_ARGS__)

#define WALK_MAYBE_DEPENDENT(vocabulary, _keyword, types, strategy,            \
                             dependent_vocabulary, ...)                        \
  if (vocabularies.contains(dependent_vocabulary)) {                           \
    WALK(vocabulary, _keyword, types, strategy, __VA_ARGS__)                   \
  } else {                                                                     \
    WALK(vocabulary, _keyword, types, strategy)                                \
  }

#define HTTPS_BASE "https://json-schema.org/draft/"
  // 2020-12
  WALK(HTTPS_BASE "2020-12/vocab/core", "$id", {}, Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$schema", {}, Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$ref", {}, Reference)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$defs", {}, LocationMembers)
  // JSON Schema still defines this for backwards-compatibility
  // See https://json-schema.org/draft/2020-12/schema
  WALK(HTTPS_BASE "2020-12/vocab/core", "definitions", {}, LocationMembers)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$comment", {}, Comment)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$anchor", {}, Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$vocabulary", {}, Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$dynamicRef", {}, Reference)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$dynamicAnchor", {}, Other)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "oneOf", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "anyOf", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "allOf", {},
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "if", {},
       ApplicatorValueInPlaceMaybe)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "then", {},
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "else", {},
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "not", {},
       ApplicatorValueInPlaceNegate)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/applicator", "properties",
                       TYPES(JSON::Type::Object),
                       ApplicatorMembersTraversePropertyStatic,
                       HTTPS_BASE "2020-12/vocab/validation", "required")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties", "patternProperties")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "patternProperties",
       TYPES(JSON::Type::Object), ApplicatorMembersTraversePropertyRegex)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "propertyNames",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseAnyPropertyKey)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "dependentSchemas",
       TYPES(JSON::Type::Object), ApplicatorMembersInPlaceSome)
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/applicator", "contains",
                       TYPES(JSON::Type::Array), ApplicatorValueTraverseAnyItem,
                       HTTPS_BASE "2020-12/vocab/validation", "minContains",
                       "maxContains")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "items", TYPES(JSON::Type::Array),
       ApplicatorValueTraverseSomeItem, "prefixItems")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "prefixItems",
       TYPES(JSON::Type::Array), ApplicatorElementsTraverseItem)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/validation", "type", {},
                       Assertion, HTTPS_BASE "2020-12/vocab/applicator",
                       "properties")
  WALK(HTTPS_BASE "2020-12/vocab/validation", "enum", {}, Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "const", {}, Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxLength",
       TYPES(JSON::Type::String), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minLength",
       TYPES(JSON::Type::String), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "pattern",
       TYPES(JSON::Type::String), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "exclusiveMinimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "multipleOf",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "type")
  WALK(HTTPS_BASE "2020-12/vocab/validation", "exclusiveMaximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "type")
  WALK(HTTPS_BASE "2020-12/vocab/validation", "dependentRequired",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minProperties",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxProperties",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "required",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxItems",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minItems",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "uniqueItems",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minContains",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxContains",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "title", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "description", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "writeOnly", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "readOnly", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "examples", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "default", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "deprecated", {}, Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/format-annotation", "format",
       TYPES(JSON::Type::String), Annotation)
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/unevaluated",
                       "unevaluatedProperties", TYPES(JSON::Type::Object),
                       ApplicatorValueTraverseSomeProperty,
                       HTTPS_BASE "2020-12/vocab/applicator", "properties",
                       "patternProperties", "additionalProperties")
  WALK_MAYBE_DEPENDENT(
      HTTPS_BASE "2020-12/vocab/unevaluated", "unevaluatedItems",
      TYPES(JSON::Type::Array), ApplicatorValueTraverseSomeItem,
      HTTPS_BASE "2020-12/vocab/applicator", "prefixItems", "items", "contains")
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentSchema",
       TYPES(JSON::Type::String), ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentMediaType",
       TYPES(JSON::Type::String), Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentEncoding",
       TYPES(JSON::Type::String), Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/format-assertion", "format",
       TYPES(JSON::Type::String), Assertion)

  // 2019-09
  WALK(HTTPS_BASE "2019-09/vocab/core", "$id", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$schema", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$ref", {}, Reference)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$defs", {}, LocationMembers)
  // JSON Schema still defines this for backwards-compatibility
  // See https://json-schema.org/draft/2019-09/schema
  WALK(HTTPS_BASE "2019-09/vocab/core", "definitions", {}, LocationMembers)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$comment", {}, Comment)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$anchor", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$vocabulary", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$recursiveRef", {}, Reference)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$recursiveAnchor", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "allOf", {},
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "anyOf", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "oneOf", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "if", {},
       ApplicatorValueInPlaceMaybe)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "then", {},
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "else", {},
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "not", {},
       ApplicatorValueInPlaceNegate)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/applicator", "properties",
                       TYPES(JSON::Type::Object),
                       ApplicatorMembersTraversePropertyStatic,
                       HTTPS_BASE "2019-09/vocab/validation", "required")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "patternProperties",
       TYPES(JSON::Type::Object), ApplicatorMembersTraversePropertyRegex)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties", "patternProperties")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "propertyNames",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseAnyPropertyKey)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "dependentSchemas",
       TYPES(JSON::Type::Object), ApplicatorMembersInPlaceSome)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "unevaluatedProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties", "patternProperties", "additionalProperties")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "unevaluatedItems",
       TYPES(JSON::Type::Array), ApplicatorValueTraverseSomeItem, "items",
       "additionalItems")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/applicator", "contains",
                       TYPES(JSON::Type::Array), ApplicatorValueTraverseAnyItem,
                       HTTPS_BASE "2019-09/vocab/validation", "minContains",
                       "maxContains")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "additionalItems",
       TYPES(JSON::Type::Array), ApplicatorValueTraverseSomeItem, "items")
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/validation", "type", {},
                       Assertion, HTTPS_BASE "2019-09/vocab/applicator",
                       "properties")
  WALK(HTTPS_BASE "2019-09/vocab/validation", "enum", {}, Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "const", {}, Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxLength",
       TYPES(JSON::Type::String), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minLength",
       TYPES(JSON::Type::String), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "pattern",
       TYPES(JSON::Type::String), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "exclusiveMaximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "multipleOf",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "type")
  WALK(HTTPS_BASE "2019-09/vocab/validation", "exclusiveMinimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "type")
  WALK(HTTPS_BASE "2019-09/vocab/validation", "required",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minProperties",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxProperties",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "dependentRequired",
       TYPES(JSON::Type::Object), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minItems",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxItems",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxContains",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minContains",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "uniqueItems",
       TYPES(JSON::Type::Array), Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "title", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "description", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "writeOnly", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "readOnly", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "examples", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "deprecated", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "default", {}, Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/format", "format", TYPES(JSON::Type::String),
       Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentSchema",
       TYPES(JSON::Type::String), ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentMediaType",
       TYPES(JSON::Type::String), Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentEncoding",
       TYPES(JSON::Type::String), Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "hrefSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "headerSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "submissionSchema", {},
       ApplicatorValueInPlaceOther)

#undef HTTPS_BASE

#define HTTP_BASE "http://json-schema.org/"
  // Draft7
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$schema", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$id", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$ref", {}, Reference)
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$comment", {}, Comment, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "type", {}, Assertion, "properties")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "enum", {}, Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "const", {}, Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "multipleOf", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "exclusiveMaximum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "exclusiveMinimum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "additionalItems", TYPES(JSON::Type::Array),
           ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "uniqueItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contains", TYPES(JSON::Type::Array), ApplicatorValueTraverseAnyItem,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxProperties", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minProperties", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "required", TYPES(JSON::Type::Object), Assertion, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic, "$ref", "required")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "patternProperties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "dependencies", TYPES(JSON::Type::Object),
           ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "propertyNames", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseAnyPropertyKey, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "if", {}, ApplicatorValueInPlaceMaybe, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "then", {}, ApplicatorValueInPlaceMaybe, "if")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "else", {}, ApplicatorValueInPlaceMaybe, "if")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "allOf", {}, ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "anyOf", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "oneOf", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "not", {}, ApplicatorValueInPlaceNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contentEncoding", TYPES(JSON::Type::String), Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contentMediaType", TYPES(JSON::Type::String), Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "definitions", {}, LocationMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "title", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "description", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "default", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "readOnly", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "writeOnly", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "examples", {}, Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "hrefSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "targetSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "headerSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "submissionSchema",
                       {}, ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-07/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-07/hyper-schema#")) &&
      keyword != "$ref") {
    return {sourcemeta::core::SchemaKeywordType::Unknown,
            std::nullopt,
            {"$ref"},
            {}};
  }

  // Draft6
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$schema", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$id", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$ref", {}, Reference)
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$comment", {}, Comment, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "type", {}, Assertion, "properties")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "enum", {}, Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "const", {}, Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "multipleOf", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "exclusiveMaximum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "exclusiveMinimum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "additionalItems", TYPES(JSON::Type::Array),
           ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "uniqueItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contains", TYPES(JSON::Type::Array), ApplicatorValueTraverseAnyItem,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxProperties", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minProperties", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "required", TYPES(JSON::Type::Object), Assertion, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic, "$ref", "required")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "patternProperties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "dependencies", TYPES(JSON::Type::Object),
           ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "propertyNames", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseAnyPropertyKey, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "allOf", {}, ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "anyOf", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "oneOf", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "not", {}, ApplicatorValueInPlaceNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contentEncoding", TYPES(JSON::Type::String), Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contentMediaType", TYPES(JSON::Type::String), Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "definitions", {}, LocationMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "title", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "description", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "default", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "readOnly", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "writeOnly", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "examples", {}, Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "hrefSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-06/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "targetSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-06/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "submissionSchema",
                       {}, ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-06/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-06/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-06/hyper-schema#")) &&
      keyword != "$ref") {
    return {sourcemeta::core::SchemaKeywordType::Unknown,
            std::nullopt,
            {"$ref"},
            {}};
  }

  // Draft4
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "$schema", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "id", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "$ref", {}, Reference)
  // These dependencies are only for the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "type", {}, Assertion, "properties")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "enum", {}, Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "multipleOf", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "exclusiveMaximum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "exclusiveMinimum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "additionalItems", TYPES(JSON::Type::Array),
           ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "uniqueItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maxProperties", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minProperties", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "required", TYPES(JSON::Type::Object), Assertion, "$ref")
  // These dependencies are only for the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic, "$ref", "required")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "patternProperties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "dependencies", TYPES(JSON::Type::Object),
           ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "allOf", {}, ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "anyOf", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "oneOf", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "not", {}, ApplicatorValueInPlaceNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "definitions", {}, LocationMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "title", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "description", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "default", {}, Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-04/hyper-schema#", "targetSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-04/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-04/hyper-schema#", "schema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-04/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-04/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-04/hyper-schema#")) &&
      keyword != "$ref") {
    return {sourcemeta::core::SchemaKeywordType::Unknown,
            std::nullopt,
            {"$ref"},
            {}};
  }

  // Draft3
  WALK(HTTP_BASE "draft-03/schema#", "id", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "$schema", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "$ref", {}, Reference)
  WALK(HTTP_BASE "draft-03/schema#", "extends", {},
       ApplicatorValueOrElementsInPlace, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "type", {}, ApplicatorElementsInPlaceSome,
       "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "disallow", {},
       ApplicatorElementsInPlaceSomeNegate, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "properties", TYPES(JSON::Type::Object),
       ApplicatorMembersTraversePropertyStatic, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "patternProperties",
       TYPES(JSON::Type::Object), ApplicatorMembersTraversePropertyRegex,
       "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties", "patternProperties")
  WALK(HTTP_BASE "draft-03/schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "additionalItems",
       TYPES(JSON::Type::Array), ApplicatorValueTraverseSomeItem, "items")
  WALK(HTTP_BASE "draft-03/schema#", "minItems", TYPES(JSON::Type::Array),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maxItems", TYPES(JSON::Type::Array),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "uniqueItems", TYPES(JSON::Type::Array),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "required", TYPES(JSON::Type::Object),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "dependencies", TYPES(JSON::Type::Object),
       ApplicatorMembersInPlaceSome, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "enum", {}, Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "pattern", TYPES(JSON::Type::String),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "minLength", TYPES(JSON::Type::String),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maxLength", TYPES(JSON::Type::String),
       Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "divisibleBy",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "minimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "exclusiveMinimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "exclusiveMaximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "format", TYPES(JSON::Type::String), Other,
       "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "description", {}, Comment, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "title", {}, Comment, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "default", {}, Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-03/hyper-schema#", "targetSchema", {},
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-03/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if (vocabularies.contains(HTTP_BASE "draft-03/schema#") &&
      keyword != "$ref") {
    return {sourcemeta::core::SchemaKeywordType::Unknown,
            std::nullopt,
            {"$ref"},
            {}};
  }

  // Draft2
  WALK(HTTP_BASE "draft-02/schema#", "$schema", {}, Other)
  WALK(HTTP_BASE "draft-02/schema#", "id", {}, Other)
  WALK(HTTP_BASE "draft-02/schema#", "$ref", {}, Reference)
  WALK(HTTP_BASE "draft-02/schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-02/schema#", "properties", TYPES(JSON::Type::Object),
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-02/schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties")
  WALK(HTTP_BASE "draft-02/schema#", "type", {}, ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-02/schema#", "enum", {}, Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maximumCanEqual",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minimumCanEqual",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maxLength", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minLength", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "pattern", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maxItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "uniqueItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "requires", TYPES(JSON::Type::Object),
       ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-02/schema#", "format", TYPES(JSON::Type::String), Other)
  WALK(HTTP_BASE "draft-02/schema#", "title", {}, Comment)
  WALK(HTTP_BASE "draft-02/schema#", "description", {}, Comment)
  WALK(HTTP_BASE "draft-02/schema#", "default", {}, Comment)
  WALK(HTTP_BASE "draft-02/schema#", "divisibleBy",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "disallow", {}, Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "extends", {},
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-02/schema#", "contentEncoding",
       TYPES(JSON::Type::String), Comment)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "requires",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "type", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "properties",
       TYPES(JSON::Type::Object), ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "extends", {},
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties")

  // Draft1
  WALK(HTTP_BASE "draft-01/schema#", "$schema", {}, Other)
  WALK(HTTP_BASE "draft-01/schema#", "id", {}, Other)
  WALK(HTTP_BASE "draft-01/schema#", "$ref", {}, Reference)
  WALK(HTTP_BASE "draft-01/schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-01/schema#", "properties", TYPES(JSON::Type::Object),
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-01/schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties")
  WALK(HTTP_BASE "draft-01/schema#", "type", {}, ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-01/schema#", "enum", {}, Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maximumCanEqual",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minimumCanEqual",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maxLength", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minLength", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "pattern", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maxItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "requires", TYPES(JSON::Type::Object),
       ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-01/schema#", "format", TYPES(JSON::Type::String), Other)
  WALK(HTTP_BASE "draft-01/schema#", "title", {}, Comment)
  WALK(HTTP_BASE "draft-01/schema#", "description", {}, Comment)
  WALK(HTTP_BASE "draft-01/schema#", "default", {}, Comment)
  WALK(HTTP_BASE "draft-01/schema#", "disallow", {}, Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "extends", {},
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-01/schema#", "contentEncoding",
       TYPES(JSON::Type::String), Comment)
  WALK(HTTP_BASE "draft-01/schema#", "optional", TYPES(JSON::Type::Object),
       Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maxDecimal", TYPES(JSON::Type::Real),
       Assertion)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "type", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "properties",
       TYPES(JSON::Type::Object), ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "extends", {},
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "requires",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties")

  // Draft0
  WALK(HTTP_BASE "draft-00/schema#", "$schema", {}, Other)
  WALK(HTTP_BASE "draft-00/schema#", "id", {}, Other)
  WALK(HTTP_BASE "draft-00/schema#", "$ref", {}, Reference)
  WALK(HTTP_BASE "draft-00/schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-00/schema#", "properties", TYPES(JSON::Type::Object),
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-00/schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties")
  WALK(HTTP_BASE "draft-00/schema#", "type", {}, ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-00/schema#", "enum", {}, Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maximum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minimum",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maximumCanEqual",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minimumCanEqual",
       TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maxLength", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minLength", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "pattern", TYPES(JSON::Type::String),
       Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maxItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minItems", TYPES(JSON::Type::Array),
       Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "requires", TYPES(JSON::Type::Object),
       ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-00/schema#", "format", TYPES(JSON::Type::String), Other)
  WALK(HTTP_BASE "draft-00/schema#", "title", {}, Comment)
  WALK(HTTP_BASE "draft-00/schema#", "description", {}, Comment)
  WALK(HTTP_BASE "draft-00/schema#", "default", {}, Comment)
  WALK(HTTP_BASE "draft-00/schema#", "disallow", {}, Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "extends", {},
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-00/schema#", "contentEncoding",
       TYPES(JSON::Type::String), Comment)
  WALK(HTTP_BASE "draft-00/schema#", "optional", TYPES(JSON::Type::Object),
       Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maxDecimal", TYPES(JSON::Type::Real),
       Assertion)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "type", {},
       ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "items", TYPES(JSON::Type::Array),
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "properties",
       TYPES(JSON::Type::Object), ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "extends", {},
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "requires",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "additionalProperties",
       TYPES(JSON::Type::Object), ApplicatorValueTraverseSomeProperty,
       "properties")
#undef HTTP_BASE
#undef WALK
#undef WALK_ANY
#undef WALK_MAYBE_DEPENDENT
  return {sourcemeta::core::SchemaKeywordType::Unknown, std::nullopt, {}, {}};
}
