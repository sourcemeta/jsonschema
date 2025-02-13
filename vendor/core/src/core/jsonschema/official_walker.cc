#include <sourcemeta/core/jsonschema.h>

auto sourcemeta::core::schema_official_walker(
    std::string_view keyword, const std::map<std::string, bool> &vocabularies)
    -> sourcemeta::core::SchemaWalkerResult {
#define WALK(vocabulary, _keyword, strategy, ...)                              \
  if (vocabularies.contains(vocabulary) && keyword == _keyword)                \
    return {sourcemeta::core::SchemaKeywordType::strategy,                     \
            vocabulary,                                                        \
            {__VA_ARGS__}};

#define WALK_ANY(vocabulary_1, vocabulary_2, _keyword, strategy, ...)          \
  WALK(vocabulary_1, _keyword, strategy, __VA_ARGS__)                          \
  WALK(vocabulary_2, _keyword, strategy, __VA_ARGS__)

#define WALK_MAYBE_DEPENDENT(vocabulary, _keyword, strategy,                   \
                             dependent_vocabulary, ...)                        \
  if (vocabularies.contains(dependent_vocabulary)) {                           \
    WALK(vocabulary, _keyword, strategy, __VA_ARGS__)                          \
  } else {                                                                     \
    WALK(vocabulary, _keyword, strategy)                                       \
  }

