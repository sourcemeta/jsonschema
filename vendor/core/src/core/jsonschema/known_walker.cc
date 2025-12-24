#include <sourcemeta/core/jsonschema.h>

#include <unordered_map> // std::unordered_map

namespace sourcemeta::core {

namespace {

using Known = Vocabularies::Known;
using KeywordHandler =
    const SchemaWalkerResult &(*)(const Vocabularies &vocabularies);

static const SchemaWalkerResult UNKNOWN_RESULT{
    SchemaKeywordType::Unknown, std::nullopt, {}, {}, {}};

static const SchemaWalkerResult UNKNOWN_WITH_REF_RESULT{
    SchemaKeywordType::Unknown, std::nullopt, {"$ref"}, {}, {}};

auto has_draft3_to_7(const Vocabularies &vocabularies) -> bool {
  return vocabularies.contains(Known::JSON_Schema_Draft_7) ||
         vocabularies.contains(Known::JSON_Schema_Draft_7_Hyper) ||
         vocabularies.contains(Known::JSON_Schema_Draft_6) ||
         vocabularies.contains(Known::JSON_Schema_Draft_6_Hyper) ||
         vocabularies.contains(Known::JSON_Schema_Draft_4) ||
         vocabularies.contains(Known::JSON_Schema_Draft_4_Hyper) ||
         vocabularies.contains(Known::JSON_Schema_Draft_3) ||
         vocabularies.contains(Known::JSON_Schema_Draft_3_Hyper);
}

#define RETURN_WITH_DEPENDENCIES(_vocabulary, _types, _strategy, ...)          \
  {                                                                            \
    static const SchemaWalkerResult result{                                    \
        SchemaKeywordType::_strategy, _vocabulary, {__VA_ARGS__}, {}, _types}; \
    return result;                                                             \
  }

#define RETURN_WITH_ORDER_DEPENDENCIES(_vocabulary, _types, _strategy, ...)    \
  {                                                                            \
    static const SchemaWalkerResult result{                                    \
        SchemaKeywordType::_strategy, _vocabulary, {}, {__VA_ARGS__}, _types}; \
    return result;                                                             \
  }

#define RETURN(_vocabulary, _types, _strategy)                                 \
  {                                                                            \
    static const SchemaWalkerResult result{                                    \
        SchemaKeywordType::_strategy, _vocabulary, {}, {}, _types};            \
    return result;                                                             \
  }

#define CHECK_VOCABULARY_WITH_DEPENDENCIES(_vocabulary, _types, _strategy,     \
                                           ...)                                \
  if (vocabularies.contains(_vocabulary)) {                                    \
    RETURN_WITH_DEPENDENCIES(_vocabulary, _types, _strategy, __VA_ARGS__)      \
  }

#define CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(_vocabulary, _types,          \
                                                 _strategy, ...)               \
  if (vocabularies.contains(_vocabulary)) {                                    \
    RETURN_WITH_ORDER_DEPENDENCIES(_vocabulary, _types, _strategy,             \
                                   __VA_ARGS__)                                \
  }

#define CHECK_VOCABULARY(_vocabulary, _types, _strategy)                       \
  if (vocabularies.contains(_vocabulary)) {                                    \
    RETURN(_vocabulary, _types, _strategy)                                     \
  }

auto handle_dollar_id(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Other)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Other, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_dollar_schema(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Other)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_dollar_ref(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3_Hyper, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Reference)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Reference)
  return UNKNOWN_RESULT;
}

auto handle_dollar_defs(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, LocationMembers)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, LocationMembers)
  return UNKNOWN_RESULT;
}

