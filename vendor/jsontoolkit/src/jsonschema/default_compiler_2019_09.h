#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2019_09_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2019_09_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_2019_09_applicator_dependentschemas(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  SchemaCompilerTemplate children;

  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (!is_schema(entry.second)) {
      continue;
    }

    if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
      children.push_back(make<SchemaCompilerInternalContainer>(
          schema_context, relative_dynamic_context, SchemaCompilerValueNone{},
          compile(context, schema_context, relative_dynamic_context,
                  {entry.first}, empty_pointer),
          {make<SchemaCompilerAssertionDefines>(
              schema_context, relative_dynamic_context, entry.first, {},
              SchemaCompilerTargetType::Instance)}));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children), type_condition(schema_context, JSON::Type::Object))};
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
    const SchemaCompilerContext &,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerAnnotationPublic>(
      schema_context, dynamic_context,
      schema_context.schema.at(dynamic_context.keyword), {},
      SchemaCompilerTargetType::Instance)};
}

auto compiler_2019_09_applicator_contains(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {

  std::size_t minimum{1};
  if (schema_context.schema.defines("minContains") &&
      schema_context.schema.at("minContains").is_integer() &&
      schema_context.schema.at("minContains").is_positive() &&
      schema_context.schema.at("minContains").to_integer() > 0) {
    minimum = static_cast<std::size_t>(
        schema_context.schema.at("minContains").to_integer());
  }

  std::optional<std::size_t> maximum;
  if (schema_context.schema.defines("maxContains") &&
      schema_context.schema.at("maxContains").is_integer() &&
      schema_context.schema.at("maxContains").is_positive()) {
    maximum = schema_context.schema.at("maxContains").to_integer();
  }

  if (maximum.has_value() && minimum > maximum.value()) {
    return {make<SchemaCompilerAssertionFail>(
        schema_context, dynamic_context, SchemaCompilerValueNone{}, {},
        SchemaCompilerTargetType::Instance)};
  }

  return {make<SchemaCompilerLoopContains>(
      schema_context, dynamic_context,
      SchemaCompilerValueRange{minimum, maximum},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer),
      {make<SchemaCompilerAssertionTypeStrict>(
          schema_context, relative_dynamic_context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)})};
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
      make<SchemaCompilerInternalNoAdjacentAnnotation>(
          schema_context, relative_dynamic_context,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"properties"}),

      make<SchemaCompilerInternalNoAdjacentAnnotation>(
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
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    SchemaCompilerTemplate children;
    children.push_back(make<SchemaCompilerLoopItems>(
        schema_context, relative_dynamic_context,
        SchemaCompilerValueUnsignedInteger{0},
        compile(context, schema_context, relative_dynamic_context,
                empty_pointer, empty_pointer),
        SchemaCompilerTemplate{}));
    children.push_back(make<SchemaCompilerAnnotationPublic>(
        schema_context, relative_dynamic_context, JSON{true}, {},
        SchemaCompilerTargetType::Instance));

    return {make<SchemaCompilerInternalContainer>(
        schema_context, dynamic_context, SchemaCompilerValueNone{},
        std::move(children),
        {make<SchemaCompilerAssertionTypeStrict>(
            schema_context, relative_dynamic_context, JSON::Type::Array, {},
            SchemaCompilerTargetType::Instance)})};
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  const auto items_size{
      schema_context.schema.at(dynamic_context.keyword).size()};
  if (items_size == 0) {
    return {};
  }

  // The idea here is to precompile all possibilities depending on the size
  // of the instance array up to the size of the `items` keyword array.
  // For example, if `items` is set to `[ {}, {}, {} ]`, we create 3
  // conjunctions:
  // - [ {}, {}, {} ] if the instance array size is >= 3
  // - [ {}, {} ] if the instance array size is == 2
  // - [ {} ] if the instance array size is == 1

  // Precompile subschemas
  std::vector<SchemaCompilerTemplate> subschemas;
  subschemas.reserve(items_size);
  const auto &array{
      schema_context.schema.at(dynamic_context.keyword).as_array()};
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    subschemas.push_back(compile(context, schema_context,
                                 relative_dynamic_context, {subschemas.size()},
                                 {subschemas.size()}));
  }

  SchemaCompilerTemplate children;
  for (std::size_t cursor = items_size; cursor > 0; cursor--) {
    SchemaCompilerTemplate subchildren;
    for (std::size_t index = 0; index < cursor; index++) {
      for (const auto &substep : subschemas.at(index)) {
        subchildren.push_back(substep);
      }
    }

    // The first entry
    if (cursor == items_size) {
      subchildren.push_back(make<SchemaCompilerAnnotationPublic>(
          schema_context, relative_dynamic_context, JSON{true},
          {make<SchemaCompilerAssertionSizeEqual>(
              schema_context, relative_dynamic_context, cursor, {},
              SchemaCompilerTargetType::Instance)},
          SchemaCompilerTargetType::Instance));
      subchildren.push_back(make<SchemaCompilerAnnotationPublic>(
          schema_context, relative_dynamic_context, JSON{cursor - 1},
          {make<SchemaCompilerAssertionSizeGreater>(
              schema_context, relative_dynamic_context, cursor, {},
              SchemaCompilerTargetType::Instance)},
          SchemaCompilerTargetType::Instance));

      children.push_back(make<SchemaCompilerInternalContainer>(
          schema_context, relative_dynamic_context, SchemaCompilerValueNone{},
          std::move(subchildren),
          {make<SchemaCompilerAssertionSizeGreater>(
              schema_context, relative_dynamic_context, cursor - 1, {},
              SchemaCompilerTargetType::Instance)}));
    } else {
      subchildren.push_back(make<SchemaCompilerAnnotationPublic>(
          schema_context, relative_dynamic_context, JSON{cursor - 1}, {},
          SchemaCompilerTargetType::Instance));
      children.push_back(make<SchemaCompilerInternalContainer>(
          schema_context, relative_dynamic_context, SchemaCompilerValueNone{},
          std::move(subchildren),
          {make<SchemaCompilerAssertionSizeEqual>(
              schema_context, relative_dynamic_context, cursor, {},
              SchemaCompilerTargetType::Instance)}));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children),
      {make<SchemaCompilerAssertionTypeStrict>(
          schema_context, relative_dynamic_context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_2019_09_applicator_additionalitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.is_object());

  // Nothing to do here
  if (!schema_context.schema.defines("items") ||
      schema_context.schema.at("items").is_object()) {
    return {};
  }

  const auto cursor{(schema_context.schema.defines("items") &&
                     schema_context.schema.at("items").is_array())
                        ? schema_context.schema.at("items").size()
                        : 0};

  SchemaCompilerTemplate condition{make<SchemaCompilerAssertionTypeStrict>(
      schema_context, dynamic_context, JSON::Type::Array, {},
      SchemaCompilerTargetType::Instance)};
  condition.push_back(make<SchemaCompilerAssertionSizeGreater>(
      schema_context, dynamic_context, cursor, {},
      SchemaCompilerTargetType::Instance));

  SchemaCompilerTemplate children{make<SchemaCompilerLoopItems>(
      schema_context, relative_dynamic_context,
      SchemaCompilerValueUnsignedInteger{cursor},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer),
      SchemaCompilerTemplate{})};
  children.push_back(make<SchemaCompilerAnnotationPublic>(
      schema_context, relative_dynamic_context, JSON{true}, {},
      SchemaCompilerTargetType::Instance));

  return {make<SchemaCompilerInternalContainer>(
      schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children), std::move(condition))};
}

