#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <algorithm> // std::sort, std::any_of
#include <cassert>   // assert
#include <regex>     // std::regex
#include <set>       // std::set
#include <utility>   // std::move

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft4_core_ref(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  // Determine the label
  const auto type{ReferenceType::Static};
  const auto current{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  assert(context.frame.contains({type, current}));
  const auto &entry{context.frame.at({type, current})};
  if (!context.references.contains({type, entry.pointer})) {
    assert(schema_context.schema.at(dynamic_context.keyword).is_string());
    throw SchemaReferenceError(
        schema_context.schema.at(dynamic_context.keyword).to_string(),
        entry.pointer, "The schema location is inside of an unknown keyword");
  }

  const auto &reference{context.references.at({type, entry.pointer})};
  const auto label{std::hash<std::string>{}(reference.destination)};

  // The label is already registered, so just jump to it
  if (schema_context.labels.contains(label)) {
    return {make<SchemaCompilerControlJump>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueUnsignedInteger{label})};
  }

  auto new_schema_context{schema_context};
  new_schema_context.references.insert(reference.destination);

  // If the reference is not a recursive one, we can avoid the extra
  // overhead of marking the location for future jumps, and pretty much
  // just expand the reference destination in place.
  const bool is_recursive{
      // This means the reference is directly recursive, by jumping to
      // a parent of the reference itself.
      (context.frame.contains({type, reference.destination}) &&
       entry.pointer.starts_with(
           context.frame.at({type, reference.destination}).pointer)) ||
      schema_context.references.contains(reference.destination)};
  if (!is_recursive) {
    // TODO: Enable this optimization for 2019-09 on-wards
    if (schema_context.vocabularies.contains(
            "http://json-schema.org/draft-04/schema#") ||
        schema_context.vocabularies.contains(
            "http://json-schema.org/draft-06/schema#") ||
        schema_context.vocabularies.contains(
            "http://json-schema.org/draft-07/schema#")) {
      return compile(context, new_schema_context, dynamic_context,
                     empty_pointer, empty_pointer, reference.destination);
    } else {
      return {make<SchemaCompilerLogicalAnd>(
          true, context, schema_context, dynamic_context,
          SchemaCompilerValueNone{},
          compile(context, new_schema_context, relative_dynamic_context,
                  empty_pointer, empty_pointer, reference.destination))};
    }
  }

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  new_schema_context.labels.insert(label);
  return {make<SchemaCompilerControlLabel>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{label},
      compile(context, new_schema_context, relative_dynamic_context,
              empty_pointer, empty_pointer, reference.destination))};
}

auto compiler_draft4_validation_type(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<SchemaCompilerAssertionTypeObjectBounded>(
            true, context, schema_context, dynamic_context,
            {minimum, maximum, false})};
      }

      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<SchemaCompilerAssertionTypeArrayBounded>(
            true, context, schema_context, dynamic_context,
            {minimum, maximum, false})};
      }

      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Array)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<JSON::Type>{JSON::Type::Real, JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<SchemaCompilerAssertionTypeStringBounded>(
            true, context, schema_context, dynamic_context,
            {minimum, maximum, false})};
      }

      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array() &&
             schema_context.schema.at(dynamic_context.keyword).size() == 1 &&
             schema_context.schema.at(dynamic_context.keyword)
                 .front()
                 .is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).front().to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Object)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Array)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<JSON::Type>{JSON::Type::Real, JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Integer)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(JSON::Type::Integer);
        types.push_back(JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(JSON::Type::Integer);
      } else if (type_string == "string") {
        types.push_back(JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<SchemaCompilerAssertionTypeStrictAny>(
        true, context, schema_context, dynamic_context, std::move(types))};
  }

  return {};
}

