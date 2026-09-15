#include <sourcemeta/blaze/compiler.h>

#include "default_compiler_2019_09.h"
#include "default_compiler_2020_12.h"
#include "default_compiler_draft3.h"
#include "default_compiler_draft4.h"
#include "default_compiler_draft6.h"
#include "default_compiler_draft7.h"
#include "default_compiler_openapi.h"

#include <cassert>       // assert
#include <string>        // std::string
#include <unordered_set> // std::unordered_set

// NOLINTNEXTLINE(google-readability-function-size,hicpp-function-size,readability-function-size)
auto sourcemeta::blaze::default_schema_compiler(
    const sourcemeta::blaze::Context &context,
    const sourcemeta::blaze::SchemaContext &schema_context,
    const sourcemeta::blaze::DynamicContext &dynamic_context,
    const sourcemeta::blaze::Instructions &current)
    -> sourcemeta::blaze::Instructions {
  assert(!dynamic_context.keyword.empty());

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
                             Known::OPENAPI_3_1_BASE,
                             Known::OPENAPI_3_2_BASE,
                             Known::SOURCEMETA_EXTENSION_V1};

  schema_context.vocabularies.throw_if_any_unsupported(
      supported_vocabularies, "Cannot compile unsupported vocabulary");

  using namespace sourcemeta::blaze;

#define COMPILE(vocabulary, _keyword, handler)                                 \
  if (schema_context.vocabularies.contains(vocabulary) &&                      \
      dynamic_context.keyword == (_keyword)) {                                 \
    return internal::handler(context, schema_context, dynamic_context,         \
                             current);                                         \
  }

#define COMPILE_ANY(vocabulary_1, vocabulary_2, _keyword, handler)             \
  if ((schema_context.vocabularies.contains(vocabulary_1) ||                   \
       schema_context.vocabularies.contains(vocabulary_2)) &&                  \
      dynamic_context.keyword == (_keyword)) {                                 \
    return internal::handler(context, schema_context, dynamic_context,         \
                             current);                                         \
  }

