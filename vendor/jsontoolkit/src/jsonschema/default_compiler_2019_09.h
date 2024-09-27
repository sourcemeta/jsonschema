#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2019_09_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2019_09_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"
#include "default_compiler_draft4.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_2019_09_applicator_dependentschemas(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  SchemaCompilerTemplate children;

  // To guarantee order
  std::vector<std::string> dependents;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    dependents.push_back(entry.first);
  }
  std::sort(dependents.begin(), dependents.end());

  for (const auto &dependent : dependents) {
    const auto &dependency{
        schema_context.schema.at(dynamic_context.keyword).at(dependent)};
    if (!is_schema(dependency)) {
      continue;
    }

    if (!dependency.is_boolean() || !dependency.to_boolean()) {
      children.push_back(make<SchemaCompilerLogicalWhenDefines>(
          false, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueString{dependent},
          compile(context, schema_context, relative_dynamic_context,
                  {dependent}, empty_pointer)));
    }
  }

  return {make<SchemaCompilerLogicalWhenType>(
      true, context, schema_context, dynamic_context, JSON::Type::Object,
      std::move(children))};
}

auto compiler_2019_09_validation_dependentrequired(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (!schema_context.schema.at(dynamic_context.keyword).is_object()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  SchemaCompilerValueStringMap dependencies;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (!entry.second.is_array()) {
      continue;
    }

    std::vector<JSON::String> properties;
    for (const auto &property : entry.second.as_array()) {
      assert(property.is_string());
      properties.push_back(property.to_string());
    }

    if (!properties.empty()) {
      dependencies.emplace(entry.first, std::move(properties));
    }
  }

  if (dependencies.empty()) {
    return {};
  }

  return {make<SchemaCompilerAssertionPropertyDependencies>(
      true, context, schema_context, dynamic_context, std::move(dependencies))};
}

auto compiler_2019_09_core_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerAnnotationEmit>(
      true, context, schema_context, dynamic_context,
      JSON{schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_2019_09_applicator_contains_conditional_annotate(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  std::size_t minimum{1};
  if (schema_context.schema.defines("minContains")) {
    if (schema_context.schema.at("minContains").is_integer() &&
        schema_context.schema.at("minContains").is_positive()) {
      minimum = static_cast<std::size_t>(
          schema_context.schema.at("minContains").to_integer());
    } else if (schema_context.schema.at("minContains").is_real() &&
               schema_context.schema.at("minContains").is_positive()) {
      minimum = static_cast<std::size_t>(
          schema_context.schema.at("minContains").as_integer());
    }
  }

  std::optional<std::size_t> maximum;
  if (schema_context.schema.defines("maxContains")) {
    if (schema_context.schema.at("maxContains").is_integer() &&
        schema_context.schema.at("maxContains").is_positive()) {
      maximum = schema_context.schema.at("maxContains").to_integer();
    } else if (schema_context.schema.at("maxContains").is_real() &&
               schema_context.schema.at("maxContains").is_positive()) {
      maximum = schema_context.schema.at("maxContains").as_integer();
    }
  }

  if (maximum.has_value() && minimum > maximum.value()) {
    return {make<SchemaCompilerAssertionFail>(true, context, schema_context,
                                              dynamic_context,
                                              SchemaCompilerValueNone{})};
  }

  if (minimum == 0 && !maximum.has_value()) {
    return {};
  }

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};

  if (annotate) {
    children.push_back(make<SchemaCompilerAnnotationBasenameToParent>(
        true, context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{}));

    // TODO: If after emitting the above annotation, the number of annotations
    // for the current schema location + instance location is equal to the
    // array size (which means we annotated all of the items), then emit
    // an annotation "true"
  }

  return {make<SchemaCompilerLoopContains>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueRange{
          minimum, maximum,
          // TODO: We only need to be exhaustive here if `unevaluatedItems` is
          // in use on the schema. Can we pre-determine that and speed things up
          // if not?
          annotate},
      std::move(children))};
}

auto compiler_2019_09_applicator_contains(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_2019_09_applicator_contains_conditional_annotate(
      context, schema_context, dynamic_context, false);
}

auto compiler_2019_09_applicator_additionalproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_additionalproperties_conditional_annotation(
      context, schema_context, dynamic_context, true);
}

auto compiler_2019_09_applicator_items(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_items_conditional_annotation(
      context, schema_context, dynamic_context, true);
}

auto compiler_2019_09_applicator_additionalitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_additionalitems_conditional_annotation(
      context, schema_context, dynamic_context, true);
}

auto compiler_2019_09_applicator_unevaluateditems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};
  children.push_back(make<SchemaCompilerAnnotationToParent>(
      true, context, schema_context, relative_dynamic_context, JSON{true}));

  if (schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/applicator")) {
    return {make<SchemaCompilerLoopItemsUnevaluated>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueItemsAnnotationKeywords{
            "items", {}, {"items", "additionalItems", "unevaluatedItems"}},
        std::move(children))};
  } else if (schema_context.vocabularies.contains(
                 "https://json-schema.org/draft/2020-12/vocab/applicator")) {
    return {make<SchemaCompilerLoopItemsUnevaluated>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueItemsAnnotationKeywords{
            "prefixItems",
            {"contains"},
            {"prefixItems", "items", "contains", "unevaluatedItems"}},
        std::move(children))};
  } else {
    return {make<SchemaCompilerLoopItemsUnmarked>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueStrings{"unevaluatedItems"}, std::move(children))};
  }
}

auto compiler_2019_09_applicator_unevaluatedproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  SchemaCompilerValueStrings dependencies{"unevaluatedProperties"};

  if (schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/applicator")) {
    dependencies.push_back("properties");
    dependencies.push_back("patternProperties");
    dependencies.push_back("additionalProperties");
  }

  if (schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/applicator")) {
    dependencies.push_back("properties");
    dependencies.push_back("patternProperties");
    dependencies.push_back("additionalProperties");
  }

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};
  children.push_back(make<SchemaCompilerAnnotationBasenameToParent>(
      true, context, schema_context, relative_dynamic_context,
      SchemaCompilerValueNone{}));

  return {make<SchemaCompilerLoopPropertiesNoAnnotation>(
      true, context, schema_context, dynamic_context, std::move(dependencies),
      std::move(children))};
}

auto compiler_2019_09_core_recursiveref(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  const auto current{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  assert(context.frame.contains({ReferenceType::Static, current}));
  const auto &entry{context.frame.at({ReferenceType::Static, current})};
  // In this case, just behave as a normal static reference
  if (!context.references.contains({ReferenceType::Dynamic, entry.pointer})) {
    return compiler_draft4_core_ref(context, schema_context, dynamic_context);
  }

  return {make<SchemaCompilerControlDynamicAnchorJump>(
      true, context, schema_context, dynamic_context, "")};
}

auto compiler_2019_09_applicator_anyof(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_anyof_conditional_exhaustive(
      context, schema_context, dynamic_context,
      // TODO: This set to true means that every disjunction of `anyOf`
      // is always evaluated. In fact, we only need to enable this if
      // the schema makes any use of `unevaluatedItems` or
      // `unevaluatedProperties`
      true);
}

auto compiler_2019_09_applicator_properties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_properties_conditional_annotation(
      context, schema_context, dynamic_context, true);
}

auto compiler_2019_09_applicator_patternproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_patternproperties_conditional_annotation(
      context, schema_context, dynamic_context, true);
}

} // namespace internal
#endif