auto handle_definitions(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, LocationMembers)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, LocationMembers)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     LocationMembers, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     LocationMembers, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {},
                                     LocationMembers, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     LocationMembers, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {},
                                     LocationMembers, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     LocationMembers, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_dollar_comment(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Comment)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_dollar_anchor(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_dollar_vocabulary(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_dollar_dynamicRef(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Reference)
  return UNKNOWN_RESULT;
}

auto handle_dollar_dynamicAnchor(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Core, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_dollar_recursiveRef(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Reference)
  return UNKNOWN_RESULT;
}

auto handle_dollar_recursiveAnchor(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Core, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_id(const Vocabularies &vocabularies) -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {}, Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_oneOf(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_anyOf(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_allOf(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_if(const Vocabularies &vocabularies) -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator, {},
                   ApplicatorValueInPlaceMaybe)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator, {},
                   ApplicatorValueInPlaceMaybe)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorValueInPlaceMaybe, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorValueInPlaceMaybe, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_then(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_2020_12_Applicator, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_2019_09_Applicator, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  return UNKNOWN_RESULT;
}

auto handle_else(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_2020_12_Applicator, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_2019_09_Applicator, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorValueInPlaceMaybe, "if")
  return UNKNOWN_RESULT;
}

auto handle_not(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator, {},
                   ApplicatorValueInPlaceNegate)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator, {},
                   ApplicatorValueInPlaceNegate)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                     ApplicatorValueInPlaceNegate, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorValueInPlaceNegate, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {},
                                     ApplicatorValueInPlaceNegate, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     ApplicatorValueInPlaceNegate, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {},
                                     ApplicatorValueInPlaceNegate, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     ApplicatorValueInPlaceNegate, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_properties(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  if (vocabularies.contains(Known::JSON_Schema_2020_12_Applicator)) {
    if (vocabularies.contains(Known::JSON_Schema_2020_12_Validation)) {
      RETURN_WITH_ORDER_DEPENDENCIES(
          Known::JSON_Schema_2020_12_Applicator, make_set({JSON::Type::Object}),
          ApplicatorMembersTraversePropertyStatic, "required")
    }
    RETURN(Known::JSON_Schema_2020_12_Applicator,
           make_set({JSON::Type::Object}),
           ApplicatorMembersTraversePropertyStatic)
  }
  if (vocabularies.contains(Known::JSON_Schema_2019_09_Applicator)) {
    if (vocabularies.contains(Known::JSON_Schema_2019_09_Validation)) {
      RETURN_WITH_ORDER_DEPENDENCIES(
          Known::JSON_Schema_2019_09_Applicator, make_set({JSON::Type::Object}),
          ApplicatorMembersTraversePropertyStatic, "required")
    }
    RETURN(Known::JSON_Schema_2019_09_Applicator,
           make_set({JSON::Type::Object}),
           ApplicatorMembersTraversePropertyStatic)
  }
  if (vocabularies.contains(Known::JSON_Schema_Draft_7)) {
    static const SchemaWalkerResult result{
        SchemaKeywordType::ApplicatorMembersTraversePropertyStatic,
        Known::JSON_Schema_Draft_7,
        {"$ref"},
        {"required"},
        make_set({JSON::Type::Object})};
    return result;
  }
  if (vocabularies.contains(Known::JSON_Schema_Draft_7_Hyper)) {
    static const SchemaWalkerResult result{
        SchemaKeywordType::ApplicatorMembersTraversePropertyStatic,
        Known::JSON_Schema_Draft_7_Hyper,
        {"$ref"},
        {"required"},
        make_set({JSON::Type::Object})};
    return result;
  }
  if (vocabularies.contains(Known::JSON_Schema_Draft_6)) {
    static const SchemaWalkerResult result{
        SchemaKeywordType::ApplicatorMembersTraversePropertyStatic,
        Known::JSON_Schema_Draft_6,
        {"$ref"},
        {"required"},
        make_set({JSON::Type::Object})};
    return result;
  }
  if (vocabularies.contains(Known::JSON_Schema_Draft_6_Hyper)) {
    static const SchemaWalkerResult result{
        SchemaKeywordType::ApplicatorMembersTraversePropertyStatic,
        Known::JSON_Schema_Draft_6_Hyper,
        {"$ref"},
        {"required"},
        make_set({JSON::Type::Object})};
    return result;
  }
  if (vocabularies.contains(Known::JSON_Schema_Draft_4)) {
    static const SchemaWalkerResult result{
        SchemaKeywordType::ApplicatorMembersTraversePropertyStatic,
        Known::JSON_Schema_Draft_4,
        {"$ref"},
        {"required"},
        make_set({JSON::Type::Object})};
    return result;
  }
  if (vocabularies.contains(Known::JSON_Schema_Draft_4_Hyper)) {
    static const SchemaWalkerResult result{
        SchemaKeywordType::ApplicatorMembersTraversePropertyStatic,
        Known::JSON_Schema_Draft_4_Hyper,
        {"$ref"},
        {"required"},
        make_set({JSON::Type::Object})};
    return result;
  }
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyStatic, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyStatic, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyStatic)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyStatic)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyStatic)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyStatic)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyStatic)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyStatic)
  return UNKNOWN_RESULT;
}