auto compiler_draft4_validation_required(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  } else if (schema_context.schema.at(dynamic_context.keyword).size() > 1) {
    std::vector<JSON::String> properties;
    for (const auto &property :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(property.is_string());
      properties.push_back(property.to_string());
    }

    if (properties.size() == 1) {
      return {make<SchemaCompilerAssertionDefines>(
          true, context, schema_context, dynamic_context,
          SchemaCompilerValueString{*(properties.cbegin())})};
    } else {
      return {make<SchemaCompilerAssertionDefinesAll>(
          true, context, schema_context, dynamic_context,
          std::move(properties))};
    }
  } else {
    assert(
        schema_context.schema.at(dynamic_context.keyword).front().is_string());
    return {make<SchemaCompilerAssertionDefines>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueString{
            schema_context.schema.at(dynamic_context.keyword)
                .front()
                .to_string()})};
  }
}

auto compiler_draft4_applicator_allof(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  SchemaCompilerTemplate children;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    for (auto &&step :
         compile(context, schema_context, relative_dynamic_context,
                 {static_cast<Pointer::Token::Index>(index)})) {
      children.push_back(std::move(step));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      true, context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children))};
}

auto compiler_draft4_applicator_anyof_conditional_exhaustive(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool exhaustive)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make<SchemaCompilerLogicalAnd>(
        false, context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<Pointer::Token::Index>(index)})));
  }

  return {make<SchemaCompilerLogicalOr>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueBoolean{exhaustive}, std::move(disjunctors))};
}

auto compiler_draft4_applicator_anyof(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_anyof_conditional_exhaustive(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_oneof(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make<SchemaCompilerLogicalAnd>(
        false, context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<Pointer::Token::Index>(index)})));
  }

  return {make<SchemaCompilerLogicalXor>(
      true, context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(disjunctors))};
}

