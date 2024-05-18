#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "default_compiler_2020_12.h"
#include "default_compiler_draft4.h"
#include "default_compiler_draft6.h"

#include <cassert> // assert

// TODO: Support every keyword
auto sourcemeta::jsontoolkit::default_schema_compiler(
    const sourcemeta::jsontoolkit::SchemaCompilerContext &context)
    -> sourcemeta::jsontoolkit::SchemaCompilerTemplate {
  assert(!context.keyword.empty());
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
  // 2020-12
  // ********************************************

  COMPILE("https://json-schema.org/draft/2020-12/vocab/validation", "type",
          compiler_2020_12_validation_type);

  // ********************************************
  // DRAFT 6
  // ********************************************

  COMPILE("http://json-schema.org/draft-06/schema#", "const",
          compiler_draft6_validation_const);

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