auto handle_additionalProperties(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_2020_12_Applicator, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_2019_09_Applicator, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties", "patternProperties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_2, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_2_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_1, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_1_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_0, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_0_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseSomeProperty, "properties")
  return UNKNOWN_RESULT;
}

auto handle_patternProperties(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator,
                   make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyRegex)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator,
                   make_set({JSON::Type::Object}),
                   ApplicatorMembersTraversePropertyRegex)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper, make_set({JSON::Type::Object}),
      ApplicatorMembersTraversePropertyRegex, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_propertyNames(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator,
                   make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseAnyPropertyKey)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator,
                   make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseAnyPropertyKey)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseAnyPropertyKey, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseAnyPropertyKey, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseAnyPropertyKey, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper, make_set({JSON::Type::Object}),
      ApplicatorValueTraverseAnyPropertyKey, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_dependentSchemas(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator,
                   make_set({JSON::Type::Object}), ApplicatorMembersInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator,
                   make_set({JSON::Type::Object}), ApplicatorMembersInPlaceSome)
  return UNKNOWN_RESULT;
}

auto handle_dependencies(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::Object}),
                                     ApplicatorMembersInPlaceSome, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_contains(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  if (vocabularies.contains(Known::JSON_Schema_2020_12_Applicator)) {
    if (vocabularies.contains(Known::JSON_Schema_2020_12_Validation)) {
      RETURN_WITH_DEPENDENCIES(
          Known::JSON_Schema_2020_12_Applicator, make_set({JSON::Type::Array}),
          ApplicatorValueTraverseAnyItem, "minContains", "maxContains")
    }
    RETURN(Known::JSON_Schema_2020_12_Applicator, make_set({JSON::Type::Array}),
           ApplicatorValueTraverseAnyItem)
  }
  if (vocabularies.contains(Known::JSON_Schema_2019_09_Applicator)) {
    if (vocabularies.contains(Known::JSON_Schema_2019_09_Validation)) {
      RETURN_WITH_DEPENDENCIES(
          Known::JSON_Schema_2019_09_Applicator, make_set({JSON::Type::Array}),
          ApplicatorValueTraverseAnyItem, "minContains", "maxContains")
    }
    RETURN(Known::JSON_Schema_2019_09_Applicator, make_set({JSON::Type::Array}),
           ApplicatorValueTraverseAnyItem)
  }
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseAnyItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseAnyItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseAnyItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseAnyItem, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_items(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_2020_12_Applicator, make_set({JSON::Type::Array}),
      ApplicatorValueTraverseSomeItem, "prefixItems")
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Applicator,
                   make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper, make_set({JSON::Type::Array}),
      ApplicatorValueOrElementsTraverseAnyItemOrItem, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Array}),
                   ApplicatorValueOrElementsTraverseAnyItemOrItem)
  return UNKNOWN_RESULT;
}

auto handle_prefixItems(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Applicator,
                   make_set({JSON::Type::Array}),
                   ApplicatorElementsTraverseItem)
  return UNKNOWN_RESULT;
}

auto handle_additionalItems(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_2019_09_Applicator,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::Array}),
                                     ApplicatorValueTraverseSomeItem, "items")
  return UNKNOWN_RESULT;
}