auto compiler_2019_09_applicator_unevaluateditems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  SchemaCompilerValueStrings dependencies{"unevaluatedItems"};
  if (schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/applicator")) {
    dependencies.emplace("items");
    dependencies.emplace("additionalItems");
  }

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};
  children.push_back(make<SchemaCompilerAnnotationPublic>(
      schema_context, relative_dynamic_context, JSON{true}, {},
      SchemaCompilerTargetType::InstanceParent));

  SchemaCompilerTemplate loop;
  if (dependencies.contains("items")) {
    loop.push_back(make<SchemaCompilerLoopItemsFromAnnotationIndex>(
        schema_context, relative_dynamic_context,
        SchemaCompilerValueString{"items"}, std::move(children),
        SchemaCompilerTemplate{}));
  } else {
    loop.push_back(make<SchemaCompilerLoopItems>(
        schema_context, relative_dynamic_context,
        SchemaCompilerValueUnsignedInteger{0}, std::move(children),
        SchemaCompilerTemplate{}));
  }

  SchemaCompilerTemplate condition{make<SchemaCompilerAssertionTypeStrict>(
      schema_context, dynamic_context, JSON::Type::Array, {},
      SchemaCompilerTargetType::Instance)};
  condition.push_back(make<SchemaCompilerInternalNoAnnotation>(
      schema_context, relative_dynamic_context, JSON{true}, {},
      SchemaCompilerTargetType::Annotations, std::move(dependencies)));

  return {make<SchemaCompilerInternalContainer>(
      schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(loop), std::move(condition))};
}

auto compiler_2019_09_applicator_unevaluatedproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  SchemaCompilerValueStrings dependencies{"unevaluatedProperties"};
  if (schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/applicator")) {
    dependencies.emplace("properties");
    dependencies.emplace("patternProperties");
    dependencies.emplace("additionalProperties");
  }

  SchemaCompilerTemplate condition{make<SchemaCompilerInternalNoAnnotation>(
      schema_context, relative_dynamic_context,
      SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                           empty_pointer},
      {}, SchemaCompilerTargetType::ParentAnnotations,
      std::move(dependencies))};

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
      std::move(children), std::move(condition))};

  return {make<SchemaCompilerLoopProperties>(
      schema_context, dynamic_context, true, {std::move(wrapper)},
      {make<SchemaCompilerAssertionTypeStrict>(
          schema_context, relative_dynamic_context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_2019_09_core_recursiveref(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate {
  // TODO: Implement
  return {};
}

} // namespace internal
#endif
