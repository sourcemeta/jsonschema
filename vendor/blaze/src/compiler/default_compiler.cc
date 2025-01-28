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
    const sourcemeta::blaze::DynamicContext &dynamic_context,
    const sourcemeta::blaze::Instructions &current)
    -> sourcemeta::blaze::Instructions {
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
      "https://json-schema.org/draft/2019-09/vocab/hyper-schema",
      "http://json-schema.org/draft-07/schema#",
      "http://json-schema.org/draft-07/hyper-schema#",
      "http://json-schema.org/draft-06/schema#",
      "http://json-schema.org/draft-06/hyper-schema#",
      "http://json-schema.org/draft-04/schema#",
      "http://json-schema.org/draft-04/hyper-schema#"};
  for (const auto &vocabulary : schema_context.vocabularies) {
    if (!SUPPORTED_VOCABULARIES.contains(vocabulary.first) &&
        vocabulary.second) {
      throw sourcemeta::core::SchemaVocabularyError(
          vocabulary.first, "Cannot compile unsupported vocabulary");
    }
  }

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

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "$ref",
              compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-07/schema#", "$ref");
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-07/hyper-schema#",
                          "$ref");

  // Any
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "if",
              compiler_draft7_applicator_if);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "then",
              compiler_draft7_applicator_then);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "else",
              compiler_draft7_applicator_else);

  // Same as Draft 6

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "type",
              compiler_draft6_validation_type);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "const",
              compiler_draft6_validation_const);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "contains",
              compiler_draft6_applicator_contains);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "propertyNames",
              compiler_draft6_validation_propertynames);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#",
              "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#",
              "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "allOf",
              compiler_draft4_applicator_allof);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "anyOf",
              compiler_draft4_applicator_anyof);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "oneOf",
              compiler_draft4_applicator_oneof);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "not",
              compiler_draft4_applicator_not);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "enum",
              compiler_draft4_validation_enum);

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "items",
              compiler_draft4_applicator_items);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#",
              "additionalItems", compiler_draft4_applicator_additionalitems);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "uniqueItems",
              compiler_draft4_validation_uniqueitems);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "maxItems",
              compiler_draft4_validation_maxitems);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "minItems",
              compiler_draft4_validation_minitems);

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "required",
              compiler_draft4_validation_required);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "maxProperties",
              compiler_draft4_validation_maxproperties);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "minProperties",
              compiler_draft4_validation_minproperties);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "properties",
              compiler_draft4_applicator_properties);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#",
              "patternProperties",
              compiler_draft4_applicator_patternproperties);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#",
              "additionalProperties",
              compiler_draft4_applicator_additionalproperties);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "dependencies",
              compiler_draft4_applicator_dependencies);

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "maximum",
              compiler_draft4_validation_maximum);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "minimum",
              compiler_draft4_validation_minimum);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "multipleOf",
              compiler_draft4_validation_multipleof);

  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "maxLength",
              compiler_draft4_validation_maxlength);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "minLength",
              compiler_draft4_validation_minlength);
  COMPILE_ANY("http://json-schema.org/draft-07/schema#",
              "http://json-schema.org/draft-07/hyper-schema#", "pattern",
              compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 6
  // ********************************************

  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "$ref",
              compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-06/schema#", "$ref");
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-06/hyper-schema#",
                          "$ref");

  // Any
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "type",
              compiler_draft6_validation_type);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "const",
              compiler_draft6_validation_const);

  // Array
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "contains",
              compiler_draft6_applicator_contains);

  // Object
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "propertyNames",
              compiler_draft6_validation_propertynames);

  // Number
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#",
              "exclusiveMaximum", compiler_draft6_validation_exclusivemaximum);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#",
              "exclusiveMinimum", compiler_draft6_validation_exclusiveminimum);

  // Same as Draft 4

  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "allOf",
              compiler_draft4_applicator_allof);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "anyOf",
              compiler_draft4_applicator_anyof);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "oneOf",
              compiler_draft4_applicator_oneof);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "not",
              compiler_draft4_applicator_not);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "enum",
              compiler_draft4_validation_enum);

  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "items",
              compiler_draft4_applicator_items);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#",
              "additionalItems", compiler_draft4_applicator_additionalitems);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "uniqueItems",
              compiler_draft4_validation_uniqueitems);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "maxItems",
              compiler_draft4_validation_maxitems);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "minItems",
              compiler_draft4_validation_minitems);

  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "required",
              compiler_draft4_validation_required);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "maxProperties",
              compiler_draft4_validation_maxproperties);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "minProperties",
              compiler_draft4_validation_minproperties);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "properties",
              compiler_draft4_applicator_properties);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#",
              "patternProperties",
              compiler_draft4_applicator_patternproperties);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#",
              "additionalProperties",
              compiler_draft4_applicator_additionalproperties);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "dependencies",
              compiler_draft4_applicator_dependencies);

  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "maximum",
              compiler_draft4_validation_maximum);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "minimum",
              compiler_draft4_validation_minimum);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "multipleOf",
              compiler_draft4_validation_multipleof);

  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "maxLength",
              compiler_draft4_validation_maxlength);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "minLength",
              compiler_draft4_validation_minlength);
  COMPILE_ANY("http://json-schema.org/draft-06/schema#",
              "http://json-schema.org/draft-06/hyper-schema#", "pattern",
              compiler_draft4_validation_pattern);

  // ********************************************
  // DRAFT 4
  // ********************************************

  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "$ref",
              compiler_draft4_core_ref);
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-04/schema#", "$ref");
  STOP_IF_SIBLING_KEYWORD("http://json-schema.org/draft-04/hyper-schema#",
                          "$ref");

  // Applicators
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "allOf",
              compiler_draft4_applicator_allof);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "anyOf",
              compiler_draft4_applicator_anyof);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "oneOf",
              compiler_draft4_applicator_oneof);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "not",
              compiler_draft4_applicator_not);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "properties",
              compiler_draft4_applicator_properties);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#",
              "patternProperties",
              compiler_draft4_applicator_patternproperties);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#",
              "additionalProperties",
              compiler_draft4_applicator_additionalproperties);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "items",
              compiler_draft4_applicator_items);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#",
              "additionalItems", compiler_draft4_applicator_additionalitems);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "dependencies",
              compiler_draft4_applicator_dependencies);

  // Any
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "type",
              compiler_draft4_validation_type);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "enum",
              compiler_draft4_validation_enum);

  // Object
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "required",
              compiler_draft4_validation_required);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "maxProperties",
              compiler_draft4_validation_maxproperties);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "minProperties",
              compiler_draft4_validation_minproperties);

  // Array
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "uniqueItems",
              compiler_draft4_validation_uniqueitems);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "maxItems",
              compiler_draft4_validation_maxitems);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "minItems",
              compiler_draft4_validation_minitems);

  // String
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "pattern",
              compiler_draft4_validation_pattern);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "maxLength",
              compiler_draft4_validation_maxlength);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "minLength",
              compiler_draft4_validation_minlength);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "format",
              compiler_draft4_validation_format);

  // Number
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "maximum",
              compiler_draft4_validation_maximum);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "minimum",
              compiler_draft4_validation_minimum);
  COMPILE_ANY("http://json-schema.org/draft-04/schema#",
              "http://json-schema.org/draft-04/hyper-schema#", "multipleOf",
              compiler_draft4_validation_multipleof);

#undef COMPILE
#undef COMPILE_ANY
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
                                                      dynamic_context, current);
  }

  return {};
}
