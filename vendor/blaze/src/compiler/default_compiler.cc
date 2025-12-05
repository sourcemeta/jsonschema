#include <sourcemeta/blaze/compiler.h>

#include "default_compiler_2019_09.h"
#include "default_compiler_2020_12.h"
#include "default_compiler_draft4.h"
#include "default_compiler_draft6.h"
#include "default_compiler_draft7.h"

#include <cassert>       // assert
#include <string>        // std::string
#include <unordered_set> // std::unordered_set

auto sourcemeta::blaze::default_schema_compiler(
    const sourcemeta::blaze::Context &context,
    const sourcemeta::blaze::SchemaContext &schema_context,
    const sourcemeta::blaze::DynamicContext &dynamic_context,
    const sourcemeta::blaze::Instructions &current)
    -> sourcemeta::blaze::Instructions {
  assert(!dynamic_context.keyword.empty());

  using Known = sourcemeta::core::Vocabularies::Known;
  static std::unordered_set<sourcemeta::core::Vocabularies::URI>
      SUPPORTED_VOCABULARIES{Known::JSON_Schema_2020_12_Core,
                             Known::JSON_Schema_2020_12_Applicator,
                             Known::JSON_Schema_2020_12_Unevaluated,
                             Known::JSON_Schema_2020_12_Validation,
                             Known::JSON_Schema_2020_12_Meta_Data,
                             Known::JSON_Schema_2020_12_Format_Annotation,
                             Known::JSON_Schema_2020_12_Format_Assertion,
                             Known::JSON_Schema_2020_12_Content,
                             Known::JSON_Schema_2019_09_Core,
                             Known::JSON_Schema_2019_09_Applicator,
                             Known::JSON_Schema_2019_09_Validation,
                             Known::JSON_Schema_2019_09_Meta_Data,
                             Known::JSON_Schema_2019_09_Format,
                             Known::JSON_Schema_2019_09_Content,
                             Known::JSON_Schema_2019_09_Hyper_Schema,
                             Known::JSON_Schema_Draft_7,
                             Known::JSON_Schema_Draft_7_Hyper,
                             Known::JSON_Schema_Draft_6,
                             Known::JSON_Schema_Draft_6_Hyper,
                             Known::JSON_Schema_Draft_4,
                             Known::JSON_Schema_Draft_4_Hyper};

  schema_context.vocabularies.throw_if_any_unsupported(
      SUPPORTED_VOCABULARIES, "Cannot compile unsupported vocabulary");

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

  COMPILE(Known::JSON_Schema_2020_12_Core, "$dynamicRef",
          compiler_2020_12_core_dynamicref);

  COMPILE(Known::JSON_Schema_2020_12_Applicator, "prefixItems",
          compiler_2020_12_applicator_prefixitems);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "items",
          compiler_2020_12_applicator_items);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "contains",
          compiler_2020_12_applicator_contains);

  // Same as 2019-09

  COMPILE(Known::JSON_Schema_2020_12_Validation, "dependentRequired",
          compiler_2019_09_validation_dependentrequired);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "dependentSchemas",
          compiler_2019_09_applicator_dependentschemas);

  COMPILE(Known::JSON_Schema_2020_12_Applicator, "properties",
          compiler_2019_09_applicator_properties);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "patternProperties",
          compiler_2019_09_applicator_patternproperties);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "additionalProperties",
          compiler_2019_09_applicator_additionalproperties);
  COMPILE(Known::JSON_Schema_2020_12_Unevaluated, "unevaluatedProperties",
          compiler_2019_09_applicator_unevaluatedproperties);
  COMPILE(Known::JSON_Schema_2020_12_Unevaluated, "unevaluatedItems",
          compiler_2019_09_applicator_unevaluateditems);
  COMPILE(Known::JSON_Schema_2020_12_Content, "contentEncoding",
          compiler_2019_09_content_contentencoding);
  COMPILE(Known::JSON_Schema_2020_12_Content, "contentMediaType",
          compiler_2019_09_content_contentmediatype);
  COMPILE(Known::JSON_Schema_2020_12_Content, "contentSchema",
          compiler_2019_09_content_contentschema);
  COMPILE(Known::JSON_Schema_2020_12_Format_Annotation, "format",
          compiler_2019_09_format_format);

  // Same as Draft 7

  COMPILE(Known::JSON_Schema_2020_12_Applicator, "if",
          compiler_draft7_applicator_if);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "then",
          compiler_draft7_applicator_then);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE(Known::JSON_Schema_2020_12_Applicator, "propertyNames",
          compiler_draft6_validation_propertynames);

  COMPILE(Known::JSON_Schema_2020_12_Validation, "type",
          compiler_draft6_validation_type);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "const",
          compiler_draft6_validation_const);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "exclusiveMaximum",
          compiler_draft6_validation_exclusivemaximum);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "exclusiveMinimum",
          compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  // As per compatibility optional test
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "dependencies",
          compiler_draft4_applicator_dependencies);

  COMPILE(Known::JSON_Schema_2020_12_Core, "$ref", compiler_draft4_core_ref);

  COMPILE(Known::JSON_Schema_2020_12_Applicator, "allOf",
          compiler_draft4_applicator_allof);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE(Known::JSON_Schema_2020_12_Applicator, "not",
          compiler_draft4_applicator_not);

  COMPILE(Known::JSON_Schema_2020_12_Validation, "enum",
          compiler_draft4_validation_enum);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "uniqueItems",
          compiler_draft4_validation_uniqueitems);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "minItems",
          compiler_draft4_validation_minitems);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "required",
          compiler_draft4_validation_required);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "minProperties",
          compiler_draft4_validation_minproperties);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "maximum",
          compiler_draft4_validation_maximum);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "minimum",
          compiler_draft4_validation_minimum);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "multipleOf",
          compiler_draft4_validation_multipleof);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "minLength",
          compiler_draft4_validation_minlength);
  COMPILE(Known::JSON_Schema_2020_12_Validation, "pattern",
          compiler_draft4_validation_pattern);

  // ********************************************
  // 2019-09
  // ********************************************

  COMPILE(Known::JSON_Schema_2019_09_Core, "$recursiveRef",
          compiler_2019_09_core_recursiveref);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "dependentRequired",
          compiler_2019_09_validation_dependentrequired);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "dependentSchemas",
          compiler_2019_09_applicator_dependentschemas);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "contains",
          compiler_2019_09_applicator_contains);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "unevaluatedItems",
          compiler_2019_09_applicator_unevaluateditems);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "unevaluatedProperties",
          compiler_2019_09_applicator_unevaluatedproperties);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "items",
          compiler_2019_09_applicator_items);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "additionalItems",
          compiler_2019_09_applicator_additionalitems);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "properties",
          compiler_2019_09_applicator_properties);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "patternProperties",
          compiler_2019_09_applicator_patternproperties);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "additionalProperties",
          compiler_2019_09_applicator_additionalproperties);
  COMPILE(Known::JSON_Schema_2019_09_Content, "contentEncoding",
          compiler_2019_09_content_contentencoding);
  COMPILE(Known::JSON_Schema_2019_09_Content, "contentMediaType",
          compiler_2019_09_content_contentmediatype);
  COMPILE(Known::JSON_Schema_2019_09_Content, "contentSchema",
          compiler_2019_09_content_contentschema);
  COMPILE(Known::JSON_Schema_2019_09_Format, "format",
          compiler_2019_09_format_format);

  // Same as Draft 7

  COMPILE(Known::JSON_Schema_2019_09_Applicator, "if",
          compiler_draft7_applicator_if);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "then",
          compiler_draft7_applicator_then);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE(Known::JSON_Schema_2019_09_Applicator, "propertyNames",
          compiler_draft6_validation_propertynames);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "type",
          compiler_draft6_validation_type);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "const",
          compiler_draft6_validation_const);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "exclusiveMaximum",
          compiler_draft6_validation_exclusivemaximum);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "exclusiveMinimum",
          compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  // As per compatibility optional test
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "dependencies",
          compiler_draft4_applicator_dependencies);

  COMPILE(Known::JSON_Schema_2019_09_Core, "$ref", compiler_draft4_core_ref);

  COMPILE(Known::JSON_Schema_2019_09_Applicator, "allOf",
          compiler_draft4_applicator_allof);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE(Known::JSON_Schema_2019_09_Applicator, "not",
          compiler_draft4_applicator_not);

  COMPILE(Known::JSON_Schema_2019_09_Validation, "enum",
          compiler_draft4_validation_enum);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "uniqueItems",
          compiler_draft4_validation_uniqueitems);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "minItems",
          compiler_draft4_validation_minitems);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "required",
          compiler_draft4_validation_required);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "minProperties",
          compiler_draft4_validation_minproperties);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "maximum",
          compiler_draft4_validation_maximum);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "minimum",
          compiler_draft4_validation_minimum);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "multipleOf",
          compiler_draft4_validation_multipleof);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "minLength",
          compiler_draft4_validation_minlength);
  COMPILE(Known::JSON_Schema_2019_09_Validation, "pattern",
          compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 7
  // ********************************************

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "$ref", compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_Schema_Draft_7, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_Schema_Draft_7_Hyper, "$ref");

  // Any
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "if", compiler_draft7_applicator_if);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "then", compiler_draft7_applicator_then);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "else", compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "type", compiler_draft6_validation_type);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "const", compiler_draft6_validation_const);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "contains", compiler_draft6_applicator_contains);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "propertyNames", compiler_draft6_validation_propertynames);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "allOf", compiler_draft4_applicator_allof);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "not", compiler_draft4_applicator_not);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "enum", compiler_draft4_validation_enum);

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "items", compiler_draft4_applicator_items);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "additionalItems", compiler_draft4_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "uniqueItems", compiler_draft4_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "maxItems", compiler_draft4_validation_maxitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "minItems", compiler_draft4_validation_minitems);

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "required", compiler_draft4_validation_required);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "minProperties", compiler_draft4_validation_minproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "properties", compiler_draft4_applicator_properties);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "patternProperties",
              compiler_draft4_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "additionalProperties",
              compiler_draft4_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "dependencies", compiler_draft4_applicator_dependencies);

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "maximum", compiler_draft4_validation_maximum);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "minimum", compiler_draft4_validation_minimum);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "multipleOf", compiler_draft4_validation_multipleof);

  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "maxLength", compiler_draft4_validation_maxlength);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "minLength", compiler_draft4_validation_minlength);
  COMPILE_ANY(Known::JSON_Schema_Draft_7, Known::JSON_Schema_Draft_7_Hyper,
              "pattern", compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 6
  // ********************************************

  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "$ref", compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_Schema_Draft_6, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_Schema_Draft_6_Hyper, "$ref");

  // Any
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "type", compiler_draft6_validation_type);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "const", compiler_draft6_validation_const);

  // Array
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "contains", compiler_draft6_applicator_contains);

  // Object
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "propertyNames", compiler_draft6_validation_propertynames);

  // Number
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "allOf", compiler_draft4_applicator_allof);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "not", compiler_draft4_applicator_not);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "enum", compiler_draft4_validation_enum);

  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "items", compiler_draft4_applicator_items);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "additionalItems", compiler_draft4_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "uniqueItems", compiler_draft4_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "maxItems", compiler_draft4_validation_maxitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "minItems", compiler_draft4_validation_minitems);

  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "required", compiler_draft4_validation_required);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "minProperties", compiler_draft4_validation_minproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "properties", compiler_draft4_applicator_properties);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "patternProperties",
              compiler_draft4_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "additionalProperties",
              compiler_draft4_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "dependencies", compiler_draft4_applicator_dependencies);

  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "maximum", compiler_draft4_validation_maximum);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "minimum", compiler_draft4_validation_minimum);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "multipleOf", compiler_draft4_validation_multipleof);

  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "maxLength", compiler_draft4_validation_maxlength);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "minLength", compiler_draft4_validation_minlength);
  COMPILE_ANY(Known::JSON_Schema_Draft_6, Known::JSON_Schema_Draft_6_Hyper,
              "pattern", compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 4
  // ********************************************

  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "$ref", compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD(Known::JSON_Schema_Draft_4, "$ref");
  STOP_IF_SIBLING_KEYWORD(Known::JSON_Schema_Draft_4_Hyper, "$ref");

  // Applicators
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "allOf", compiler_draft4_applicator_allof);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "anyOf", compiler_draft4_applicator_anyof);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "oneOf", compiler_draft4_applicator_oneof);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "not", compiler_draft4_applicator_not);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "properties", compiler_draft4_applicator_properties);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "patternProperties",
              compiler_draft4_applicator_patternproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "additionalProperties",
              compiler_draft4_applicator_additionalproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "items", compiler_draft4_applicator_items);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "additionalItems", compiler_draft4_applicator_additionalitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "dependencies", compiler_draft4_applicator_dependencies);

  // Any
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "type", compiler_draft4_validation_type);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "enum", compiler_draft4_validation_enum);

  // Object
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "required", compiler_draft4_validation_required);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "minProperties", compiler_draft4_validation_minproperties);

  // Array
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "uniqueItems", compiler_draft4_validation_uniqueitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "maxItems", compiler_draft4_validation_maxitems);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "minItems", compiler_draft4_validation_minitems);

  // String
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "pattern", compiler_draft4_validation_pattern);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "maxLength", compiler_draft4_validation_maxlength);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "minLength", compiler_draft4_validation_minlength);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "format", compiler_draft4_validation_format);

  // Number
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "maximum", compiler_draft4_validation_maximum);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "minimum", compiler_draft4_validation_minimum);
  COMPILE_ANY(Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper,
              "multipleOf", compiler_draft4_validation_multipleof);

#undef COMPILE
#undef COMPILE_ANY
#undef STOP_IF_SIBLING_KEYWORD

  if ((schema_context.vocabularies.contains(Known::JSON_Schema_2019_09_Core) ||
       schema_context.vocabularies.contains(Known::JSON_Schema_2020_12_Core)) &&
      !dynamic_context.keyword.starts_with('$') &&
      dynamic_context.keyword != "definitions") {

    // We handle these keywords as part of "contains"
    if ((schema_context.vocabularies.contains(
             Known::JSON_Schema_2019_09_Validation) ||
         schema_context.vocabularies.contains(
             Known::JSON_Schema_2020_12_Validation)) &&
        (dynamic_context.keyword == "minContains" ||
         dynamic_context.keyword == "maxContains")) {
      return {};
    }

    if (context.mode == Mode::FastValidation ||
        schema_context.is_property_name) {
      return {};
    }

    return internal::compiler_2019_09_core_annotation(context, schema_context,
                                                      dynamic_context, current);
  }

  return {};
}
