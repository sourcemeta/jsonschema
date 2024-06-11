#include <sourcemeta/jsontoolkit/jsonschema_compile.h>
#include <sourcemeta/jsontoolkit/jsonschema_error.h>

#include "default_compiler_draft4.h"
#include "default_compiler_draft6.h"
#include "default_compiler_draft7.h"

#include <cassert> // assert
#include <set>     // std::set
#include <sstream> // std::ostringstream
#include <string>  // std::string

// TODO: Support every keyword
auto sourcemeta::jsontoolkit::default_schema_compiler(
    const sourcemeta::jsontoolkit::SchemaCompilerContext &context)
    -> sourcemeta::jsontoolkit::SchemaCompilerTemplate {
  assert(!context.keyword.empty());

  static std::set<std::string> SUPPORTED_VOCABULARIES{
      "http://json-schema.org/draft-07/schema#",
      "http://json-schema.org/draft-06/schema#",
      "http://json-schema.org/draft-04/schema#"};
  for (const auto &vocabulary : context.vocabularies) {
    if (!SUPPORTED_VOCABULARIES.contains(vocabulary.first) &&
        vocabulary.second) {
      std::ostringstream error;
      error << "Cannot compile unsupported vocabulary: " << vocabulary.first;
      throw SchemaError(error.str());
    }
  }

  using namespace sourcemeta::jsontoolkit;

#define COMPILE(vocabulary, _keyword, handler)                                 \
  if (context.vocabularies.contains(vocabulary) &&                             \
      context.keyword == (_keyword)) {                                         \
    return internal::handler(context);                                         \
  }

#define STOP_IF_SIBLING_KEYWORD(vocabulary, _keyword)                          \
  if (context.vocabularies.contains(vocabulary) &&                             \
      context.schema.is_object() && context.schema.defines(_keyword)) {        \
    return {};                                                                 \
  }

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

  // TODO: Collect unknown keywords as annotations starting in 2019-09
  return {};
}
