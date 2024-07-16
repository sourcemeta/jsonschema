#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2019_09_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2019_09_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_2019_09_applicator_dependentschemas(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_validation_dependentrequired(
    const SchemaCompilerContext &,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (!schema_context.schema.at(dynamic_context.keyword).is_object()) {
    return {};
  }

  SchemaCompilerTemplate children;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (!entry.second.is_array()) {
      continue;
    }

    std::set<JSON::String> properties;
    for (const auto &property : entry.second.as_array()) {
      assert(property.is_string());
      properties.emplace(property.to_string());
    }

    children.push_back(make<SchemaCompilerInternalDefinesAll>(
        schema_context, relative_dynamic_context, std::move(properties),
        {make<SchemaCompilerAssertionDefines>(
            schema_context, relative_dynamic_context, entry.first, {},
            SchemaCompilerTargetType::Instance)},
        SchemaCompilerTargetType::Instance));
  }

  return {make<SchemaCompilerLogicalAnd>(
      schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children), type_condition(schema_context, JSON::Type::Object))};
}

auto compiler_2019_09_core_annotation(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_applicator_contains(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_applicator_additionalproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  // Evaluate the subschema against the current property if it
  // was NOT collected as an annotation on either "properties" or
  // "patternProperties"
  SchemaCompilerTemplate conjunctions{
      make<SchemaCompilerInternalNoAnnotation>(
          schema_context, relative_dynamic_context,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"properties"}),

      make<SchemaCompilerInternalNoAnnotation>(
          schema_context, relative_dynamic_context,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"patternProperties"}),
  };

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};

  children.push_back(make<SchemaCompilerAnnotationPublic>(
      schema_context, relative_dynamic_context,
      SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                           empty_pointer},
      {}, SchemaCompilerTargetType::InstanceParent));

  SchemaCompilerTemplate wrapper{make<SchemaCompilerInternalContainer>(
      schema_context, relative_dynamic_context, SchemaCompilerValueNone{},
      std::move(children),
      {make<SchemaCompilerLogicalAnd>(
          schema_context, relative_dynamic_context, SchemaCompilerValueNone{},
          std::move(conjunctions), SchemaCompilerTemplate{})})};

  return {make<SchemaCompilerLoopProperties>(
      schema_context, dynamic_context, true, {std::move(wrapper)},
      {make<SchemaCompilerAssertionTypeStrict>(
          schema_context, relative_dynamic_context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_2019_09_applicator_items(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_applicator_additionalitems(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_applicator_unevaluateditems(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_applicator_unevaluatedproperties(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

auto compiler_2019_09_core_recursiveref(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

} // namespace internal
#endif