auto compiler_draft4_applicator_properties_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  const auto size{schema_context.schema.at(dynamic_context.keyword).size()};
  const auto imports_required_keyword =
      schema_context.vocabularies.contains(
          "http://json-schema.org/draft-04/schema#") ||
      schema_context.vocabularies.contains(
          "http://json-schema.org/draft-06/schema#") ||
      schema_context.vocabularies.contains(
          "http://json-schema.org/draft-07/schema#") ||
      schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/validation") ||
      schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/validation");
  std::set<std::string> required;
  if (imports_required_keyword && schema_context.schema.defines("required") &&
      schema_context.schema.at("required").is_array()) {
    for (const auto &property :
         schema_context.schema.at("required").as_array()) {
      if (property.is_string() &&
          // Only count the required property if its indeed in "properties"
          schema_context.schema.at(dynamic_context.keyword)
              .defines(property.to_string())) {
        required.insert(property.to_string());
      }
    }
  }

  std::size_t is_required = 0;
  std::vector<std::string> properties;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    properties.push_back(entry.first);
    if (required.contains(entry.first)) {
      is_required += 1;
    }
  }

  // To guarantee order
  std::sort(properties.begin(), properties.end());

  // There are two ways to compile `properties` depending on whether
  // most of the properties are marked as required using `required`
  // or whether most of the properties are optional. Each shines
  // in the corresponding case.

  const auto prefer_loop_over_instance{
      // This strategy only makes sense if most of the properties are "optional"
      is_required <= (size / 2) &&
      // If `properties` only defines a relatively small amount of properties,
      // then its probably still faster to unroll
      schema_context.schema.at(dynamic_context.keyword).size() > 5};

  if (prefer_loop_over_instance) {
    SchemaCompilerValueNamedIndexes indexes;
    SchemaCompilerTemplate children;
    std::size_t cursor = 0;

    for (const auto &name : properties) {
      indexes.emplace(name, cursor);
      auto substeps{compile(context, schema_context, relative_dynamic_context,
                            {name}, {name})};

      if (annotate) {
        substeps.push_back(make<SchemaCompilerAnnotationEmit>(
            true, context, schema_context, relative_dynamic_context,
            JSON{name}));
      }

      // Note that the evaluator completely ignores this wrapper anyway
      children.push_back(make<SchemaCompilerLogicalAnd>(
          false, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueNone{}, std::move(substeps)));
      cursor += 1;
    }

    return {make<SchemaCompilerLoopPropertiesMatch>(
        true, context, schema_context, dynamic_context, std::move(indexes),
        std::move(children))};
  }

  SchemaCompilerTemplate children;

  for (const auto &name : properties) {
    auto substeps{compile(context, schema_context, relative_dynamic_context,
                          {name}, {name})};

    if (annotate) {
      substeps.push_back(make<SchemaCompilerAnnotationEmit>(
          true, context, schema_context, relative_dynamic_context, JSON{name}));
    }

    // We can avoid this "defines" condition if the property is a required one
    if (imports_required_keyword && schema_context.schema.defines("required") &&
        schema_context.schema.at("required").is_array() &&
        schema_context.schema.at("required").contains(JSON{name})) {
      // We can avoid the container too and just inline these steps
      for (auto &&substep : substeps) {
        children.push_back(std::move(substep));
      }

      // Optimize `properties` where its subschemas just include a type check,
      // as that's a very common pattern

    } else if (substeps.size() == 1 &&
               std::holds_alternative<SchemaCompilerAssertionTypeStrict>(
                   substeps.front())) {
      const auto &type_step{
          std::get<SchemaCompilerAssertionTypeStrict>(substeps.front())};
      children.push_back(SchemaCompilerAssertionPropertyTypeStrict{
          type_step.relative_schema_location,
          dynamic_context.base_instance_location.concat(
              type_step.relative_instance_location),
          type_step.keyword_location, type_step.schema_resource,
          type_step.dynamic, type_step.report, type_step.value});
    } else if (substeps.size() == 1 &&
               std::holds_alternative<SchemaCompilerAssertionType>(
                   substeps.front())) {
      const auto &type_step{
          std::get<SchemaCompilerAssertionType>(substeps.front())};
      children.push_back(SchemaCompilerAssertionPropertyType{
          type_step.relative_schema_location,
          dynamic_context.base_instance_location.concat(
              type_step.relative_instance_location),
          type_step.keyword_location, type_step.schema_resource,
          type_step.dynamic, type_step.report, type_step.value});
    } else if (substeps.size() == 1 &&
               std::holds_alternative<
                   SchemaCompilerAssertionPropertyTypeStrict>(
                   substeps.front())) {
      children.push_back(unroll<SchemaCompilerAssertionPropertyTypeStrict>(
          relative_dynamic_context, substeps.front(),
          dynamic_context.base_instance_location));
    } else if (substeps.size() == 1 &&
               std::holds_alternative<SchemaCompilerAssertionPropertyType>(
                   substeps.front())) {
      children.push_back(unroll<SchemaCompilerAssertionPropertyType>(
          relative_dynamic_context, substeps.front(),
          dynamic_context.base_instance_location));

    } else {
      children.push_back(make<SchemaCompilerLogicalWhenDefines>(
          false, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueString{name}, std::move(substeps)));
    }
  }

  // Optimize away the wrapper when emitting a single instruction
  if (children.size() == 1 &&
      std::holds_alternative<SchemaCompilerAssertionPropertyTypeStrict>(
          children.front())) {
    return {unroll<SchemaCompilerAssertionPropertyTypeStrict>(
        dynamic_context, children.front())};
  }

  return {make<SchemaCompilerLogicalAnd>(
      true, context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children))};
}