#define STOP_IF_SIBLING_KEYWORD(vocabulary, _keyword)                          \
  if (schema_context.vocabularies.contains(vocabulary) &&                      \
      schema_context.schema.is_object() &&                                     \
      schema_context.schema.defines(_keyword)) {                               \
    return {};                                                                 \
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

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "$ref", compiler_draft3_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_7, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_7_HYPER, "$ref");

  // Any
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "if", compiler_draft7_applicator_if);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "then", compiler_draft7_applicator_then);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "else", compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "type", compiler_draft6_validation_type);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "const", compiler_draft6_validation_const);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "contains", compiler_draft6_applicator_contains);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "propertyNames", compiler_draft6_validation_propertynames);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "allOf", compiler_draft4_applicator_allof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "not", compiler_draft4_applicator_not);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "enum", compiler_draft3_validation_enum);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "items", compiler_draft3_applicator_items);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "additionalItems", compiler_draft3_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "minItems", compiler_draft3_validation_minitems);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "required", compiler_draft4_validation_required);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "minProperties", compiler_draft4_validation_minproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "properties", compiler_draft3_applicator_properties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "patternProperties",
              compiler_draft3_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "additionalProperties",
              compiler_draft3_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "dependencies", compiler_draft3_applicator_dependencies);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "maximum", compiler_draft3_validation_maximum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "minimum", compiler_draft3_validation_minimum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "multipleOf", compiler_draft3_validation_divisibleby);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "minLength", compiler_draft3_validation_minlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "pattern", compiler_draft3_validation_pattern);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER,
              "format", compiler_draft3_validation_format);

  // ********************************************
  // DRAFT 6
  // ********************************************

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "$ref", compiler_draft3_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_6, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_6_HYPER, "$ref");

  // Any
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "type", compiler_draft6_validation_type);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "const", compiler_draft6_validation_const);

  // Array
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "contains", compiler_draft6_applicator_contains);

  // Object
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "propertyNames", compiler_draft6_validation_propertynames);

  // Number
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "allOf", compiler_draft4_applicator_allof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "not", compiler_draft4_applicator_not);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "enum", compiler_draft3_validation_enum);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "items", compiler_draft3_applicator_items);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "additionalItems", compiler_draft3_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "minItems", compiler_draft3_validation_minitems);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "required", compiler_draft4_validation_required);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "minProperties", compiler_draft4_validation_minproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "properties", compiler_draft3_applicator_properties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "patternProperties",
              compiler_draft3_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "additionalProperties",
              compiler_draft3_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "dependencies", compiler_draft3_applicator_dependencies);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "maximum", compiler_draft3_validation_maximum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "minimum", compiler_draft3_validation_minimum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "multipleOf", compiler_draft3_validation_divisibleby);

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "minLength", compiler_draft3_validation_minlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "pattern", compiler_draft3_validation_pattern);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER,
              "format", compiler_draft3_validation_format);

  // ********************************************
  // DRAFT 4
  // ********************************************

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "$ref", compiler_draft3_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_4, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_4_HYPER, "$ref");

  // Applicators
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "allOf", compiler_draft4_applicator_allof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "not", compiler_draft4_applicator_not);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "properties", compiler_draft3_applicator_properties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "patternProperties",
              compiler_draft3_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "additionalProperties",
              compiler_draft3_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "items", compiler_draft3_applicator_items);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "additionalItems", compiler_draft3_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "dependencies", compiler_draft3_applicator_dependencies);

  // Any
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "type", compiler_draft3_validation_type);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "enum", compiler_draft3_validation_enum);

  // Object
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "required", compiler_draft4_validation_required);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "minProperties", compiler_draft4_validation_minproperties);

  // Array
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "minItems", compiler_draft3_validation_minitems);

  // String
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "pattern", compiler_draft3_validation_pattern);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "minLength", compiler_draft3_validation_minlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "format", compiler_draft3_validation_format);

  // Number
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "maximum", compiler_draft3_validation_maximum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "minimum", compiler_draft3_validation_minimum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
              "multipleOf", compiler_draft3_validation_divisibleby);

  // ********************************************
  // DRAFT 3
  // ********************************************

  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "$ref", compiler_draft3_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_3, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_SCHEMA_DRAFT_3_HYPER, "$ref");

  // Applicators
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "extends", compiler_draft3_applicator_extends);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "properties", compiler_draft3_applicator_properties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "patternProperties",
              compiler_draft3_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "additionalProperties",
              compiler_draft3_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "items", compiler_draft3_applicator_items);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "additionalItems", compiler_draft3_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "dependencies", compiler_draft3_applicator_dependencies);

  // Any
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "type", compiler_draft3_validation_type);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "disallow", compiler_draft3_validation_disallow);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "enum", compiler_draft3_validation_enum);

  // Array
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "uniqueItems", compiler_draft3_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "maxItems", compiler_draft3_validation_maxitems);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "minItems", compiler_draft3_validation_minitems);

  // String
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "pattern", compiler_draft3_validation_pattern);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "maxLength", compiler_draft3_validation_maxlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "minLength", compiler_draft3_validation_minlength);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "format", compiler_draft3_validation_format);

  // Number
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "maximum", compiler_draft3_validation_maximum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "minimum", compiler_draft3_validation_minimum);
  COMPILE_ANY(Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
              "divisibleBy", compiler_draft3_validation_divisibleby);

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

#undef COMPILE
#undef COMPILE_ANY
#undef STOP_IF_SIBLING_KEYWORD

  if ((schema_context.vocabularies.contains(Known::JSON_SCHEMA_2019_09_CORE) ||
       schema_context.vocabularies.contains(Known::JSON_SCHEMA_2020_12_CORE)) &&
      !dynamic_context.keyword.starts_with('$') &&
      dynamic_context.keyword != "definitions") {

    // We handle these keywords as part of "contains"
    if ((schema_context.vocabularies.contains(
             Known::JSON_SCHEMA_2019_09_VALIDATION) ||
         schema_context.vocabularies.contains(
             Known::JSON_SCHEMA_2020_12_VALIDATION)) &&
        (dynamic_context.keyword == "minContains" ||
         dynamic_context.keyword == "maxContains")) {
      return {};
    }

    if (!annotations_enabled(context, dynamic_context.keyword) ||
        schema_context.is_property_name) {
      return {};
    }

    return internal::compiler_2019_09_core_annotation(context, schema_context,
                                                      dynamic_context, current);
  }

  return {};
}