auto handle_unevaluatedProperties(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  if (vocabularies.contains(Known::JSON_Schema_2020_12_Unevaluated)) {
    if (vocabularies.contains(Known::JSON_Schema_2020_12_Applicator)) {
      RETURN_WITH_DEPENDENCIES(
          Known::JSON_Schema_2020_12_Unevaluated,
          make_set({JSON::Type::Object}), ApplicatorValueTraverseSomeProperty,
          "properties", "patternProperties", "additionalProperties")
    }
    RETURN(Known::JSON_Schema_2020_12_Unevaluated,
           make_set({JSON::Type::Object}), ApplicatorValueTraverseSomeProperty)
  }
  if (vocabularies.contains(Known::JSON_Schema_2019_09_Applicator)) {
    RETURN_WITH_DEPENDENCIES(Known::JSON_Schema_2019_09_Applicator,
                             make_set({JSON::Type::Object}),
                             ApplicatorValueTraverseSomeProperty, "properties",
                             "patternProperties", "additionalProperties")
  }
  return UNKNOWN_RESULT;
}

auto handle_unevaluatedItems(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  if (vocabularies.contains(Known::JSON_Schema_2020_12_Unevaluated)) {
    if (vocabularies.contains(Known::JSON_Schema_2020_12_Applicator)) {
      RETURN_WITH_DEPENDENCIES(
          Known::JSON_Schema_2020_12_Unevaluated, make_set({JSON::Type::Array}),
          ApplicatorValueTraverseSomeItem, "prefixItems", "items", "contains")
    }
    RETURN(Known::JSON_Schema_2020_12_Unevaluated,
           make_set({JSON::Type::Array}), ApplicatorValueTraverseSomeItem)
  }
  if (vocabularies.contains(Known::JSON_Schema_2019_09_Applicator)) {
    RETURN_WITH_DEPENDENCIES(
        Known::JSON_Schema_2019_09_Applicator, make_set({JSON::Type::Array}),
        ApplicatorValueTraverseSomeItem, "items", "additionalItems")
  }
  return UNKNOWN_RESULT;
}

auto handle_type(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  if (vocabularies.contains(Known::JSON_Schema_2020_12_Validation)) {
    if (vocabularies.contains(Known::JSON_Schema_2020_12_Applicator)) {
      RETURN_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_2020_12_Validation, {},
                                     Assertion, "properties")
    }
    RETURN(Known::JSON_Schema_2020_12_Validation, {}, Assertion)
  }
  if (vocabularies.contains(Known::JSON_Schema_2019_09_Validation)) {
    if (vocabularies.contains(Known::JSON_Schema_2019_09_Applicator)) {
      RETURN_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_2019_09_Validation, {},
                                     Assertion, "properties")
    }
    RETURN(Known::JSON_Schema_2019_09_Validation, {}, Assertion)
  }
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_Draft_7, {},
                                           Assertion, "properties")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                           Assertion, "properties")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_Draft_6, {},
                                           Assertion, "properties")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                           Assertion, "properties")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_Draft_4, {},
                                           Assertion, "properties")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                           Assertion, "properties")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     ApplicatorElementsInPlaceSome, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {},
                   ApplicatorElementsInPlaceSome)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {},
                   ApplicatorElementsInPlaceSome)
  return UNKNOWN_RESULT;
}

auto handle_enum(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation, {}, Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {}, Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {}, Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Assertion, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Assertion)
  return UNKNOWN_RESULT;
}

auto handle_const(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation, {}, Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Assertion, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_multipleOf(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_maximum(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(
      Known::JSON_Schema_2020_12_Validation,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "type")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(
      Known::JSON_Schema_2019_09_Validation,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "type")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_minimum(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(
      Known::JSON_Schema_2020_12_Validation,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "type")
  CHECK_VOCABULARY_WITH_ORDER_DEPENDENCIES(
      Known::JSON_Schema_2019_09_Validation,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "type")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_exclusiveMaximum(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_exclusiveMinimum(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_7_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_6_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_4_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_maxLength(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_minLength(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_pattern(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::String}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::String}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::String}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::String}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_maxItems(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_minItems(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_uniqueItems(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::Array}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::Array}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::Array}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Array}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_maxProperties(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  return UNKNOWN_RESULT;
}

auto handle_minProperties(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  return UNKNOWN_RESULT;
}

auto handle_required(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::Object}), Assertion,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::Object}), Assertion,
                                     "$ref")
  return UNKNOWN_RESULT;
}