auto compiler_draft4_applicator_properties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_properties_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_patternproperties_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  SchemaCompilerTemplate children;

  // To guarantee ordering
  std::vector<std::string> patterns;
  for (auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    patterns.push_back(entry.first);
  }

  std::sort(patterns.begin(), patterns.end());

  // For each regular expression and corresponding subschema in the object
  for (const auto &pattern : patterns) {
    auto substeps{compile(context, schema_context, relative_dynamic_context,
                          {pattern}, {})};

    if (annotate) {
      // The evaluator will make sure the same annotation is not reported twice.
      // For example, if the same property matches more than one subschema in
      // `patternProperties`
      substeps.push_back(make<SchemaCompilerAnnotationBasenameToParent>(
          true, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueNone{}));
    }

    // If the `patternProperties` subschema for the given pattern does
    // nothing, then we can avoid generating an entire loop for it
    if (!substeps.empty()) {
      // Loop over the instance properties
      children.push_back(make<SchemaCompilerLoopPropertiesRegex>(
          // Treat this as an internal step
          false, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueRegex{std::regex{pattern, std::regex::ECMAScript},
                                   pattern},
          std::move(substeps)));
    }
  }

  if (children.empty()) {
    return {};
  }

  // If the instance is an object...
  return {make<SchemaCompilerLogicalWhenType>(
      true, context, schema_context, dynamic_context, JSON::Type::Object,
      std::move(children))};
}

auto compiler_draft4_applicator_patternproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_patternproperties_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_additionalproperties_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};

  if (annotate) {
    children.push_back(make<SchemaCompilerAnnotationBasenameToParent>(
        true, context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{}));
  }

  SchemaCompilerValuePropertyFilter filter;

  if (schema_context.schema.defines("properties") &&
      schema_context.schema.at("properties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("properties").as_object()) {
      filter.first.insert(entry.first);
    }
  }

  if (schema_context.schema.defines("patternProperties") &&
      schema_context.schema.at("patternProperties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("patternProperties").as_object()) {
      filter.second.push_back(
          {std::regex{entry.first, std::regex::ECMAScript}, entry.first});
    }
  }

  if (!filter.first.empty() || !filter.second.empty()) {
    return {make<SchemaCompilerLoopPropertiesExcept>(
        true, context, schema_context, dynamic_context, std::move(filter),
        std::move(children))};
  } else {
    if (children.size() == 1) {
      // Optimize `additionalProperties` set to just `type`, which is a
      // pretty common pattern
      if (std::holds_alternative<SchemaCompilerAssertionTypeStrict>(
              children.front())) {
        const auto &type_step{
            std::get<SchemaCompilerAssertionTypeStrict>(children.front())};
        return {make<SchemaCompilerLoopPropertiesTypeStrict>(
            true, context, schema_context, dynamic_context, type_step.value)};
      } else if (std::holds_alternative<SchemaCompilerAssertionType>(
                     children.front())) {
        const auto &type_step{
            std::get<SchemaCompilerAssertionType>(children.front())};
        return {make<SchemaCompilerLoopPropertiesType>(
            true, context, schema_context, dynamic_context, type_step.value)};
      }
    }

    return {make<SchemaCompilerLoopProperties>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueNone{}, std::move(children))};
  }
}

auto compiler_draft4_applicator_additionalproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_additionalproperties_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_validation_pattern(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_string());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  const auto &regex_string{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  return {make<SchemaCompilerAssertionRegex>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueRegex{std::regex{regex_string, std::regex::ECMAScript},
                               regex_string})};
}

