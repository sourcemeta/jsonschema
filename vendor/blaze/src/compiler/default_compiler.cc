#include <sourcemeta/blaze/compiler.h>

#include "default_compiler_2019_09.h"
#include "default_compiler_2020_12.h"
#include "default_compiler_draft3.h"
#include "default_compiler_draft4.h"
#include "default_compiler_draft6.h"
#include "default_compiler_draft7.h"

#include <algorithm>     // std::ranges::any_of
#include <cassert>       // assert
#include <string>        // std::string
#include <unordered_set> // std::unordered_set

namespace sourcemeta::blaze {

// A keyword that asserts nothing still needs an arm in the dispatch table, as
// the table is what settles whether a keyword belongs to the dialect at all
auto compiler_openapi_noop(const KeywordContext &, const Instructions &)
    -> Instructions {
  return {};
}

} // namespace sourcemeta::blaze

namespace {

// Whether any element of the array is a schema
auto any_element_is_a_schema(
    const sourcemeta::core::JSON &value,
    const sourcemeta::core::SchemaVocabularies &vocabularies,
    const sourcemeta::core::JSON::String &keyword) -> bool {
  using namespace sourcemeta::blaze;
  return value.is_array() &&
         std::ranges::any_of(
             value.as_array(),
             [&vocabularies, &keyword](const auto &element) -> bool {
               return is_keyword_schema(element, vocabularies, keyword);
             });
}

// Whether a schema sits where a keyword of this kind holds one, and so would
// have to compile. A single schema is enough to settle it, as several of these
// keywords let a schema stand beside a value that is not one: Draft 3 `type`
// lists type names among its subschemas, and `dependencies` lists property
// names among them, so asking that every member or element be a schema would
// hand a schema to a compiler that cannot be given one
auto holds_schema(const sourcemeta::core::SchemaKeywordType type,
                  const sourcemeta::core::JSON &value,
                  const sourcemeta::core::SchemaVocabularies &vocabularies,
                  const sourcemeta::core::JSON::String &keyword) -> bool {
  using namespace sourcemeta::blaze;
  switch (type) {
    case sourcemeta::core::SchemaKeywordType::Unknown:
    case sourcemeta::core::SchemaKeywordType::Assertion:
    case sourcemeta::core::SchemaKeywordType::Annotation:
    case sourcemeta::core::SchemaKeywordType::Reference:
    case sourcemeta::core::SchemaKeywordType::Other:
    case sourcemeta::core::SchemaKeywordType::Comment:
      return false;
    // The schemas are the members
    case sourcemeta::core::SchemaKeywordType::LocationMembers:
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorMembersTraversePropertyStatic:
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorMembersTraversePropertyRegex:
    case sourcemeta::core::SchemaKeywordType::ApplicatorMembersInPlaceSome:
      return value.is_object() &&
             std::ranges::any_of(
                 value.as_object(),
                 [&vocabularies, &keyword](const auto &entry) -> bool {
                   return is_keyword_schema(entry.second, vocabularies,
                                            keyword);
                 });
    // The schemas are the elements, and the value itself is never one
    case sourcemeta::core::SchemaKeywordType::ApplicatorElementsTraverseItem:
    case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlace:
    case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlaceSome:
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorElementsInPlaceSomeNegate:
      return any_element_is_a_schema(value, vocabularies, keyword);
    // Either the value is the schema, or its elements are
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorValueOrElementsTraverseAnyItemOrItem:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueOrElementsInPlace:
      return is_keyword_schema(value, vocabularies, keyword) ||
             any_element_is_a_schema(value, vocabularies, keyword);
    // The value is the schema
    default:
      return is_keyword_schema(value, vocabularies, keyword);
  }
}

// NOLINTNEXTLINE(google-readability-function-size,hicpp-function-size,readability-function-size)
auto dispatch_keyword(const sourcemeta::blaze::KeywordContext &context,
                      const sourcemeta::blaze::Instructions &current)
    -> sourcemeta::blaze::Instructions {
  assert(!context.keyword().empty());

  using Known = sourcemeta::core::SchemaVocabularies::Known;
  static std::unordered_set<sourcemeta::core::SchemaVocabularies::URI>
      supported_vocabularies{Known::JSON_SCHEMA_2020_12_CORE,
                             Known::JSON_SCHEMA_2020_12_APPLICATOR,
                             Known::JSON_SCHEMA_2020_12_UNEVALUATED,
                             Known::JSON_SCHEMA_2020_12_VALIDATION,
                             Known::JSON_SCHEMA_2020_12_META_DATA,
                             Known::JSON_SCHEMA_2020_12_FORMAT_ANNOTATION,
                             Known::JSON_SCHEMA_2020_12_FORMAT_ASSERTION,
                             Known::JSON_SCHEMA_2020_12_CONTENT,
                             Known::JSON_SCHEMA_2019_09_CORE,
                             Known::JSON_SCHEMA_2019_09_APPLICATOR,
                             Known::JSON_SCHEMA_2019_09_VALIDATION,
                             Known::JSON_SCHEMA_2019_09_META_DATA,
                             Known::JSON_SCHEMA_2019_09_FORMAT,
                             Known::JSON_SCHEMA_2019_09_CONTENT,
                             Known::JSON_SCHEMA_2019_09_HYPER_SCHEMA,
                             Known::JSON_SCHEMA_DRAFT_7,
                             Known::JSON_SCHEMA_DRAFT_7_HYPER,
                             Known::JSON_SCHEMA_DRAFT_6,
                             Known::JSON_SCHEMA_DRAFT_6_HYPER,
                             Known::JSON_SCHEMA_DRAFT_4,
                             Known::JSON_SCHEMA_DRAFT_4_HYPER,
                             Known::JSON_SCHEMA_DRAFT_3,
                             Known::JSON_SCHEMA_DRAFT_3_HYPER,
                             Known::OPENAPI_3_0_BASE,
                             Known::OPENAPI_3_1_BASE,
                             Known::OPENAPI_3_2_BASE,
                             Known::SOURCEMETA_EXTENSION_V1};

  context.vocabularies().throw_if_any_unsupported(
      supported_vocabularies, "Cannot compile unsupported vocabulary");

  using namespace sourcemeta::blaze;

#define COMPILE(vocabulary, _keyword, handler)                                 \
  if (context.vocabularies().contains(vocabulary) &&                           \
      context.keyword() == (_keyword)) {                                       \
    return handler(context, current);                                          \
  }

#define COMPILE_ANY(vocabulary_1, vocabulary_2, _keyword, handler)             \
  if ((context.vocabularies().contains(vocabulary_1) ||                        \
       context.vocabularies().contains(vocabulary_2)) &&                       \
      context.keyword() == (_keyword)) {                                       \
    return handler(context, current);                                          \
  }

// A rung covers both the validation vocabulary and the hyper-schema one that
// shares it, which is what `is_draft<rung>` answers
#define COMPILE_DRAFT(rung, _keyword, handler)                                 \
  if (is_draft##rung(context.vocabularies()) &&                                \
      context.keyword() == (_keyword)) {                                       \
    return handler(context, current);                                          \
  }

  // ********************************************
  // 2020-12
  // ********************************************

  COMPILE(Known::JSON_SCHEMA_2020_12_CORE, "$dynamicRef",
          compiler_2020_12_core_dynamicref);

  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "prefixItems",
          compiler_2020_12_applicator_prefixitems);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "items",
          compiler_2020_12_applicator_items);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "contains",
          compiler_2020_12_applicator_contains);

  // Same as 2019-09

  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "dependentRequired",
          compiler_2019_09_validation_dependentrequired);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "dependentSchemas",
          compiler_2019_09_applicator_dependentschemas);

  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "properties",
          compiler_2019_09_applicator_properties);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "patternProperties",
          compiler_2019_09_applicator_patternproperties);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "additionalProperties",
          compiler_2019_09_applicator_additionalproperties);
  COMPILE(Known::JSON_SCHEMA_2020_12_UNEVALUATED, "unevaluatedProperties",
          compiler_2019_09_applicator_unevaluatedproperties);
  COMPILE(Known::JSON_SCHEMA_2020_12_UNEVALUATED, "unevaluatedItems",
          compiler_2019_09_applicator_unevaluateditems);
  COMPILE(Known::JSON_SCHEMA_2020_12_CONTENT, "contentEncoding",
          compiler_2019_09_content_contentencoding);
  COMPILE(Known::JSON_SCHEMA_2020_12_CONTENT, "contentMediaType",
          compiler_2019_09_content_contentmediatype);
  COMPILE(Known::JSON_SCHEMA_2020_12_CONTENT, "contentSchema",
          compiler_2019_09_content_contentschema);
  COMPILE(Known::JSON_SCHEMA_2020_12_FORMAT_ANNOTATION, "format",
          compiler_draft3_validation_format);
  COMPILE(Known::JSON_SCHEMA_2020_12_FORMAT_ASSERTION, "format",
          compiler_draft3_validation_format);

  // Same as Draft 7

  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "if",
          compiler_draft7_applicator_if);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "then",
          compiler_draft7_applicator_then);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "propertyNames",
          compiler_draft6_validation_propertynames);

  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "type",
          compiler_draft6_validation_type);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "const",
          compiler_draft6_validation_const);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "exclusiveMaximum",
          compiler_draft6_validation_exclusivemaximum);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "exclusiveMinimum",
          compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  // As per compatibility optional test
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "dependencies",
          compiler_draft3_applicator_dependencies);

  COMPILE(Known::JSON_SCHEMA_2020_12_CORE, "$ref", compiler_draft3_core_ref);

  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "allOf",
          compiler_draft4_applicator_allof);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE(Known::JSON_SCHEMA_2020_12_APPLICATOR, "not",
          compiler_draft4_applicator_not);

  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "enum",
          compiler_draft3_validation_enum);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "uniqueItems",
          compiler_draft3_validation_uniqueitems);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "maxItems",
          compiler_draft3_validation_maxitems);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "minItems",
          compiler_draft3_validation_minitems);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "required",
          compiler_draft4_validation_required);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "minProperties",
          compiler_draft4_validation_minproperties);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "maximum",
          compiler_draft3_validation_maximum);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "minimum",
          compiler_draft3_validation_minimum);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "multipleOf",
          compiler_draft3_validation_divisibleby);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "maxLength",
          compiler_draft3_validation_maxlength);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "minLength",
          compiler_draft3_validation_minlength);
  COMPILE(Known::JSON_SCHEMA_2020_12_VALIDATION, "pattern",
          compiler_draft3_validation_pattern);

  // ********************************************
  // 2019-09
  // ********************************************

  COMPILE(Known::JSON_SCHEMA_2019_09_CORE, "$recursiveRef",
          compiler_2019_09_core_recursiveref);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "dependentRequired",
          compiler_2019_09_validation_dependentrequired);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "dependentSchemas",
          compiler_2019_09_applicator_dependentschemas);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "contains",
          compiler_2019_09_applicator_contains);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "unevaluatedItems",
          compiler_2019_09_applicator_unevaluateditems);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "unevaluatedProperties",
          compiler_2019_09_applicator_unevaluatedproperties);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "items",
          compiler_2019_09_applicator_items);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "additionalItems",
          compiler_2019_09_applicator_additionalitems);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "properties",
          compiler_2019_09_applicator_properties);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "patternProperties",
          compiler_2019_09_applicator_patternproperties);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "additionalProperties",
          compiler_2019_09_applicator_additionalproperties);
  COMPILE(Known::JSON_SCHEMA_2019_09_CONTENT, "contentEncoding",
          compiler_2019_09_content_contentencoding);
  COMPILE(Known::JSON_SCHEMA_2019_09_CONTENT, "contentMediaType",
          compiler_2019_09_content_contentmediatype);
  COMPILE(Known::JSON_SCHEMA_2019_09_CONTENT, "contentSchema",
          compiler_2019_09_content_contentschema);
  COMPILE(Known::JSON_SCHEMA_2019_09_FORMAT, "format",
          compiler_draft3_validation_format);

  // Same as Draft 7

  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "if",
          compiler_draft7_applicator_if);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "then",
          compiler_draft7_applicator_then);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "propertyNames",
          compiler_draft6_validation_propertynames);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "type",
          compiler_draft6_validation_type);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "const",
          compiler_draft6_validation_const);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "exclusiveMaximum",
          compiler_draft6_validation_exclusivemaximum);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "exclusiveMinimum",
          compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  // As per compatibility optional test
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "dependencies",
          compiler_draft3_applicator_dependencies);

  COMPILE(Known::JSON_SCHEMA_2019_09_CORE, "$ref", compiler_draft3_core_ref);

  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "allOf",
          compiler_draft4_applicator_allof);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE(Known::JSON_SCHEMA_2019_09_APPLICATOR, "not",
          compiler_draft4_applicator_not);

  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "enum",
          compiler_draft3_validation_enum);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "uniqueItems",
          compiler_draft3_validation_uniqueitems);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "maxItems",
          compiler_draft3_validation_maxitems);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "minItems",
          compiler_draft3_validation_minitems);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "required",
          compiler_draft4_validation_required);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "minProperties",
          compiler_draft4_validation_minproperties);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "maximum",
          compiler_draft3_validation_maximum);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "minimum",
          compiler_draft3_validation_minimum);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "multipleOf",
          compiler_draft3_validation_divisibleby);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "maxLength",
          compiler_draft3_validation_maxlength);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "minLength",
          compiler_draft3_validation_minlength);
  COMPILE(Known::JSON_SCHEMA_2019_09_VALIDATION, "pattern",
          compiler_draft3_validation_pattern);

  // ********************************************
  // DRAFT 7
  // ********************************************

  COMPILE_DRAFT(7, "$ref", compiler_draft3_core_ref);

  // Any
  COMPILE_DRAFT(7, "if", compiler_draft7_applicator_if);
  COMPILE_DRAFT(7, "then", compiler_draft7_applicator_then);
  COMPILE_DRAFT(7, "else", compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE_DRAFT(7, "type", compiler_draft6_validation_type);
  COMPILE_DRAFT(7, "const", compiler_draft6_validation_const);
  COMPILE_DRAFT(7, "contains", compiler_draft6_applicator_contains);
  COMPILE_DRAFT(7, "propertyNames", compiler_draft6_validation_propertynames);
  COMPILE_DRAFT(7, "exclusiveMaximum",
                compiler_draft6_validation_exclusivemaximum);
  COMPILE_DRAFT(7, "exclusiveMinimum",
                compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_DRAFT(7, "allOf", compiler_draft4_applicator_allof);
  COMPILE_DRAFT(7, "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_DRAFT(7, "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_DRAFT(7, "not", compiler_draft4_applicator_not);
  COMPILE_DRAFT(7, "enum", compiler_draft3_validation_enum);

  COMPILE_DRAFT(7, "items", compiler_draft3_applicator_items);
  COMPILE_DRAFT(7, "additionalItems",
                compiler_draft3_applicator_additionalitems);
  COMPILE_DRAFT(7, "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_DRAFT(7, "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_DRAFT(7, "minItems", compiler_draft3_validation_minitems);

  COMPILE_DRAFT(7, "required", compiler_draft4_validation_required);
  COMPILE_DRAFT(7, "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_DRAFT(7, "minProperties", compiler_draft4_validation_minproperties);
  COMPILE_DRAFT(7, "properties", compiler_draft3_applicator_properties);
  COMPILE_DRAFT(7, "patternProperties",
                compiler_draft3_applicator_patternproperties);
  COMPILE_DRAFT(7, "additionalProperties",
                compiler_draft3_applicator_additionalproperties);
  COMPILE_DRAFT(7, "dependencies", compiler_draft3_applicator_dependencies);

  COMPILE_DRAFT(7, "maximum", compiler_draft3_validation_maximum);
  COMPILE_DRAFT(7, "minimum", compiler_draft3_validation_minimum);
  COMPILE_DRAFT(7, "multipleOf", compiler_draft3_validation_divisibleby);

  COMPILE_DRAFT(7, "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_DRAFT(7, "minLength", compiler_draft3_validation_minlength);
  COMPILE_DRAFT(7, "pattern", compiler_draft3_validation_pattern);
  COMPILE_DRAFT(7, "format", compiler_draft3_validation_format);

  // ********************************************
  // DRAFT 6
  // ********************************************

  COMPILE_DRAFT(6, "$ref", compiler_draft3_core_ref);

  // Any
  COMPILE_DRAFT(6, "type", compiler_draft6_validation_type);
  COMPILE_DRAFT(6, "const", compiler_draft6_validation_const);

  // Array
  COMPILE_DRAFT(6, "contains", compiler_draft6_applicator_contains);

  // Object
  COMPILE_DRAFT(6, "propertyNames", compiler_draft6_validation_propertynames);

  // Number
  COMPILE_DRAFT(6, "exclusiveMaximum",
                compiler_draft6_validation_exclusivemaximum);
  COMPILE_DRAFT(6, "exclusiveMinimum",
                compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_DRAFT(6, "allOf", compiler_draft4_applicator_allof);
  COMPILE_DRAFT(6, "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_DRAFT(6, "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_DRAFT(6, "not", compiler_draft4_applicator_not);
  COMPILE_DRAFT(6, "enum", compiler_draft3_validation_enum);

  COMPILE_DRAFT(6, "items", compiler_draft3_applicator_items);
  COMPILE_DRAFT(6, "additionalItems",
                compiler_draft3_applicator_additionalitems);
  COMPILE_DRAFT(6, "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_DRAFT(6, "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_DRAFT(6, "minItems", compiler_draft3_validation_minitems);

  COMPILE_DRAFT(6, "required", compiler_draft4_validation_required);
  COMPILE_DRAFT(6, "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_DRAFT(6, "minProperties", compiler_draft4_validation_minproperties);
  COMPILE_DRAFT(6, "properties", compiler_draft3_applicator_properties);
  COMPILE_DRAFT(6, "patternProperties",
                compiler_draft3_applicator_patternproperties);
  COMPILE_DRAFT(6, "additionalProperties",
                compiler_draft3_applicator_additionalproperties);
  COMPILE_DRAFT(6, "dependencies", compiler_draft3_applicator_dependencies);

  COMPILE_DRAFT(6, "maximum", compiler_draft3_validation_maximum);
  COMPILE_DRAFT(6, "minimum", compiler_draft3_validation_minimum);
  COMPILE_DRAFT(6, "multipleOf", compiler_draft3_validation_divisibleby);

  COMPILE_DRAFT(6, "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_DRAFT(6, "minLength", compiler_draft3_validation_minlength);
  COMPILE_DRAFT(6, "pattern", compiler_draft3_validation_pattern);
  COMPILE_DRAFT(6, "format", compiler_draft3_validation_format);

  // ********************************************
  // DRAFT 4
  // ********************************************

  COMPILE_DRAFT(4, "$ref", compiler_draft3_core_ref);

  // Applicators
  COMPILE_DRAFT(4, "allOf", compiler_draft4_applicator_allof);
  COMPILE_DRAFT(4, "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_DRAFT(4, "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_DRAFT(4, "not", compiler_draft4_applicator_not);
  COMPILE_DRAFT(4, "properties", compiler_draft3_applicator_properties);
  COMPILE_DRAFT(4, "patternProperties",
                compiler_draft3_applicator_patternproperties);
  COMPILE_DRAFT(4, "additionalProperties",
                compiler_draft3_applicator_additionalproperties);
  COMPILE_DRAFT(4, "items", compiler_draft3_applicator_items);
  COMPILE_DRAFT(4, "additionalItems",
                compiler_draft3_applicator_additionalitems);
  COMPILE_DRAFT(4, "dependencies", compiler_draft3_applicator_dependencies);

  // Any
  COMPILE_DRAFT(4, "type", compiler_draft3_validation_type);
  COMPILE_DRAFT(4, "enum", compiler_draft3_validation_enum);

  // Object
  COMPILE_DRAFT(4, "required", compiler_draft4_validation_required);
  COMPILE_DRAFT(4, "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_DRAFT(4, "minProperties", compiler_draft4_validation_minproperties);

  // Array
  COMPILE_DRAFT(4, "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_DRAFT(4, "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_DRAFT(4, "minItems", compiler_draft3_validation_minitems);

  // String
  COMPILE_DRAFT(4, "pattern", compiler_draft3_validation_pattern);
  COMPILE_DRAFT(4, "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_DRAFT(4, "minLength", compiler_draft3_validation_minlength);
  COMPILE_DRAFT(4, "format", compiler_draft3_validation_format);

  // Number
  COMPILE_DRAFT(4, "maximum", compiler_draft3_validation_maximum);
  COMPILE_DRAFT(4, "minimum", compiler_draft3_validation_minimum);
  COMPILE_DRAFT(4, "multipleOf", compiler_draft3_validation_divisibleby);

  // ********************************************
  // DRAFT 3
  // ********************************************

  COMPILE_DRAFT(3, "$ref", compiler_draft3_core_ref);

  // Applicators
  COMPILE_DRAFT(3, "extends", compiler_draft3_applicator_extends);
  COMPILE_DRAFT(3, "properties", compiler_draft3_applicator_properties);
  COMPILE_DRAFT(3, "patternProperties",
                compiler_draft3_applicator_patternproperties);
  COMPILE_DRAFT(3, "additionalProperties",
                compiler_draft3_applicator_additionalproperties);
  COMPILE_DRAFT(3, "items", compiler_draft3_applicator_items);
  COMPILE_DRAFT(3, "additionalItems",
                compiler_draft3_applicator_additionalitems);
  COMPILE_DRAFT(3, "dependencies", compiler_draft3_applicator_dependencies);

  // Any
  COMPILE_DRAFT(3, "type", compiler_draft3_validation_type);
  COMPILE_DRAFT(3, "disallow", compiler_draft3_validation_disallow);
  COMPILE_DRAFT(3, "enum", compiler_draft3_validation_enum);

  // Array
  COMPILE_DRAFT(3, "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_DRAFT(3, "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_DRAFT(3, "minItems", compiler_draft3_validation_minitems);

  // String
  COMPILE_DRAFT(3, "pattern", compiler_draft3_validation_pattern);
  COMPILE_DRAFT(3, "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_DRAFT(3, "minLength", compiler_draft3_validation_minlength);
  COMPILE_DRAFT(3, "format", compiler_draft3_validation_format);

  // Number
  COMPILE_DRAFT(3, "maximum", compiler_draft3_validation_maximum);
  COMPILE_DRAFT(3, "minimum", compiler_draft3_validation_minimum);
  COMPILE_DRAFT(3, "divisibleBy", compiler_draft3_validation_divisibleby);

  // ********************************************
  // OpenAPI
  // ********************************************

  COMPILE_ANY(Known::OPENAPI_3_1_BASE, Known::OPENAPI_3_2_BASE, "discriminator",
              compiler_openapi_noop);
  COMPILE_ANY(Known::OPENAPI_3_1_BASE, Known::OPENAPI_3_2_BASE, "xml",
              compiler_openapi_noop);
  COMPILE_ANY(Known::OPENAPI_3_1_BASE, Known::OPENAPI_3_2_BASE, "externalDocs",
              compiler_openapi_noop);
  COMPILE_ANY(Known::OPENAPI_3_1_BASE, Known::OPENAPI_3_2_BASE, "example",
              compiler_openapi_noop);

  // ********************************************
  // OPENAPI 3.0
  // ********************************************

  // Every arm below guards on this vocabulary alone, never sharing a
  // `COMPILE_ANY` with a JSON Schema one. The OpenAPI 3.0 Schema Object is a
  // closed dialect whose keyword set is neither a subset nor a superset of
  // Draft 4, so a shared arm would hand it a keyword it must not have. The
  // rejection at the end of this function relies on this table naming every
  // keyword of the dialect and nothing else

  COMPILE(Known::OPENAPI_3_0_BASE, "$ref", compiler_draft3_core_ref);

  // Applicators
  COMPILE(Known::OPENAPI_3_0_BASE, "allOf", compiler_draft4_applicator_allof);
  COMPILE(Known::OPENAPI_3_0_BASE, "anyOf", compiler_draft4_applicator_anyof);
  COMPILE(Known::OPENAPI_3_0_BASE, "oneOf", compiler_draft4_applicator_oneof);
  COMPILE(Known::OPENAPI_3_0_BASE, "not", compiler_draft4_applicator_not);
  COMPILE(Known::OPENAPI_3_0_BASE, "properties",
          compiler_draft3_applicator_properties);
  COMPILE(Known::OPENAPI_3_0_BASE, "additionalProperties",
          compiler_draft3_applicator_additionalproperties);
  COMPILE(Known::OPENAPI_3_0_BASE, "items", compiler_draft3_applicator_items);

  // Any
  COMPILE(Known::OPENAPI_3_0_BASE, "type", compiler_draft3_validation_type);
  COMPILE(Known::OPENAPI_3_0_BASE, "enum", compiler_draft3_validation_enum);

  // Object
  COMPILE(Known::OPENAPI_3_0_BASE, "required",
          compiler_draft4_validation_required);
  COMPILE(Known::OPENAPI_3_0_BASE, "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE(Known::OPENAPI_3_0_BASE, "minProperties",
          compiler_draft4_validation_minproperties);

  // Array
  COMPILE(Known::OPENAPI_3_0_BASE, "uniqueItems",
          compiler_draft3_validation_uniqueitems);
  COMPILE(Known::OPENAPI_3_0_BASE, "maxItems",
          compiler_draft3_validation_maxitems);
  COMPILE(Known::OPENAPI_3_0_BASE, "minItems",
          compiler_draft3_validation_minitems);

  // String
  COMPILE(Known::OPENAPI_3_0_BASE, "pattern",
          compiler_draft3_validation_pattern);
  COMPILE(Known::OPENAPI_3_0_BASE, "maxLength",
          compiler_draft3_validation_maxlength);
  COMPILE(Known::OPENAPI_3_0_BASE, "minLength",
          compiler_draft3_validation_minlength);
  COMPILE(Known::OPENAPI_3_0_BASE, "format", compiler_draft3_validation_format);

  // Number
  COMPILE(Known::OPENAPI_3_0_BASE, "maximum",
          compiler_draft3_validation_maximum);
  COMPILE(Known::OPENAPI_3_0_BASE, "minimum",
          compiler_draft3_validation_minimum);
  COMPILE(Known::OPENAPI_3_0_BASE, "multipleOf",
          compiler_draft3_validation_divisibleby);

  // `maximum` and `minimum` read these as siblings, as they do on the Draft 4
  // rung, so the keywords themselves emit nothing. They need an arm all the
  // same, or the rejection below would refuse them
  COMPILE(Known::OPENAPI_3_0_BASE, "exclusiveMaximum", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "exclusiveMinimum", compiler_openapi_noop);

  // `type` reads `nullable` as a sibling, and the rest assert nothing at all
  COMPILE(Known::OPENAPI_3_0_BASE, "nullable", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "discriminator", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "xml", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "externalDocs", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "example", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "readOnly", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "writeOnly", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "deprecated", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "title", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "description", compiler_openapi_noop);
  COMPILE(Known::OPENAPI_3_0_BASE, "default", compiler_openapi_noop);

#undef COMPILE
#undef COMPILE_ANY

  // The OpenAPI 3.0 Schema Object is closed: its meta-schema sets
  // `additionalProperties` to `false` and admits no member other than the
  // keywords it names and those prefixed with `x-`. A member outside that set
  // is no keyword we may ignore, so compilation refuses it rather than passing
  // it by, and the table above is therefore a complete enumeration of the
  // dialect rather than a convenience
  if (context.vocabularies().contains(Known::OPENAPI_3_0_BASE) &&
      !context.keyword().starts_with("x-")) {
    throw_keyword_error(context, PROHIBITED_KEYWORD_FOR_DIALECT);
  }

  if ((context.vocabularies().contains(Known::JSON_SCHEMA_2019_09_CORE) ||
       context.vocabularies().contains(Known::JSON_SCHEMA_2020_12_CORE)) &&
      !context.keyword().starts_with('$') &&
      context.keyword() != "definitions") {

    // We handle these keywords as part of "contains"
    if ((context.vocabularies().contains(
             Known::JSON_SCHEMA_2019_09_VALIDATION) ||
         context.vocabularies().contains(
             Known::JSON_SCHEMA_2020_12_VALIDATION)) &&
        (context.keyword() == "minContains" ||
         context.keyword() == "maxContains")) {
      return {};
    }

    if (!annotations_enabled(context) || context.local.is_property_name) {
      return {};
    }

    return compiler_2019_09_core_annotation(context, current);
  }

  return {};
}

} // namespace

auto sourcemeta::blaze::default_schema_compiler(
    const KeywordContext &context,
    const sourcemeta::blaze::Instructions &current)
    -> sourcemeta::blaze::Instructions {
  if (ref_overrides_sibling_keywords(context)) {
    const auto &metadata{
        context.global.walker(context.keyword(), context.vocabularies())};
    if (metadata.type != sourcemeta::core::SchemaKeywordType::Reference) {
      // Whether the shape an overridden keyword takes is one its dialect
      // allows at all is still for the keyword compiler to judge, so such a
      // keyword compiles for that judgement alone and what it makes is thrown
      // away. One holding a schema is where that stops, as core's schema
      // walker frames nothing under an overridden keyword, and an unframed
      // schema is not one this compiler can be handed. The dialect that reads
      // a reference as an object of its own stops earlier still, as there the
      // sibling is not merely overridden but ignored.
      if (!ref_object_ignores_siblings(context.vocabularies()) &&
          !holds_schema(metadata.type, context.value(), context.vocabularies(),
                        context.keyword())) {
        [[maybe_unused]] const auto discarded{
            dispatch_keyword(context, current)};
      }

      return {};
    }
  }

  return dispatch_keyword(context, current);
}