auto handle_dependentRequired(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Object}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_minContains(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_maxContains(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Validation,
                   make_set({JSON::Type::Array}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_title(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Comment)
  return UNKNOWN_RESULT;
}

auto handle_description(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Comment)
  return UNKNOWN_RESULT;
}

auto handle_default(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Comment)
  return UNKNOWN_RESULT;
}

auto handle_deprecated(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  return UNKNOWN_RESULT;
}

auto handle_readOnly(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_writeOnly(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_examples(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Meta_Data, {}, Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Comment, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6, {}, Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Comment, "$ref")
  return UNKNOWN_RESULT;
}

auto handle_format(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Format_Assertion,
                   make_set({JSON::Type::String}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Format_Annotation,
                   make_set({JSON::Type::String}), Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Format,
                   make_set({JSON::Type::String}), Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::String}), Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::String}), Other,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::String}), Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::String}), Other,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4,
                                     make_set({JSON::Type::String}), Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper,
                                     make_set({JSON::Type::String}), Other,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3,
                                     make_set({JSON::Type::String}), Other,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper,
                                     make_set({JSON::Type::String}), Other,
                                     "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::String}),
                   Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::String}), Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::String}),
                   Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::String}), Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::String}),
                   Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::String}), Other)
  return UNKNOWN_RESULT;
}

auto handle_contentSchema(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Content,
                   make_set({JSON::Type::String}), ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Content,
                   make_set({JSON::Type::String}), ApplicatorValueInPlaceOther)
  return UNKNOWN_RESULT;
}

auto handle_contentMediaType(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Content,
                   make_set({JSON::Type::String}), Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Content,
                   make_set({JSON::Type::String}), Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::String}), Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::String}), Comment,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::String}), Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::String}), Comment,
                                     "$ref")
  return UNKNOWN_RESULT;
}

auto handle_contentEncoding(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2020_12_Content,
                   make_set({JSON::Type::String}), Annotation)
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Content,
                   make_set({JSON::Type::String}), Annotation)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7,
                                     make_set({JSON::Type::String}), Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper,
                                     make_set({JSON::Type::String}), Comment,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6,
                                     make_set({JSON::Type::String}), Comment,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper,
                                     make_set({JSON::Type::String}), Comment,
                                     "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  if (vocabularies.contains(Known::JSON_Schema_Draft_3) ||
      vocabularies.contains(Known::JSON_Schema_Draft_4)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::String}),
                   Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::String}), Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::String}),
                   Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::String}), Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::String}),
                   Comment)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::String}), Comment)
  return UNKNOWN_RESULT;
}

auto handle_extends(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {},
                                     ApplicatorValueOrElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     ApplicatorValueOrElementsInPlace, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {},
                   ApplicatorValueOrElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {},
                   ApplicatorValueOrElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {},
                   ApplicatorValueOrElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {},
                   ApplicatorValueOrElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {},
                   ApplicatorValueOrElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {},
                   ApplicatorValueOrElementsInPlace)
  return UNKNOWN_RESULT;
}

auto handle_disallow(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3, {},
                                     ApplicatorElementsInPlaceSomeNegate,
                                     ("$ref"))
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     ApplicatorElementsInPlaceSomeNegate,
                                     ("$ref"))
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, {}, Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Assertion)
  return UNKNOWN_RESULT;
}

auto handle_divisibleBy(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(
      Known::JSON_Schema_Draft_3_Hyper,
      make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_maximumCanEqual(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_minimumCanEqual(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Integer, JSON::Type::Real}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_requires(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2, make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseParent)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper,
                   make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseParent)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseParent)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseParent)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseParent)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Object}),
                   ApplicatorValueTraverseParent)
  return UNKNOWN_RESULT;
}

auto handle_optional(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Object}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Object}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Object}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Object}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_maxDecimal(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1, make_set({JSON::Type::Real}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper,
                   make_set({JSON::Type::Real}), Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0, make_set({JSON::Type::Real}),
                   Assertion)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper,
                   make_set({JSON::Type::Real}), Assertion)
  return UNKNOWN_RESULT;
}