auto compiler_draft4_validation_format(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (!schema_context.schema.at(dynamic_context.keyword).is_string()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  // Regular expressions

  static const std::string FORMAT_REGEX_IPV4{
      "^(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-"
      "9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-"
      "9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])$"};

  const auto &format{
      schema_context.schema.at(dynamic_context.keyword).to_string()};

  if (format == "uri") {
    return {make<SchemaCompilerAssertionStringType>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueStringType::URI)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {make<SchemaCompilerAssertionRegex>(                                \
        true, context, schema_context, dynamic_context,                        \
        SchemaCompilerValueRegex{                                              \
            std::regex{(regular_expression), std::regex::ECMAScript},          \
            (regular_expression)})};                                           \
  }

  COMPILE_FORMAT_REGEX("ipv4", FORMAT_REGEX_IPV4)

#undef COMPILE_FORMAT_REGEX

  return {};
}

auto compiler_draft4_applicator_not(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerLogicalNot>(
      true, context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};
}

auto compiler_draft4_applicator_items_array(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  const auto items_size{
      schema_context.schema.at(dynamic_context.keyword).size()};
  if (items_size == 0) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
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
      if (annotate) {
        subchildren.push_back(make<SchemaCompilerAnnotationWhenArraySizeEqual>(
            true, context, schema_context, relative_dynamic_context,
            SchemaCompilerValueIndexedJSON{cursor, JSON{true}}));
        subchildren.push_back(
            make<SchemaCompilerAnnotationWhenArraySizeGreater>(
                true, context, schema_context, relative_dynamic_context,
                SchemaCompilerValueIndexedJSON{cursor, JSON{cursor - 1}}));
      }

      children.push_back(make<SchemaCompilerLogicalWhenArraySizeGreater>(
          false, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueUnsignedInteger{cursor - 1},
          std::move(subchildren)));
    } else {
      if (annotate) {
        subchildren.push_back(make<SchemaCompilerAnnotationEmit>(
            true, context, schema_context, relative_dynamic_context,
            JSON{cursor - 1}));
      }

      children.push_back(make<SchemaCompilerLogicalWhenArraySizeEqual>(
          false, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueUnsignedInteger{cursor}, std::move(subchildren)));
    }
  }

  return {make<SchemaCompilerLogicalWhenType>(
      true, context, schema_context, dynamic_context, JSON::Type::Array,
      std::move(children))};
}

auto compiler_draft4_applicator_items_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    if (annotate) {
      SchemaCompilerTemplate children;
      children.push_back(make<SchemaCompilerLoopItems>(
          true, context, schema_context, relative_dynamic_context,
          SchemaCompilerValueUnsignedInteger{0},
          compile(context, schema_context, relative_dynamic_context,
                  empty_pointer, empty_pointer)));
      children.push_back(make<SchemaCompilerAnnotationEmit>(
          true, context, schema_context, relative_dynamic_context, JSON{true}));

      return {make<SchemaCompilerLogicalWhenType>(
          false, context, schema_context, dynamic_context, JSON::Type::Array,
          std::move(children))};
    }

    return {make<SchemaCompilerLoopItems>(
        true, context, schema_context, dynamic_context,
        SchemaCompilerValueUnsignedInteger{0},
        compile(context, schema_context, relative_dynamic_context,
                empty_pointer, empty_pointer))};
  }

  return compiler_draft4_applicator_items_array(context, schema_context,
                                                dynamic_context, annotate);
}

auto compiler_draft4_applicator_items(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_items_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_additionalitems_from_cursor(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context,
    const std::size_t cursor, const bool annotate) -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  SchemaCompilerTemplate children{make<SchemaCompilerLoopItems>(
      true, context, schema_context, relative_dynamic_context,
      SchemaCompilerValueUnsignedInteger{cursor},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};

  if (annotate) {
    children.push_back(make<SchemaCompilerAnnotationEmit>(
        true, context, schema_context, relative_dynamic_context, JSON{true}));
  }

  return {make<SchemaCompilerLogicalWhenArraySizeGreater>(
      false, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{cursor}, std::move(children))};
}

auto compiler_draft4_applicator_additionalitems_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context, const bool annotate)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

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

  return compiler_draft4_applicator_additionalitems_from_cursor(
      context, schema_context, dynamic_context, cursor, annotate);
}

