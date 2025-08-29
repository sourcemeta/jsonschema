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
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "base", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "links", {},
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "hrefSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "headerSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "submissionSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "anchor", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "anchorPointer", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "href", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "rel", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "submissionMediaType", {},
       Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "targetHints", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "targetMediaType", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "templatePointers", {}, Other)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "templateRequired", {}, Other)

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
  WALK(HTTP_BASE "draft-07/hyper-schema#", "base", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-07/hyper-schema#", "links", {},
       ApplicatorElementsInPlace, "$ref")
  // Keywords from the Link Description Object are not affected by `$ref`, as
  // `$ref` is not permitted there
  WALK(HTTP_BASE "draft-07/hyper-schema#", "hrefSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "headerSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "submissionSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "anchor", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "anchorPointer", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "submissionMediaType", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "targetHints", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "targetMediaType", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "templatePointers", {}, Other)
  WALK(HTTP_BASE "draft-07/hyper-schema#", "templateRequired", {}, Other)

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-07/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-07/hyper-schema#")) &&
      keyword != "$ref") {
    return {.type = sourcemeta::core::SchemaKeywordType::Unknown,
            .vocabulary = std::nullopt,
            .dependencies = {"$ref"},
            .instances = {}};
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
  WALK(HTTP_BASE "draft-06/hyper-schema#", "base", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-06/hyper-schema#", "links", {},
       ApplicatorElementsInPlace, "$ref")
  WALK(HTTP_BASE "draft-06/hyper-schema#", "media", {}, Other, "$ref")
  // Keywords from the Link Description Object are not affected by `$ref`, as
  // `$ref` is not permitted there
  WALK(HTTP_BASE "draft-06/hyper-schema#", "hrefSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-06/hyper-schema#", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-06/hyper-schema#", "submissionSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-06/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-06/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-06/hyper-schema#", "submissionEncType", {}, Other)
  WALK(HTTP_BASE "draft-06/hyper-schema#", "mediaType", {}, Other)

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-06/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-06/hyper-schema#")) &&
      keyword != "$ref") {
    return {.type = sourcemeta::core::SchemaKeywordType::Unknown,
            .vocabulary = std::nullopt,
            .dependencies = {"$ref"},
            .instances = {}};
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
  WALK(HTTP_BASE "draft-04/hyper-schema#", "fragmentResolution", {}, Other,
       "$ref")
  WALK(HTTP_BASE "draft-04/hyper-schema#", "links", {},
       ApplicatorElementsInPlace, "$ref")
  WALK(HTTP_BASE "draft-04/hyper-schema#", "media", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-04/hyper-schema#", "pathStart", {}, Other, "$ref")
  // Keywords from the Link Description Object are not affected by `$ref`, as
  // `$ref` is not permitted there
  WALK(HTTP_BASE "draft-04/hyper-schema#", "encType", {}, Other)
  WALK(HTTP_BASE "draft-04/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-04/hyper-schema#", "mediaType", {}, Other)
  WALK(HTTP_BASE "draft-04/hyper-schema#", "method", {}, Other)
  WALK(HTTP_BASE "draft-04/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-04/hyper-schema#", "schema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-04/hyper-schema#", "targetSchema", {},
       ApplicatorValueInPlaceOther)

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-04/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-04/hyper-schema#")) &&
      keyword != "$ref") {
    return {.type = sourcemeta::core::SchemaKeywordType::Unknown,
            .vocabulary = std::nullopt,
            .dependencies = {"$ref"},
            .instances = {}};
  }

  // Draft3
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "id", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "$schema", {}, Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "$ref", {}, Reference)
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "extends", {}, ApplicatorValueOrElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "type", {}, ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "disallow", {}, ApplicatorElementsInPlaceSomeNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "patternProperties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "additionalItems", TYPES(JSON::Type::Array),
           ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "uniqueItems", TYPES(JSON::Type::Array), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "required", TYPES(JSON::Type::Object), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "dependencies", TYPES(JSON::Type::Object),
           ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "enum", {}, Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "divisibleBy", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "exclusiveMinimum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion,
           "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "exclusiveMaximum", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "description", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "title", {}, Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-03/schema#", HTTP_BASE "draft-03/hyper-schema#",
           "default", {}, Comment, "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "fragmentResolution", {}, Other,
       "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "root", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "readonly", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "contentEncoding", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "pathStart", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "mediaType", {}, Other, "$ref")
  WALK(HTTP_BASE "draft-03/hyper-schema#", "links", {},
       ApplicatorElementsInPlace, "$ref")
  // Keywords from the Link Description Object are not affected by `$ref`, as
  // `$ref` is not permitted there
  WALK(HTTP_BASE "draft-03/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-03/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-03/hyper-schema#", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-03/hyper-schema#", "method", {}, Other)
  WALK(HTTP_BASE "draft-03/hyper-schema#", "enctype", {}, Other)

  // $ref also takes precedence over any unknown keyword
  if (vocabularies.contains(HTTP_BASE "draft-03/schema#") &&
      keyword != "$ref") {
    return {.type = sourcemeta::core::SchemaKeywordType::Unknown,
            .vocabulary = std::nullopt,
            .dependencies = {"$ref"},
            .instances = {}};
  }

  // Draft2
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "$schema", {}, Other)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "id", {}, Other)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "$ref", {}, Reference)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties")
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "type", {}, ApplicatorElementsInPlaceSome)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "enum", {}, Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "maximumCanEqual", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "minimumCanEqual", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "uniqueItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "requires", TYPES(JSON::Type::Object), ApplicatorValueTraverseParent)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "title", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "description", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "default", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "divisibleBy", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "disallow", {}, Assertion)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "extends", {}, ApplicatorValueOrElementsInPlace)
  WALK_ANY(HTTP_BASE "draft-02/schema#", HTTP_BASE "draft-02/hyper-schema#",
           "contentEncoding", TYPES(JSON::Type::String), Comment)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "fragmentResolution", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "root", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "readonly", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "pathStart", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "mediaType", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "alternate", {},
       ApplicatorElementsInPlace)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "links", {},
       ApplicatorElementsInPlace)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "targetSchema", {},
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "method", {}, Other)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "enctype", {}, Other)

  // Draft1
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "$schema", {}, Other)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "id", {}, Other)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "$ref", {}, Reference)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties")
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "type", {}, ApplicatorElementsInPlaceSome)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "enum", {}, Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "maximumCanEqual", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "minimumCanEqual", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "requires", TYPES(JSON::Type::Object), ApplicatorValueTraverseParent)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "title", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "description", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "default", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "disallow", {}, Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "extends", {}, ApplicatorValueOrElementsInPlace)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "contentEncoding", TYPES(JSON::Type::String), Comment)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "optional", TYPES(JSON::Type::Object), Assertion)
  WALK_ANY(HTTP_BASE "draft-01/schema#", HTTP_BASE "draft-01/hyper-schema#",
           "maxDecimal", TYPES(JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "fragmentResolution", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "root", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "readonly", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "pathStart", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "mediaType", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "alternate", {},
       ApplicatorElementsInPlace)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "links", {},
       ApplicatorElementsInPlace)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "method", {}, Other)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "enctype", {}, Other)

  // Draft0
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "$schema", {}, Other)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "id", {}, Other)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "$ref", {}, Reference)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "items", TYPES(JSON::Type::Array),
           ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "properties", TYPES(JSON::Type::Object),
           ApplicatorMembersTraversePropertyStatic)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "additionalProperties", TYPES(JSON::Type::Object),
           ApplicatorValueTraverseSomeProperty, "properties")
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "type", {}, ApplicatorElementsInPlaceSome)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "enum", {}, Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "maximum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "minimum", TYPES(JSON::Type::Integer, JSON::Type::Real), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "maximumCanEqual", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "minimumCanEqual", TYPES(JSON::Type::Integer, JSON::Type::Real),
           Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "maxLength", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "minLength", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "pattern", TYPES(JSON::Type::String), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "maxItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "minItems", TYPES(JSON::Type::Array), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "requires", TYPES(JSON::Type::Object), ApplicatorValueTraverseParent)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "format", TYPES(JSON::Type::String), Other)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "title", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "description", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "default", {}, Comment)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "disallow", {}, Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "extends", {}, ApplicatorValueOrElementsInPlace)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "contentEncoding", TYPES(JSON::Type::String), Comment)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "optional", TYPES(JSON::Type::Object), Assertion)
  WALK_ANY(HTTP_BASE "draft-00/schema#", HTTP_BASE "draft-00/hyper-schema#",
           "maxDecimal", TYPES(JSON::Type::Real), Assertion)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "fragmentResolution", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "root", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "readonly", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "pathStart", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "mediaType", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "alternate", {},
       ApplicatorElementsInPlace)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "links", {},
       ApplicatorElementsInPlace)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "href", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "rel", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "method", {}, Other)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "enctype", {}, Other)
#undef HTTP_BASE
#undef WALK
#undef WALK_ANY
#undef WALK_MAYBE_DEPENDENT
  return {.type = sourcemeta::core::SchemaKeywordType::Unknown,
          .vocabulary = std::nullopt,
          .dependencies = {},
          .instances = {}};
}