auto handle_links(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     ApplicatorElementsInPlace, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {},
                   ApplicatorElementsInPlace)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_base(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_7_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Other, "$ref")
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_anchor(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_anchorPointer(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_rel(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_href(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_templatePointers(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_templateRequired(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_targetMediaType(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_targetHints(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_submissionMediaType(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_hrefSchema(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {},
                   ApplicatorValueInPlaceOther)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_targetSchema(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3_Hyper, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {},
                   ApplicatorValueInPlaceOther)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_headerSchema(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {},
                   ApplicatorValueInPlaceOther)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_submissionSchema(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_2019_09_Hyper_Schema, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_7_Hyper, {},
                   ApplicatorValueInPlaceOther)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {},
                   ApplicatorValueInPlaceOther)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_media(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_6_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Other, "$ref")
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_fragmentResolution(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_root(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_readonly(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_pathStart(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_4_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_mediaType(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {}, Other)
  CHECK_VOCABULARY_WITH_DEPENDENCIES(Known::JSON_Schema_Draft_3_Hyper, {},
                                     Other, "$ref")
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_alternate(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {},
                   ApplicatorElementsInPlace)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {},
                   ApplicatorElementsInPlace)
  return UNKNOWN_RESULT;
}

auto handle_method(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_enctype(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_3_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_2_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_1_Hyper, {}, Other)
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_0_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_encType(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_submissionEncType(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_6_Hyper, {}, Other)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

auto handle_schema_hyper(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::JSON_Schema_Draft_4_Hyper, {},
                   ApplicatorValueInPlaceOther)
  if (has_draft3_to_7(vocabularies)) {
    return UNKNOWN_WITH_REF_RESULT;
  }
  return UNKNOWN_RESULT;
}

// OpenAPI 3.1/3.2 Base Vocabulary
// https://spec.openapis.org/oas/v3.1.0.html#fixed-fields-19
// https://spec.openapis.org/oas/v3.2.0.html#fixed-fields-20

auto handle_discriminator(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::OpenAPI_3_2_Base, {}, Other)
  CHECK_VOCABULARY(Known::OpenAPI_3_1_Base, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_xml(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::OpenAPI_3_2_Base, {}, Other)
  CHECK_VOCABULARY(Known::OpenAPI_3_1_Base, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_externalDocs(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::OpenAPI_3_2_Base, {}, Other)
  CHECK_VOCABULARY(Known::OpenAPI_3_1_Base, {}, Other)
  return UNKNOWN_RESULT;
}

auto handle_example(const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  CHECK_VOCABULARY(Known::OpenAPI_3_2_Base, {}, Other)
  CHECK_VOCABULARY(Known::OpenAPI_3_1_Base, {}, Other)
  return UNKNOWN_RESULT;
}

#undef RETURN_WITH_DEPENDENCIES
#undef RETURN
#undef CHECK_VOCABULARY_WITH_DEPENDENCIES
#undef CHECK_VOCABULARY

} // anonymous namespace

auto schema_walker(std::string_view keyword, const Vocabularies &vocabularies)
    -> const SchemaWalkerResult & {
  // TODO: Make use of JSON key's perfect hashes, as we mostly run the walker by
  // checking JSON property names
  static const std::unordered_map<std::string_view, KeywordHandler> handlers{
      {"$id", handle_dollar_id},
      {"$schema", handle_dollar_schema},
      {"$ref", handle_dollar_ref},
      {"$defs", handle_dollar_defs},
      {"definitions", handle_definitions},
      {"$comment", handle_dollar_comment},
      {"$anchor", handle_dollar_anchor},
      {"$vocabulary", handle_dollar_vocabulary},
      {"$dynamicRef", handle_dollar_dynamicRef},
      {"$dynamicAnchor", handle_dollar_dynamicAnchor},
      {"$recursiveRef", handle_dollar_recursiveRef},
      {"$recursiveAnchor", handle_dollar_recursiveAnchor},
      {"id", handle_id},
      {"oneOf", handle_oneOf},
      {"anyOf", handle_anyOf},
      {"allOf", handle_allOf},
      {"if", handle_if},
      {"then", handle_then},
      {"else", handle_else},
      {"not", handle_not},
      {"properties", handle_properties},
      {"additionalProperties", handle_additionalProperties},
      {"patternProperties", handle_patternProperties},
      {"propertyNames", handle_propertyNames},
      {"dependentSchemas", handle_dependentSchemas},
      {"dependencies", handle_dependencies},
      {"contains", handle_contains},
      {"items", handle_items},
      {"prefixItems", handle_prefixItems},
      {"additionalItems", handle_additionalItems},
      {"unevaluatedProperties", handle_unevaluatedProperties},
      {"unevaluatedItems", handle_unevaluatedItems},
      {"type", handle_type},
      {"enum", handle_enum},
      {"const", handle_const},
      {"multipleOf", handle_multipleOf},
      {"maximum", handle_maximum},
      {"minimum", handle_minimum},
      {"exclusiveMaximum", handle_exclusiveMaximum},
      {"exclusiveMinimum", handle_exclusiveMinimum},
      {"maxLength", handle_maxLength},
      {"minLength", handle_minLength},
      {"pattern", handle_pattern},
      {"maxItems", handle_maxItems},
      {"minItems", handle_minItems},
      {"uniqueItems", handle_uniqueItems},
      {"maxProperties", handle_maxProperties},
      {"minProperties", handle_minProperties},
      {"required", handle_required},
      {"dependentRequired", handle_dependentRequired},
      {"minContains", handle_minContains},
      {"maxContains", handle_maxContains},
      {"title", handle_title},
      {"description", handle_description},
      {"default", handle_default},
      {"deprecated", handle_deprecated},
      {"readOnly", handle_readOnly},
      {"writeOnly", handle_writeOnly},
      {"examples", handle_examples},
      {"format", handle_format},
      {"contentSchema", handle_contentSchema},
      {"contentMediaType", handle_contentMediaType},
      {"contentEncoding", handle_contentEncoding},
      {"extends", handle_extends},
      {"disallow", handle_disallow},
      {"divisibleBy", handle_divisibleBy},
      {"maximumCanEqual", handle_maximumCanEqual},
      {"minimumCanEqual", handle_minimumCanEqual},
      {"requires", handle_requires},
      {"optional", handle_optional},
      {"maxDecimal", handle_maxDecimal},
      {"links", handle_links},
      {"base", handle_base},
      {"anchor", handle_anchor},
      {"anchorPointer", handle_anchorPointer},
      {"rel", handle_rel},
      {"href", handle_href},
      {"templatePointers", handle_templatePointers},
      {"templateRequired", handle_templateRequired},
      {"targetMediaType", handle_targetMediaType},
      {"targetHints", handle_targetHints},
      {"submissionMediaType", handle_submissionMediaType},
      {"hrefSchema", handle_hrefSchema},
      {"targetSchema", handle_targetSchema},
      {"headerSchema", handle_headerSchema},
      {"submissionSchema", handle_submissionSchema},
      {"media", handle_media},
      {"fragmentResolution", handle_fragmentResolution},
      {"root", handle_root},
      {"readonly", handle_readonly},
      {"pathStart", handle_pathStart},
      {"mediaType", handle_mediaType},
      {"alternate", handle_alternate},
      {"method", handle_method},
      {"enctype", handle_enctype},
      {"encType", handle_encType},
      {"submissionEncType", handle_submissionEncType},
      {"schema", handle_schema_hyper},
      // OpenAPI
      {"discriminator", handle_discriminator},
      {"xml", handle_xml},
      {"externalDocs", handle_externalDocs},
      {"example", handle_example},
  };

  const auto iterator = handlers.find(keyword);
  if (iterator != handlers.end()) {
    return iterator->second(vocabularies);
  }

  if (vocabularies.contains(Known::JSON_Schema_Draft_7) ||
      vocabularies.contains(Known::JSON_Schema_Draft_7_Hyper) ||
      vocabularies.contains(Known::JSON_Schema_Draft_6) ||
      vocabularies.contains(Known::JSON_Schema_Draft_6_Hyper) ||
      vocabularies.contains(Known::JSON_Schema_Draft_4) ||
      vocabularies.contains(Known::JSON_Schema_Draft_4_Hyper) ||
      vocabularies.contains(Known::JSON_Schema_Draft_3) ||
      vocabularies.contains(Known::JSON_Schema_Draft_3_Hyper)) {
    return UNKNOWN_WITH_REF_RESULT;
  }

  return UNKNOWN_RESULT;
}

} // namespace sourcemeta::core