auto compiler_draft4_applicator_additionalitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_applicator_additionalitems_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_dependencies(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  SchemaCompilerTemplate children;
  SchemaCompilerValueStringMap dependencies;

  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (is_schema(entry.second)) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(make<SchemaCompilerLogicalWhenDefines>(
            false, context, schema_context, relative_dynamic_context,
            SchemaCompilerValueString{entry.first},
            compile(context, schema_context, relative_dynamic_context,
                    {entry.first}, empty_pointer)));
      }
    } else if (entry.second.is_array()) {
      std::vector<JSON::String> properties;
      for (const auto &property : entry.second.as_array()) {
        assert(property.is_string());
        properties.push_back(property.to_string());
      }

      if (!properties.empty()) {
        dependencies.emplace(entry.first, std::move(properties));
      }
    }
  }

  if (!dependencies.empty()) {
    children.push_back(make<SchemaCompilerAssertionPropertyDependencies>(
        false, context, schema_context, relative_dynamic_context,
        std::move(dependencies)));
  }

  return {make<SchemaCompilerLogicalWhenType>(
      true, context, schema_context, dynamic_context, JSON::Type::Object,
      std::move(children))};
}

auto compiler_draft4_validation_enum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.at(dynamic_context.keyword).size() == 1) {
    return {make<SchemaCompilerAssertionEqual>(
        true, context, schema_context, dynamic_context,
        JSON{schema_context.schema.at(dynamic_context.keyword).front()})};
  }

  std::vector<JSON> options;
  for (const auto &option :
       schema_context.schema.at(dynamic_context.keyword).as_array()) {
    options.push_back(option);
  }

  return {make<SchemaCompilerAssertionEqualsAny>(
      true, context, schema_context, dynamic_context, std::move(options))};
}

auto compiler_draft4_validation_uniqueitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (!schema_context.schema.at(dynamic_context.keyword).is_boolean() ||
      !schema_context.schema.at(dynamic_context.keyword).to_boolean()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  return {make<SchemaCompilerAssertionUnique>(true, context, schema_context,
                                              dynamic_context,
                                              SchemaCompilerValueNone{})};
}

auto compiler_draft4_validation_maxlength(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "string") {
    return {};
  }

  return {make<SchemaCompilerAssertionStringSizeLess>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minlength(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "string") {
    return {};
  }

  return {make<SchemaCompilerAssertionStringSizeGreater>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maxitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "array") {
    return {};
  }

  return {make<SchemaCompilerAssertionArraySizeLess>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "array") {
    return {};
  }

  return {make<SchemaCompilerAssertionArraySizeGreater>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maxproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "object") {
    return {};
  }

  return {make<SchemaCompilerAssertionObjectSizeLess>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "object") {
    return {};
  }

  return {make<SchemaCompilerAssertionObjectSizeGreater>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maximum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  // TODO: As an optimization, if `minimum` is set to the same number, do
  // a single equality assertion

  assert(schema_context.schema.is_object());
  if (schema_context.schema.defines("exclusiveMaximum") &&
      schema_context.schema.at("exclusiveMaximum").is_boolean() &&
      schema_context.schema.at("exclusiveMaximum").to_boolean()) {
    return {make<SchemaCompilerAssertionLess>(
        true, context, schema_context, dynamic_context,
        JSON{schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make<SchemaCompilerAssertionLessEqual>(
        true, context, schema_context, dynamic_context,
        JSON{schema_context.schema.at(dynamic_context.keyword)})};
  }
}

auto compiler_draft4_validation_minimum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  // TODO: As an optimization, if `maximum` is set to the same number, do
  // a single equality assertion

  assert(schema_context.schema.is_object());
  if (schema_context.schema.defines("exclusiveMinimum") &&
      schema_context.schema.at("exclusiveMinimum").is_boolean() &&
      schema_context.schema.at("exclusiveMinimum").to_boolean()) {
    return {make<SchemaCompilerAssertionGreater>(
        true, context, schema_context, dynamic_context,
        JSON{schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make<SchemaCompilerAssertionGreaterEqual>(
        true, context, schema_context, dynamic_context,
        JSON{schema_context.schema.at(dynamic_context.keyword)})};
  }
}

auto compiler_draft4_validation_multipleof(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<SchemaCompilerAssertionDivisible>(
      true, context, schema_context, dynamic_context,
      JSON{schema_context.schema.at(dynamic_context.keyword)})};
}

} // namespace internal
#endif