#define HTTPS_BASE "https://json-schema.org/draft/"
  // 2020-12
  WALK(HTTPS_BASE "2020-12/vocab/core", "$id", Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$schema", Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$ref", Reference)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$defs", LocationMembers)
  // JSON Schema still defines this for backwards-compatibility
  // See https://json-schema.org/draft/2020-12/schema
  WALK(HTTPS_BASE "2020-12/vocab/core", "definitions", LocationMembers)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$comment", Comment)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$anchor", Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$vocabulary", Other)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$dynamicRef", Reference)
  WALK(HTTPS_BASE "2020-12/vocab/core", "$dynamicAnchor", Other)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "oneOf",
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "anyOf",
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "allOf",
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "if", ApplicatorValueInPlaceMaybe)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "then",
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "else",
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "not",
       ApplicatorValueInPlaceNegate)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/applicator", "properties",
                       ApplicatorMembersTraversePropertyStatic,
                       HTTPS_BASE "2020-12/vocab/validation", "required")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "patternProperties",
       ApplicatorMembersTraversePropertyRegex)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "propertyNames",
       ApplicatorValueTraverseAnyPropertyKey)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "dependentSchemas",
       ApplicatorMembersInPlaceSome)
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/applicator", "contains",
                       ApplicatorValueTraverseAnyItem,
                       HTTPS_BASE "2020-12/vocab/validation", "minContains",
                       "maxContains")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "items",
       ApplicatorValueTraverseSomeItem, "prefixItems")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "prefixItems",
       ApplicatorElementsTraverseItem)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/validation", "type", Assertion,
                       HTTPS_BASE "2020-12/vocab/applicator", "properties")
  WALK(HTTPS_BASE "2020-12/vocab/validation", "enum", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "const", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxLength", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minLength", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "pattern", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "exclusiveMinimum", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "multipleOf", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maximum", Assertion, "type")
  WALK(HTTPS_BASE "2020-12/vocab/validation", "exclusiveMaximum", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minimum", Assertion, "type")
  WALK(HTTPS_BASE "2020-12/vocab/validation", "dependentRequired", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minProperties", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxProperties", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "required", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxItems", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minItems", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "uniqueItems", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "minContains", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/validation", "maxContains", Assertion)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "title", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "description", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "writeOnly", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "readOnly", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "examples", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "default", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/meta-data", "deprecated", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/format-annotation", "format", Annotation)
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/unevaluated",
                       "unevaluatedProperties",
                       ApplicatorValueTraverseSomeProperty,
                       HTTPS_BASE "2020-12/vocab/applicator", "properties",
                       "patternProperties", "additionalProperties")
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/unevaluated",
                       "unevaluatedItems", ApplicatorValueTraverseSomeItem,
                       HTTPS_BASE "2020-12/vocab/applicator", "prefixItems",
                       "items", "contains")
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentSchema",
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentMediaType", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentEncoding", Annotation)
  WALK(HTTPS_BASE "2020-12/vocab/format-assertion", "format", Assertion)

  // 2019-09
  WALK(HTTPS_BASE "2019-09/vocab/core", "$id", Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$schema", Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$ref", Reference)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$defs", LocationMembers)
  // JSON Schema still defines this for backwards-compatibility
  // See https://json-schema.org/draft/2019-09/schema
  WALK(HTTPS_BASE "2019-09/vocab/core", "definitions", LocationMembers)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$comment", Comment)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$anchor", Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$vocabulary", Other)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$recursiveRef", Reference)
  WALK(HTTPS_BASE "2019-09/vocab/core", "$recursiveAnchor", Other)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "allOf",
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "anyOf",
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "oneOf",
       ApplicatorElementsInPlaceSome)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "if", ApplicatorValueInPlaceMaybe)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "then",
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "else",
       ApplicatorValueInPlaceMaybe, "if")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "not",
       ApplicatorValueInPlaceNegate)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/applicator", "properties",
                       ApplicatorMembersTraversePropertyStatic,
                       HTTPS_BASE "2019-09/vocab/validation", "required")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "patternProperties",
       ApplicatorMembersTraversePropertyRegex)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "propertyNames",
       ApplicatorValueTraverseAnyPropertyKey)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "dependentSchemas",
       ApplicatorMembersInPlaceSome)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "unevaluatedProperties",
       ApplicatorValueTraverseSomeProperty, "properties", "patternProperties",
       "additionalProperties")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "unevaluatedItems",
       ApplicatorValueTraverseSomeItem, "items", "additionalItems")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/applicator", "contains",
                       ApplicatorValueTraverseAnyItem,
                       HTTPS_BASE "2019-09/vocab/validation", "minContains",
                       "maxContains")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "additionalItems",
       ApplicatorValueTraverseSomeItem, "items")
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/validation", "type", Assertion,
                       HTTPS_BASE "2019-09/vocab/applicator", "properties")
  WALK(HTTPS_BASE "2019-09/vocab/validation", "enum", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "const", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxLength", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minLength", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "pattern", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "exclusiveMaximum", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "multipleOf", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minimum", Assertion, "type")
  WALK(HTTPS_BASE "2019-09/vocab/validation", "exclusiveMinimum", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maximum", Assertion, "type")
  WALK(HTTPS_BASE "2019-09/vocab/validation", "required", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minProperties", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxProperties", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "dependentRequired", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minItems", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxItems", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "maxContains", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "minContains", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/validation", "uniqueItems", Assertion)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "title", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "description", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "writeOnly", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "readOnly", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "examples", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "deprecated", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/meta-data", "default", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/format", "format", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentSchema",
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentMediaType", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentEncoding", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "hrefSchema",
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "targetSchema",
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "headerSchema",
       ApplicatorValueInPlaceOther)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "submissionSchema",
       ApplicatorValueInPlaceOther)

#undef HTTPS_BASE

#define HTTP_BASE "http://json-schema.org/"
  // Draft7
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$schema", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$id", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$ref", Reference)
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "$comment", Comment, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "type", Assertion, "properties")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "enum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "const", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "multipleOf", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maximum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "exclusiveMaximum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minimum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "exclusiveMinimum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxLength", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minLength", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "pattern", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "items", ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "additionalItems", ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "uniqueItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contains", ApplicatorValueTraverseAnyItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "required", Assertion, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "properties", ApplicatorMembersTraversePropertyStatic, "$ref",
           "required")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "patternProperties", ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "additionalProperties", ApplicatorValueTraverseSomeProperty,
           "properties", "patternProperties")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "dependencies", ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "propertyNames", ApplicatorValueTraverseAnyPropertyKey, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "if", ApplicatorValueInPlaceMaybe, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "then", ApplicatorValueInPlaceMaybe, "if")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "else", ApplicatorValueInPlaceMaybe, "if")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "allOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "anyOf", ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "oneOf", ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "not", ApplicatorValueInPlaceNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "format", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contentEncoding", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contentMediaType", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "definitions", LocationMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "title", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "description", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "default", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "readOnly", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "writeOnly", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "examples", Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "hrefSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "targetSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "headerSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "submissionSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-07/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-07/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-07/hyper-schema#")) &&
      keyword != "$ref") {
    return {
        sourcemeta::core::SchemaKeywordType::Unknown, std::nullopt, {"$ref"}};
  }

  // Draft6
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$schema", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$id", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$ref", Reference)
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "$comment", Comment, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "type", Assertion, "properties")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "enum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "const", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "multipleOf", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maximum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "exclusiveMaximum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minimum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "exclusiveMinimum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxLength", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minLength", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "pattern", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "items", ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "additionalItems", ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "uniqueItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contains", ApplicatorValueTraverseAnyItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "required", Assertion, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "properties", ApplicatorMembersTraversePropertyStatic, "$ref",
           "required")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "patternProperties", ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "additionalProperties", ApplicatorValueTraverseSomeProperty,
           "properties", "patternProperties")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "dependencies", ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "propertyNames", ApplicatorValueTraverseAnyPropertyKey, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "allOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "anyOf", ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "oneOf", ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "not", ApplicatorValueInPlaceNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "format", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contentEncoding", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contentMediaType", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "definitions", LocationMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "title", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "description", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "default", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "readOnly", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "writeOnly", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "examples", Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "hrefSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-06/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "targetSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-06/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "submissionSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-06/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-06/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-06/hyper-schema#")) &&
      keyword != "$ref") {
    return {
        sourcemeta::core::SchemaKeywordType::Unknown, std::nullopt, {"$ref"}};
  }

  // Draft4
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "$schema", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "id", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "$ref", Reference)
  // These dependencies are only for the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "type", Assertion, "properties")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "enum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "multipleOf", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maximum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "exclusiveMaximum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minimum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "exclusiveMinimum", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maxLength", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minLength", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "pattern", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "items", ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "additionalItems", ApplicatorValueTraverseSomeItem, "items")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maxItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "uniqueItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "maxProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "minProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "required", Assertion, "$ref")
  // These dependencies are only for the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "properties", ApplicatorMembersTraversePropertyStatic, "$ref",
           "required")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "patternProperties", ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "additionalProperties", ApplicatorValueTraverseSomeProperty,
           "properties", "patternProperties")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "dependencies", ApplicatorMembersInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "allOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "anyOf", ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "oneOf", ApplicatorElementsInPlaceSome, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "not", ApplicatorValueInPlaceNegate, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "format", Other, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "definitions", LocationMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "title", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "description", Comment, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "default", Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-04/hyper-schema#", "targetSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-04/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-04/hyper-schema#", "schema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-04/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-04/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-04/hyper-schema#")) &&
      keyword != "$ref") {
    return {
        sourcemeta::core::SchemaKeywordType::Unknown, std::nullopt, {"$ref"}};
  }

  // Draft3
  WALK(HTTP_BASE "draft-03/schema#", "id", Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "$schema", Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-03/schema#", "extends",
       ApplicatorValueOrElementsInPlace, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "type", ApplicatorElementsInPlaceSome,
       "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "disallow",
       ApplicatorElementsInPlaceSomeNegate, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "properties",
       ApplicatorMembersTraversePropertyStatic, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "patternProperties",
       ApplicatorMembersTraversePropertyRegex, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  WALK(HTTP_BASE "draft-03/schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "additionalItems",
       ApplicatorValueTraverseSomeItem, "items")
  WALK(HTTP_BASE "draft-03/schema#", "minItems", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maxItems", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "uniqueItems", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "required", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "dependencies",
       ApplicatorMembersInPlaceSome, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "enum", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "pattern", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "minLength", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maxLength", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "divisibleBy", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "minimum", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "exclusiveMinimum", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maximum", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "exclusiveMaximum", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "format", Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "description", Comment, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "title", Comment, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "default", Comment, "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-03/hyper-schema#", "targetSchema",
                       ApplicatorValueInPlaceOther,
                       HTTP_BASE "draft-03/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if (vocabularies.contains(HTTP_BASE "draft-03/schema#") &&
      keyword != "$ref") {
    return {
        sourcemeta::core::SchemaKeywordType::Unknown, std::nullopt, {"$ref"}};
  }

  // Draft2
  WALK(HTTP_BASE "draft-02/schema#", "$schema", Other)
  WALK(HTTP_BASE "draft-02/schema#", "id", Other)
  WALK(HTTP_BASE "draft-02/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-02/schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-02/schema#", "properties",
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-02/schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties")
  WALK(HTTP_BASE "draft-02/schema#", "type", ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-02/schema#", "enum", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maximum", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minimum", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maximumCanEqual", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minimumCanEqual", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maxLength", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minLength", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "pattern", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "maxItems", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "minItems", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "uniqueItems", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "requires", ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-02/schema#", "format", Other)
  WALK(HTTP_BASE "draft-02/schema#", "title", Comment)
  WALK(HTTP_BASE "draft-02/schema#", "description", Comment)
  WALK(HTTP_BASE "draft-02/schema#", "default", Comment)
  WALK(HTTP_BASE "draft-02/schema#", "divisibleBy", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "disallow", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-02/schema#", "contentEncoding", Comment)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "requires",
       ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "targetSchema",
       ApplicatorValueInPlaceOther)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "type",
       ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "properties",
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties")

  // Draft1
  WALK(HTTP_BASE "draft-01/schema#", "$schema", Other)
  WALK(HTTP_BASE "draft-01/schema#", "id", Other)
  WALK(HTTP_BASE "draft-01/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-01/schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-01/schema#", "properties",
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-01/schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties")
  WALK(HTTP_BASE "draft-01/schema#", "type", ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-01/schema#", "enum", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maximum", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minimum", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maximumCanEqual", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minimumCanEqual", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maxLength", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minLength", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "pattern", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maxItems", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "minItems", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "requires", ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-01/schema#", "format", Other)
  WALK(HTTP_BASE "draft-01/schema#", "title", Comment)
  WALK(HTTP_BASE "draft-01/schema#", "description", Comment)
  WALK(HTTP_BASE "draft-01/schema#", "default", Comment)
  WALK(HTTP_BASE "draft-01/schema#", "disallow", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-01/schema#", "contentEncoding", Comment)
  WALK(HTTP_BASE "draft-01/schema#", "optional", Assertion)
  WALK(HTTP_BASE "draft-01/schema#", "maxDecimal", Assertion)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "type",
       ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "properties",
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "requires",
       ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties")

  // Draft0
  WALK(HTTP_BASE "draft-00/schema#", "$schema", Other)
  WALK(HTTP_BASE "draft-00/schema#", "id", Other)
  WALK(HTTP_BASE "draft-00/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-00/schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-00/schema#", "properties",
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-00/schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties")
  WALK(HTTP_BASE "draft-00/schema#", "type", ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-00/schema#", "enum", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maximum", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minimum", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maximumCanEqual", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minimumCanEqual", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maxLength", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minLength", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "pattern", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maxItems", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "minItems", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "requires", ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-00/schema#", "format", Other)
  WALK(HTTP_BASE "draft-00/schema#", "title", Comment)
  WALK(HTTP_BASE "draft-00/schema#", "description", Comment)
  WALK(HTTP_BASE "draft-00/schema#", "default", Comment)
  WALK(HTTP_BASE "draft-00/schema#", "disallow", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-00/schema#", "contentEncoding", Comment)
  WALK(HTTP_BASE "draft-00/schema#", "optional", Assertion)
  WALK(HTTP_BASE "draft-00/schema#", "maxDecimal", Assertion)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "type",
       ApplicatorElementsInPlaceSome)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "items",
       ApplicatorValueOrElementsTraverseAnyItemOrItem)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "properties",
       ApplicatorMembersTraversePropertyStatic)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "requires",
       ApplicatorValueTraverseParent)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "additionalProperties",
       ApplicatorValueTraverseSomeProperty, "properties")
#undef HTTP_BASE
#undef WALK
#undef WALK_ANY
#undef WALK_MAYBE_DEPENDENT
  return {sourcemeta::core::SchemaKeywordType::Unknown, std::nullopt, {}};
}
