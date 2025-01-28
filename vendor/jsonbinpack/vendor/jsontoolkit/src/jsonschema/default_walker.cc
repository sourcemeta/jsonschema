#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

auto sourcemeta::jsontoolkit::default_schema_walker(
    std::string_view keyword, const std::map<std::string, bool> &vocabularies)
    -> sourcemeta::jsontoolkit::SchemaWalkerResult {
#define WALK(vocabulary, _keyword, strategy, ...)                              \
  if (vocabularies.contains(vocabulary) && keyword == _keyword)                \
    return {sourcemeta::jsontoolkit::KeywordType::strategy,                    \
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
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "anyOf",
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "allOf", ApplicatorElementsInline)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "if", ApplicatorValueInPlace)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "then", ApplicatorValueInPlace,
       "if")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "else", ApplicatorValueInPlace,
       "if")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "not", ApplicatorValueOther)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/applicator", "properties",
                       ApplicatorMembers, HTTPS_BASE "2020-12/vocab/validation",
                       "required")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "additionalProperties",
       ApplicatorValue, "properties", "patternProperties")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "patternProperties",
       ApplicatorMembers)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "propertyNames",
       ApplicatorValueInPlace)
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "dependentSchemas",
       ApplicatorMembersInPlace)
  WALK_MAYBE_DEPENDENT(
      HTTPS_BASE "2020-12/vocab/applicator", "contains", ApplicatorValueInPlace,
      HTTPS_BASE "2020-12/vocab/validation", "minContains", "maxContains")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "items", ApplicatorValue,
       "prefixItems")
  WALK(HTTPS_BASE "2020-12/vocab/applicator", "prefixItems", ApplicatorElements)
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
                       "unevaluatedProperties", ApplicatorValue,
                       HTTPS_BASE "2020-12/vocab/applicator", "properties",
                       "patternProperties", "additionalProperties")
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2020-12/vocab/unevaluated",
                       "unevaluatedItems", ApplicatorValue,
                       HTTPS_BASE "2020-12/vocab/applicator", "prefixItems",
                       "items", "contains")
  WALK(HTTPS_BASE "2020-12/vocab/content", "contentSchema",
       ApplicatorValueOther)
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
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "allOf", ApplicatorElementsInline)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "anyOf",
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "oneOf",
       ApplicatorElementsInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "if", ApplicatorValueInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "then", ApplicatorValueInPlace,
       "if")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "else", ApplicatorValueInPlace,
       "if")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "not", ApplicatorValueOther)
  // For the purpose of compiler optimizations
  WALK_MAYBE_DEPENDENT(HTTPS_BASE "2019-09/vocab/applicator", "properties",
                       ApplicatorMembers, HTTPS_BASE "2019-09/vocab/validation",
                       "required")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "patternProperties",
       ApplicatorMembers)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "additionalProperties",
       ApplicatorValue, "properties", "patternProperties")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "propertyNames",
       ApplicatorValueInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "dependentSchemas",
       ApplicatorMembersInPlace)
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "unevaluatedProperties",
       ApplicatorValue, "properties", "patternProperties",
       "additionalProperties")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "unevaluatedItems",
       ApplicatorValue, "items", "additionalItems")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "items",
       ApplicatorValueOrElements)
  WALK_MAYBE_DEPENDENT(
      HTTPS_BASE "2019-09/vocab/applicator", "contains", ApplicatorValueInPlace,
      HTTPS_BASE "2019-09/vocab/validation", "minContains", "maxContains")
  WALK(HTTPS_BASE "2019-09/vocab/applicator", "additionalItems",
       ApplicatorValue, "items")
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
       ApplicatorValueOther)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentMediaType", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/content", "contentEncoding", Annotation)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "hrefSchema", ApplicatorValue)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "targetSchema", ApplicatorValue)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "headerSchema", ApplicatorValue)
  WALK(HTTPS_BASE "2019-09/vocab/hyper-schema", "submissionSchema",
       ApplicatorValue)

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
           "items", ApplicatorValueOrElements, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "additionalItems", ApplicatorValue, "items")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "uniqueItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "contains", ApplicatorValueInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "maxProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "minProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "required", Assertion, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "properties", ApplicatorMembers, "$ref", "required")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "patternProperties", ApplicatorMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "additionalProperties", ApplicatorValue, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "dependencies", ApplicatorMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "propertyNames", ApplicatorValueInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "if", ApplicatorValueInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "then", ApplicatorValueInPlace, "if")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "else", ApplicatorValueInPlace, "if")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "allOf", ApplicatorElementsInline, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "anyOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "oneOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-07/schema#", HTTP_BASE "draft-07/hyper-schema#",
           "not", ApplicatorValueOther, "$ref")
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
                       ApplicatorValue, HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "targetSchema",
                       ApplicatorValue, HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "headerSchema",
                       ApplicatorValue, HTTP_BASE "draft-07/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-07/hyper-schema#", "submissionSchema",
                       ApplicatorValue, HTTP_BASE "draft-07/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-07/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-07/hyper-schema#")) &&
      keyword != "$ref") {
    return {
        sourcemeta::jsontoolkit::KeywordType::Unknown, std::nullopt, {"$ref"}};
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
           "items", ApplicatorValueOrElements, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "additionalItems", ApplicatorValue, "items")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "uniqueItems", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "contains", ApplicatorValueInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "maxProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "minProperties", Assertion, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "required", Assertion, "$ref")
  // For the purpose of compiler optimizations
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "properties", ApplicatorMembers, "$ref", "required")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "patternProperties", ApplicatorMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "additionalProperties", ApplicatorValue, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "dependencies", ApplicatorMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "propertyNames", ApplicatorValueInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "allOf", ApplicatorElementsInline, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "anyOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "oneOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-06/schema#", HTTP_BASE "draft-06/hyper-schema#",
           "not", ApplicatorValueOther, "$ref")
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
                       ApplicatorValue, HTTP_BASE "draft-06/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "targetSchema",
                       ApplicatorValue, HTTP_BASE "draft-06/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-06/hyper-schema#", "submissionSchema",
                       ApplicatorValue, HTTP_BASE "draft-06/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-06/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-06/hyper-schema#")) &&
      keyword != "$ref") {
    return {
        sourcemeta::jsontoolkit::KeywordType::Unknown, std::nullopt, {"$ref"}};
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
           "items", ApplicatorValueOrElements, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "additionalItems", ApplicatorValue, "items")
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
           "properties", ApplicatorMembers, "$ref", "required")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "patternProperties", ApplicatorMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "additionalProperties", ApplicatorValue, "properties",
           "patternProperties")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "dependencies", ApplicatorMembers, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "allOf", ApplicatorElementsInline, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "anyOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "oneOf", ApplicatorElementsInPlace, "$ref")
  WALK_ANY(HTTP_BASE "draft-04/schema#", HTTP_BASE "draft-04/hyper-schema#",
           "not", ApplicatorValueOther, "$ref")
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
                       ApplicatorValue, HTTP_BASE "draft-04/schema#", "$ref")
  WALK_MAYBE_DEPENDENT(HTTP_BASE "draft-04/hyper-schema#", "schema",
                       ApplicatorValue, HTTP_BASE "draft-04/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if ((vocabularies.contains(HTTP_BASE "draft-04/schema#") ||
       vocabularies.contains(HTTP_BASE "draft-04/hyper-schema#")) &&
      keyword != "$ref") {
    return {
        sourcemeta::jsontoolkit::KeywordType::Unknown, std::nullopt, {"$ref"}};
  }

  // Draft3
  WALK(HTTP_BASE "draft-03/schema#", "id", Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "$schema", Other, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-03/schema#", "extends",
       ApplicatorValueOrElementsInPlace, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "type", ApplicatorElements, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "disallow", ApplicatorElementsInPlace,
       "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "properties", ApplicatorMembers, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "patternProperties", ApplicatorMembers,
       "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "additionalProperties", ApplicatorValue,
       "properties", "patternProperties")
  WALK(HTTP_BASE "draft-03/schema#", "items", ApplicatorValueOrElements, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "additionalItems", ApplicatorValue,
       "items")
  WALK(HTTP_BASE "draft-03/schema#", "minItems", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "maxItems", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "uniqueItems", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "required", Assertion, "$ref")
  WALK(HTTP_BASE "draft-03/schema#", "dependencies", ApplicatorMembers, "$ref")
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
                       ApplicatorValue, HTTP_BASE "draft-03/schema#", "$ref")

  // $ref also takes precedence over any unknown keyword
  if (vocabularies.contains(HTTP_BASE "draft-03/schema#") &&
      keyword != "$ref") {
    return {
        sourcemeta::jsontoolkit::KeywordType::Unknown, std::nullopt, {"$ref"}};
  }

  // Draft2
  WALK(HTTP_BASE "draft-02/schema#", "$schema", Other)
  WALK(HTTP_BASE "draft-02/schema#", "id", Other)
  WALK(HTTP_BASE "draft-02/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-02/schema#", "items", ApplicatorValueOrElements)
  WALK(HTTP_BASE "draft-02/schema#", "properties", ApplicatorMembers)
  WALK(HTTP_BASE "draft-02/schema#", "additionalProperties", ApplicatorValue,
       "properties")
  WALK(HTTP_BASE "draft-02/schema#", "type", ApplicatorElements)
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
  WALK(HTTP_BASE "draft-02/schema#", "requires", ApplicatorValueInPlace)
  WALK(HTTP_BASE "draft-02/schema#", "format", Other)
  WALK(HTTP_BASE "draft-02/schema#", "title", Comment)
  WALK(HTTP_BASE "draft-02/schema#", "description", Comment)
  WALK(HTTP_BASE "draft-02/schema#", "default", Comment)
  WALK(HTTP_BASE "draft-02/schema#", "divisibleBy", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "disallow", Assertion)
  WALK(HTTP_BASE "draft-02/schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-02/schema#", "contentEncoding", Comment)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "requires", ApplicatorValueInPlace)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "targetSchema", ApplicatorValue)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "type", ApplicatorElements)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "items", ApplicatorValueOrElements)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "properties", ApplicatorMembers)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-02/hyper-schema#", "additionalProperties",
       ApplicatorValue, "properties")

  // Draft1
  WALK(HTTP_BASE "draft-01/schema#", "$schema", Other)
  WALK(HTTP_BASE "draft-01/schema#", "id", Other)
  WALK(HTTP_BASE "draft-01/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-01/schema#", "items", ApplicatorValueOrElements)
  WALK(HTTP_BASE "draft-01/schema#", "properties", ApplicatorMembers)
  WALK(HTTP_BASE "draft-01/schema#", "additionalProperties", ApplicatorValue,
       "properties")
  WALK(HTTP_BASE "draft-01/schema#", "type", ApplicatorElements)
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
  WALK(HTTP_BASE "draft-01/schema#", "requires", ApplicatorValueInPlace)
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
  WALK(HTTP_BASE "draft-01/hyper-schema#", "type", ApplicatorElements)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "items", ApplicatorValueOrElements)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "properties", ApplicatorMembers)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "requires", ApplicatorValueInPlace)
  WALK(HTTP_BASE "draft-01/hyper-schema#", "additionalProperties",
       ApplicatorValue, "properties")

  // Draft0
  WALK(HTTP_BASE "draft-00/schema#", "$schema", Other)
  WALK(HTTP_BASE "draft-00/schema#", "id", Other)
  WALK(HTTP_BASE "draft-00/schema#", "$ref", Reference)
  WALK(HTTP_BASE "draft-00/schema#", "items", ApplicatorValueOrElements)
  WALK(HTTP_BASE "draft-00/schema#", "properties", ApplicatorMembers)
  WALK(HTTP_BASE "draft-00/schema#", "additionalProperties", ApplicatorValue,
       "properties")
  WALK(HTTP_BASE "draft-00/schema#", "type", ApplicatorElements)
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
  WALK(HTTP_BASE "draft-00/schema#", "requires", ApplicatorValueInPlace)
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
  WALK(HTTP_BASE "draft-00/hyper-schema#", "type", ApplicatorElements)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "items", ApplicatorValueOrElements)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "properties", ApplicatorMembers)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "extends",
       ApplicatorValueOrElementsInPlace)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "requires", ApplicatorValueInPlace)
  WALK(HTTP_BASE "draft-00/hyper-schema#", "additionalProperties",
       ApplicatorValue, "properties")
#undef HTTP_BASE
#undef WALK
#undef WALK_ANY
#undef WALK_MAYBE_DEPENDENT
  return {sourcemeta::jsontoolkit::KeywordType::Unknown, std::nullopt, {}};
}
