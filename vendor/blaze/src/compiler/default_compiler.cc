#include <sourcemeta/blaze/compiler.h>

#include "default_compiler_2019_09.h"
#include "default_compiler_2020_12.h"
#include "default_compiler_draft4.h"
#include "default_compiler_draft6.h"
#include "default_compiler_draft7.h"

#include <cassert> // assert
#include <set>     // std::set
#include <string>  // std::string

auto sourcemeta::blaze::default_schema_compiler(
    const sourcemeta::blaze::Context &context,
    const sourcemeta::blaze::SchemaContext &schema_context,
    const sourcemeta::blaze::DynamicContext &dynamic_context)
    -> sourcemeta::blaze::Template {
  assert(!dynamic_context.keyword.empty());

  static std::set<std::string> SUPPORTED_VOCABULARIES{
      "https://json-schema.org/draft/2020-12/vocab/core",
      "https://json-schema.org/draft/2020-12/vocab/applicator",
      "https://json-schema.org/draft/2020-12/vocab/validation",
      "https://json-schema.org/draft/2020-12/vocab/meta-data",
      "https://json-schema.org/draft/2020-12/vocab/unevaluated",
      "https://json-schema.org/draft/2020-12/vocab/format-annotation",
      "https://json-schema.org/draft/2020-12/vocab/content",
      "https://json-schema.org/draft/2019-09/vocab/core",
      "https://json-schema.org/draft/2019-09/vocab/applicator",
      "https://json-schema.org/draft/2019-09/vocab/validation",
      "https://json-schema.org/draft/2019-09/vocab/meta-data",
      "https://json-schema.org/draft/2019-09/vocab/format",
      "https://json-schema.org/draft/2019-09/vocab/content",
      "http://json-schema.org/draft-07/schema#",
      "http://json-schema.org/draft-06/schema#",
      "http://json-schema.org/draft-04/schema#"};
  for (const auto &vocabulary : schema_context.vocabularies) {
    if (!SUPPORTED_VOCABULARIES.contains(vocabulary.first) &&
        vocabulary.second) {
      throw sourcemeta::jsontoolkit::SchemaVocabularyError(
          vocabulary.first, "Cannot compile unsupported vocabulary");
    }
  }

  using namespace sourcemeta::blaze;

#define COMPILE(vocabulary, _keyword, handler)                                 \
  if (schema_context.vocabularies.contains(vocabulary) &&                      \
      dynamic_context.keyword == (_keyword)) {                                 \
    return internal::handler(context, schema_context, dynamic_context);        \
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

  COMPILE("https://json-schema.org/draft/2020-12/vocab/core", "$dynamicRef",
          compiler_2020_12_core_dynamicref);

  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "prefixItems", compiler_2020_12_applicator_prefixitems);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "items",
          compiler_2020_12_applicator_items);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "contains",
          compiler_2020_12_applicator_contains);

  // Same as 2019-09

  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "dependentRequired", compiler_2019_09_validation_dependentrequired);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "dependentSchemas", compiler_2019_09_applicator_dependentschemas);

  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "properties", compiler_2019_09_applicator_properties);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "patternProperties", compiler_2019_09_applicator_patternproperties);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "additionalProperties",
          compiler_2019_09_applicator_additionalproperties);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/unevaluated",
          "unevaluatedProperties",
          compiler_2019_09_applicator_unevaluatedproperties);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/unevaluated",
          "unevaluatedItems", compiler_2019_09_applicator_unevaluateditems);

  // Same as Draft 7

  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "if",
          compiler_draft7_applicator_if);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "then",
          compiler_draft7_applicator_then);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "propertyNames", compiler_draft6_validation_propertynames);

  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "type",
          compiler_draft6_validation_type);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "const",
          compiler_draft6_validation_const);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  // As per compatibility optional test
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator",
          "dependencies", compiler_draft4_applicator_dependencies);

  COMPILE("https://json-schema.org/draft/2020-12/vocab/core", "$ref",
          compiler_draft4_core_ref);

  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "allOf",
          compiler_draft4_applicator_allof);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/applicator", "not",
          compiler_draft4_applicator_not);

  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "enum",
          compiler_draft4_validation_enum);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "uniqueItems", compiler_draft4_validation_uniqueitems);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "minItems",
          compiler_draft4_validation_minitems);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "required",
          compiler_draft4_validation_required);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "minProperties", compiler_draft4_validation_minproperties);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "maximum",
          compiler_draft4_validation_maximum);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "minimum",
          compiler_draft4_validation_minimum);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation",
          "multipleOf", compiler_draft4_validation_multipleof);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "minLength",
          compiler_draft4_validation_minlength);
  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "pattern",
          compiler_draft4_validation_pattern);

  // ********************************************
  // 2019-09
  // ********************************************

  COMPILE("https://json-schema.org/draft/2019-09/vocab/core", "$recursiveRef",
          compiler_2019_09_core_recursiveref);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "dependentRequired", compiler_2019_09_validation_dependentrequired);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "dependentSchemas", compiler_2019_09_applicator_dependentschemas);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "contains",
          compiler_2019_09_applicator_contains);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "unevaluatedItems", compiler_2019_09_applicator_unevaluateditems);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "unevaluatedProperties",
          compiler_2019_09_applicator_unevaluatedproperties);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "items",
          compiler_2019_09_applicator_items);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "additionalItems", compiler_2019_09_applicator_additionalitems);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "properties", compiler_2019_09_applicator_properties);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "patternProperties", compiler_2019_09_applicator_patternproperties);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "additionalProperties",
          compiler_2019_09_applicator_additionalproperties);

  // Same as Draft 7

  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "if",
          compiler_draft7_applicator_if);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "then",
          compiler_draft7_applicator_then);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "propertyNames", compiler_draft6_validation_propertynames);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "type",
          compiler_draft6_validation_type);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "const",
          compiler_draft6_validation_const);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  // As per compatibility optional test
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator",
          "dependencies", compiler_draft4_applicator_dependencies);

  COMPILE("https://json-schema.org/draft/2019-09/vocab/core", "$ref",
          compiler_draft4_core_ref);

  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "allOf",
          compiler_draft4_applicator_allof);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/applicator", "not",
          compiler_draft4_applicator_not);

  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "enum",
          compiler_draft4_validation_enum);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "uniqueItems", compiler_draft4_validation_uniqueitems);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "minItems",
          compiler_draft4_validation_minitems);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "required",
          compiler_draft4_validation_required);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "maxProperties", compiler_draft4_validation_maxproperties);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "minProperties", compiler_draft4_validation_minproperties);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "maximum",
          compiler_draft4_validation_maximum);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "minimum",
          compiler_draft4_validation_minimum);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation",
          "multipleOf", compiler_draft4_validation_multipleof);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "minLength",
          compiler_draft4_validation_minlength);
  COMPILE("https://json-schema.org/draft/2019-09/vocab/validation", "pattern",
          compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 7
  // ********************************************

  COMPILE("http://json-schema.org/draft-07/schema#", "$ref",
          compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-07/schema#", "$ref");

  // Any
  COMPILE("http://json-schema.org/draft-07/schema#", "if",
          compiler_draft7_applicator_if);
  COMPILE("http://json-schema.org/draft-07/schema#", "then",
          compiler_draft7_applicator_then);
  COMPILE("http://json-schema.org/draft-07/schema#", "else",
          compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE("http://json-schema.org/draft-07/schema#", "type",
          compiler_draft6_validation_type);
  COMPILE("http://json-schema.org/draft-07/schema#", "const",
          compiler_draft6_validation_const);
  COMPILE("http://json-schema.org/draft-07/schema#", "contains",
          compiler_draft6_applicator_contains);
  COMPILE("http://json-schema.org/draft-07/schema#", "propertyNames",
          compiler_draft6_validation_propertynames);
  COMPILE("http://json-schema.org/draft-07/schema#", "exclusiveMaximum",
          compiler_draft6_validation_exclusivemaximum);
  COMPILE("http://json-schema.org/draft-07/schema#", "exclusiveMinimum",
          compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE("http://json-schema.org/draft-07/schema#", "allOf",
          compiler_draft4_applicator_allof);
  COMPILE("http://json-schema.org/draft-07/schema#", "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE("http://json-schema.org/draft-07/schema#", "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE("http://json-schema.org/draft-07/schema#", "not",
          compiler_draft4_applicator_not);
  COMPILE("http://json-schema.org/draft-07/schema#", "enum",
          compiler_draft4_validation_enum);

  COMPILE("http://json-schema.org/draft-07/schema#", "items",
          compiler_draft4_applicator_items);
  COMPILE("http://json-schema.org/draft-07/schema#", "additionalItems",
          compiler_draft4_applicator_additionalitems);
  COMPILE("http://json-schema.org/draft-07/schema#", "uniqueItems",
          compiler_draft4_validation_uniqueitems);
  COMPILE("http://json-schema.org/draft-07/schema#", "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE("http://json-schema.org/draft-07/schema#", "minItems",
          compiler_draft4_validation_minitems);

  COMPILE("http://json-schema.org/draft-07/schema#", "required",
          compiler_draft4_validation_required);
  COMPILE("http://json-schema.org/draft-07/schema#", "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE("http://json-schema.org/draft-07/schema#", "minProperties",
          compiler_draft4_validation_minproperties);
  COMPILE("http://json-schema.org/draft-07/schema#", "properties",
          compiler_draft4_applicator_properties);
  COMPILE("http://json-schema.org/draft-07/schema#", "patternProperties",
          compiler_draft4_applicator_patternproperties);
  COMPILE("http://json-schema.org/draft-07/schema#", "additionalProperties",
          compiler_draft4_applicator_additionalproperties);
  COMPILE("http://json-schema.org/draft-07/schema#", "dependencies",
          compiler_draft4_applicator_dependencies);

  COMPILE("http://json-schema.org/draft-07/schema#", "maximum",
          compiler_draft4_validation_maximum);
  COMPILE("http://json-schema.org/draft-07/schema#", "minimum",
          compiler_draft4_validation_minimum);
  COMPILE("http://json-schema.org/draft-07/schema#", "multipleOf",
          compiler_draft4_validation_multipleof);

  COMPILE("http://json-schema.org/draft-07/schema#", "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE("http://json-schema.org/draft-07/schema#", "minLength",
          compiler_draft4_validation_minlength);
  COMPILE("http://json-schema.org/draft-07/schema#", "pattern",
          compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 6
  // ********************************************

  COMPILE("http://json-schema.org/draft-06/schema#", "$ref",
          compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-06/schema#", "$ref");

  // Any
  COMPILE("http://json-schema.org/draft-06/schema#", "type",
          compiler_draft6_validation_type);
  COMPILE("http://json-schema.org/draft-06/schema#", "const",
          compiler_draft6_validation_const);

  // Array
  COMPILE("http://json-schema.org/draft-06/schema#", "contains",
          compiler_draft6_applicator_contains);

  // Object
  COMPILE("http://json-schema.org/draft-06/schema#", "propertyNames",
          compiler_draft6_validation_propertynames);

  // Number
  COMPILE("http://json-schema.org/draft-06/schema#", "exclusiveMaximum",
          compiler_draft6_validation_exclusivemaximum);
  COMPILE("http://json-schema.org/draft-06/schema#", "exclusiveMinimum",
          compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE("http://json-schema.org/draft-06/schema#", "allOf",
          compiler_draft4_applicator_allof);
  COMPILE("http://json-schema.org/draft-06/schema#", "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE("http://json-schema.org/draft-06/schema#", "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE("http://json-schema.org/draft-06/schema#", "not",
          compiler_draft4_applicator_not);
  COMPILE("http://json-schema.org/draft-06/schema#", "enum",
          compiler_draft4_validation_enum);

  COMPILE("http://json-schema.org/draft-06/schema#", "items",
          compiler_draft4_applicator_items);
  COMPILE("http://json-schema.org/draft-06/schema#", "additionalItems",
          compiler_draft4_applicator_additionalitems);
  COMPILE("http://json-schema.org/draft-06/schema#", "uniqueItems",
          compiler_draft4_validation_uniqueitems);
  COMPILE("http://json-schema.org/draft-06/schema#", "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE("http://json-schema.org/draft-06/schema#", "minItems",
          compiler_draft4_validation_minitems);

  COMPILE("http://json-schema.org/draft-06/schema#", "required",
          compiler_draft4_validation_required);
  COMPILE("http://json-schema.org/draft-06/schema#", "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE("http://json-schema.org/draft-06/schema#", "minProperties",
          compiler_draft4_validation_minproperties);
  COMPILE("http://json-schema.org/draft-06/schema#", "properties",
          compiler_draft4_applicator_properties);
  COMPILE("http://json-schema.org/draft-06/schema#", "patternProperties",
          compiler_draft4_applicator_patternproperties);
  COMPILE("http://json-schema.org/draft-06/schema#", "additionalProperties",
          compiler_draft4_applicator_additionalproperties);
  COMPILE("http://json-schema.org/draft-06/schema#", "dependencies",
          compiler_draft4_applicator_dependencies);

  COMPILE("http://json-schema.org/draft-06/schema#", "maximum",
          compiler_draft4_validation_maximum);
  COMPILE("http://json-schema.org/draft-06/schema#", "minimum",
          compiler_draft4_validation_minimum);
  COMPILE("http://json-schema.org/draft-06/schema#", "multipleOf",
          compiler_draft4_validation_multipleof);

  COMPILE("http://json-schema.org/draft-06/schema#", "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE("http://json-schema.org/draft-06/schema#", "minLength",
          compiler_draft4_validation_minlength);
  COMPILE("http://json-schema.org/draft-06/schema#", "pattern",
          compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 4
  // ********************************************

  COMPILE("http://json-schema.org/draft-04/schema#", "$ref",
          compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-04/schema#", "$ref");

  // Applicators
  COMPILE("http://json-schema.org/draft-04/schema#", "allOf",
          compiler_draft4_applicator_allof);
  COMPILE("http://json-schema.org/draft-04/schema#", "anyOf",
          compiler_draft4_applicator_anyof);
  COMPILE("http://json-schema.org/draft-04/schema#", "oneOf",
          compiler_draft4_applicator_oneof);
  COMPILE("http://json-schema.org/draft-04/schema#", "not",
          compiler_draft4_applicator_not);
  COMPILE("http://json-schema.org/draft-04/schema#", "properties",
          compiler_draft4_applicator_properties);
  COMPILE("http://json-schema.org/draft-04/schema#", "patternProperties",
          compiler_draft4_applicator_patternproperties);
  COMPILE("http://json-schema.org/draft-04/schema#", "additionalProperties",
          compiler_draft4_applicator_additionalproperties);
  COMPILE("http://json-schema.org/draft-04/schema#", "items",
          compiler_draft4_applicator_items);
  COMPILE("http://json-schema.org/draft-04/schema#", "additionalItems",
          compiler_draft4_applicator_additionalitems);
  COMPILE("http://json-schema.org/draft-04/schema#", "dependencies",
          compiler_draft4_applicator_dependencies);

  // Any
  COMPILE("http://json-schema.org/draft-04/schema#", "type",
          compiler_draft4_validation_type);
  COMPILE("http://json-schema.org/draft-04/schema#", "enum",
          compiler_draft4_validation_enum);

  // Object
  COMPILE("http://json-schema.org/draft-04/schema#", "required",
          compiler_draft4_validation_required);
  COMPILE("http://json-schema.org/draft-04/schema#", "maxProperties",
          compiler_draft4_validation_maxproperties);
  COMPILE("http://json-schema.org/draft-04/schema#", "minProperties",
          compiler_draft4_validation_minproperties);

  // Array
  COMPILE("http://json-schema.org/draft-04/schema#", "uniqueItems",
          compiler_draft4_validation_uniqueitems);
  COMPILE("http://json-schema.org/draft-04/schema#", "maxItems",
          compiler_draft4_validation_maxitems);
  COMPILE("http://json-schema.org/draft-04/schema#", "minItems",
          compiler_draft4_validation_minitems);

  // String
  COMPILE("http://json-schema.org/draft-04/schema#", "pattern",
          compiler_draft4_validation_pattern);
  COMPILE("http://json-schema.org/draft-04/schema#", "maxLength",
          compiler_draft4_validation_maxlength);
  COMPILE("http://json-schema.org/draft-04/schema#", "minLength",
          compiler_draft4_validation_minlength);
  COMPILE("http://json-schema.org/draft-04/schema#", "format",
          compiler_draft4_validation_format);

  // Number
  COMPILE("http://json-schema.org/draft-04/schema#", "maximum",
          compiler_draft4_validation_maximum);
  COMPILE("http://json-schema.org/draft-04/schema#", "minimum",
          compiler_draft4_validation_minimum);
  COMPILE("http://json-schema.org/draft-04/schema#", "multipleOf",
          compiler_draft4_validation_multipleof);

#undef COMPILE
#undef STOP_IF_SIBLING_KEYWORD

  if ((schema_context.vocabularies.contains(
           "https://json-schema.org/draft/2019-09/vocab/core") ||
       schema_context.vocabularies.contains(
           "https://json-schema.org/draft/2020-12/vocab/core")) &&
      !dynamic_context.keyword.starts_with('$') &&
      dynamic_context.keyword != "definitions") {

    // We handle these keywords as part of "contains"
    if ((schema_context.vocabularies.contains(
             "https://json-schema.org/draft/2019-09/vocab/validation") ||
         schema_context.vocabularies.contains(
             "https://json-schema.org/draft/2020-12/vocab/validation")) &&
        (dynamic_context.keyword == "minContains" ||
         dynamic_context.keyword == "maxContains")) {
      return {};
    }

    if (context.mode == Mode::FastValidation) {
      return {};
    }

    return internal::compiler_2019_09_core_annotation(context, schema_context,
                                                      dynamic_context);
  }

  return {};
}
