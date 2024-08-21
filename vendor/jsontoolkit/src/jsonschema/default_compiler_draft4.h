#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert> // assert
#include <regex>   // std::regex
#include <set>     // std::set
#include <utility> // std::move

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft4_core_ref(const SchemaCompilerContext &context,
                              const SchemaCompilerSchemaContext &schema_context,
                              const SchemaCompilerDynamicContext
                                  &dynamic_context) -> SchemaCompilerTemplate {
  // Determine the label
  const auto type{ReferenceType::Static};
  const auto current{keyword_location(schema_context)};
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
        context, schema_context, dynamic_context,
        SchemaCompilerValueUnsignedInteger{label}, {})};
  }

  // TODO: Avoid this copy
  auto new_schema_context{schema_context};

  new_schema_context.labels.insert(label);

  // TODO: It is possible to check framing/referencing information to detect
  // whether a schema will recurse. If not, we can avoid the label wrapper
  // altogether as a minor optimization

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  return {make<SchemaCompilerControlLabel>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{label},
      compile(context, std::move(new_schema_context), relative_dynamic_context,
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
          context, schema_context, dynamic_context, JSON::Type::Null, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer}, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::String, {},
          SchemaCompilerTargetType::Instance)};
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
          context, schema_context, dynamic_context, JSON::Type::Null, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer}, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::String, {},
          SchemaCompilerTargetType::Instance)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::set<JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.emplace(JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.emplace(JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.emplace(JSON::Type::Object);
      } else if (type_string == "array") {
        types.emplace(JSON::Type::Array);
      } else if (type_string == "number") {
        types.emplace(JSON::Type::Integer);
        types.emplace(JSON::Type::Real);
      } else if (type_string == "integer") {
        types.emplace(JSON::Type::Integer);
      } else if (type_string == "string") {
        types.emplace(JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<SchemaCompilerAssertionTypeStrictAny>(
        context, schema_context, dynamic_context, std::move(types), {},
        SchemaCompilerTargetType::Instance)};
  }

  return {};
}

auto compiler_draft4_validation_required(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  } else if (schema_context.schema.at(dynamic_context.keyword).size() > 1) {
    std::set<JSON::String> properties;
    for (const auto &property :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(property.is_string());
      properties.emplace(property.to_string());
    }

    return {make<SchemaCompilerAssertionDefinesAll>(
        context, schema_context, dynamic_context, std::move(properties),
        type_condition(context, schema_context, JSON::Type::Object),
        SchemaCompilerTargetType::Instance)};
  } else {
    assert(
        schema_context.schema.at(dynamic_context.keyword).front().is_string());
    return {make<SchemaCompilerAssertionDefines>(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword).front().to_string(),
        type_condition(context, schema_context, JSON::Type::Object),
        SchemaCompilerTargetType::Instance)};
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
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children), SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_anyof_conditional_exhaustive(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context,
    const bool exhaustive) -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make<SchemaCompilerInternalContainer>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<Pointer::Token::Index>(index)}),
        SchemaCompilerTemplate{}));
  }

  return {make<SchemaCompilerLogicalOr>(
      context, schema_context, dynamic_context, exhaustive,
      std::move(disjunctors), SchemaCompilerTemplate{})};
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
    disjunctors.push_back(make<SchemaCompilerInternalContainer>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<Pointer::Token::Index>(index)}),
        SchemaCompilerTemplate{}));
  }

  return {make<SchemaCompilerLogicalXor>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(disjunctors), SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_properties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  SchemaCompilerTemplate children;
  for (auto &[key, subschema] :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    auto substeps{compile(context, schema_context, relative_dynamic_context,
                          {key}, {key})};
    substeps.push_back(make<SchemaCompilerAnnotationPublic>(
        context, schema_context, relative_dynamic_context, JSON{key}, {},
        SchemaCompilerTargetType::Instance));
    children.push_back(make<SchemaCompilerInternalContainer>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerValueNone{}, std::move(substeps),
        // TODO: As an optimization, avoid this condition if the subschema
        // declares `required` and includes the given key
        {make<SchemaCompilerAssertionDefines>(
            context, schema_context, relative_dynamic_context, key, {},
            SchemaCompilerTargetType::Instance)}));
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children), SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_patternproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  SchemaCompilerTemplate children;

  // For each regular expression and corresponding subschema in the object
  for (auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    auto substeps{compile(context, schema_context, relative_dynamic_context,
                          {entry.first}, {})};

    // TODO: As an optimization, only emit an annotation if
    // `additionalProperties` is also declared in the same subschema

    // The evaluator will make sure the same annotation is not reported twice.
    // For example, if the same property matches more than one subschema in
    // `patternProperties`
    substeps.push_back(make<SchemaCompilerAnnotationPublic>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                             empty_pointer},
        {}, SchemaCompilerTargetType::InstanceParent));

    // The instance property matches the schema property regex
    SchemaCompilerTemplate loop_condition{make<SchemaCompilerAssertionRegex>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerValueRegex{
            std::regex{entry.first, std::regex::ECMAScript}, entry.first},
        {}, SchemaCompilerTargetType::InstanceBasename)};

    // Loop over the instance properties
    children.push_back(make<SchemaCompilerLoopProperties>(
        context, schema_context, relative_dynamic_context,
        // Treat this as an internal step
        false,
        {make<SchemaCompilerInternalContainer>(
            context, schema_context, relative_dynamic_context,
            SchemaCompilerValueNone{}, std::move(substeps),
            std::move(loop_condition))},
        SchemaCompilerTemplate{}));
  }

  // If the instance is an object...
  return {make<SchemaCompilerLogicalAnd>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, relative_dynamic_context, JSON::Type::Object,
          {}, SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_additionalproperties_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context,
    const bool annotate) -> SchemaCompilerTemplate {
  // Evaluate the subschema against the current property if it
  // was NOT collected as an annotation on either "properties" or
  // "patternProperties"
  SchemaCompilerTemplate conjunctions{
      make<SchemaCompilerInternalNoAdjacentAnnotation>(
          context, schema_context, relative_dynamic_context,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"properties"}),

      make<SchemaCompilerInternalNoAdjacentAnnotation>(
          context, schema_context, relative_dynamic_context,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"patternProperties"}),
  };

  SchemaCompilerTemplate children{compile(context, schema_context,
                                          relative_dynamic_context,
                                          empty_pointer, empty_pointer)};

  if (annotate) {
    children.push_back(make<SchemaCompilerAnnotationPublic>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                             empty_pointer},
        {}, SchemaCompilerTargetType::InstanceParent));
  }

  SchemaCompilerTemplate wrapper{make<SchemaCompilerInternalContainer>(
      context, schema_context, relative_dynamic_context,
      SchemaCompilerValueNone{}, std::move(children),
      {make<SchemaCompilerLogicalAnd>(
          context, schema_context, relative_dynamic_context,
          SchemaCompilerValueNone{}, std::move(conjunctions),
          SchemaCompilerTemplate{})})};

  return {make<SchemaCompilerLoopProperties>(
      context, schema_context, dynamic_context, true, {std::move(wrapper)},
      {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, relative_dynamic_context, JSON::Type::Object,
          {}, SchemaCompilerTargetType::Instance)})};
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
  const auto &regex_string{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  return {make<SchemaCompilerAssertionRegex>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueRegex{std::regex{regex_string, std::regex::ECMAScript},
                               regex_string},
      type_condition(context, schema_context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_format(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (!schema_context.schema.at(dynamic_context.keyword).is_string()) {
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
        context, schema_context, dynamic_context,
        SchemaCompilerValueStringType::URI,
        type_condition(context, schema_context, JSON::Type::String),
        SchemaCompilerTargetType::Instance)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {make<SchemaCompilerAssertionRegex>(                                \
        context, schema_context, dynamic_context,                              \
        SchemaCompilerValueRegex{                                              \
            std::regex{(regular_expression), std::regex::ECMAScript},          \
            (regular_expression)},                                             \
        type_condition(context, schema_context, JSON::Type::String),           \
        SchemaCompilerTargetType::Instance)};                                  \
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
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer),
      SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_items_array(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context,
    const bool annotate) -> SchemaCompilerTemplate {
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
      if (annotate) {
        subchildren.push_back(make<SchemaCompilerAnnotationPublic>(
            context, schema_context, relative_dynamic_context, JSON{true},
            {make<SchemaCompilerInternalSizeEqual>(
                context, schema_context, relative_dynamic_context, cursor, {},
                SchemaCompilerTargetType::Instance)},
            SchemaCompilerTargetType::Instance));
        subchildren.push_back(make<SchemaCompilerAnnotationPublic>(
            context, schema_context, relative_dynamic_context, JSON{cursor - 1},
            {make<SchemaCompilerAssertionSizeGreater>(
                context, schema_context, relative_dynamic_context, cursor, {},
                SchemaCompilerTargetType::Instance)},
            SchemaCompilerTargetType::Instance));
      }

      children.push_back(make<SchemaCompilerInternalContainer>(
          context, schema_context, relative_dynamic_context,
          SchemaCompilerValueNone{}, std::move(subchildren),
          {make<SchemaCompilerAssertionSizeGreater>(
              context, schema_context, relative_dynamic_context, cursor - 1, {},
              SchemaCompilerTargetType::Instance)}));
    } else {
      if (annotate) {
        subchildren.push_back(make<SchemaCompilerAnnotationPublic>(
            context, schema_context, relative_dynamic_context, JSON{cursor - 1},
            {}, SchemaCompilerTargetType::Instance));
      }

      children.push_back(make<SchemaCompilerInternalContainer>(
          context, schema_context, relative_dynamic_context,
          SchemaCompilerValueNone{}, std::move(subchildren),
          {make<SchemaCompilerInternalSizeEqual>(
              context, schema_context, relative_dynamic_context, cursor, {},
              SchemaCompilerTargetType::Instance)}));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children),
      {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, relative_dynamic_context, JSON::Type::Array,
          {}, SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_items_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context,
    const bool annotate) -> SchemaCompilerTemplate {
  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    SchemaCompilerTemplate children;
    children.push_back(make<SchemaCompilerLoopItems>(
        context, schema_context, relative_dynamic_context,
        SchemaCompilerValueUnsignedInteger{0},
        compile(context, schema_context, relative_dynamic_context,
                empty_pointer, empty_pointer),
        SchemaCompilerTemplate{}));

    if (annotate) {
      children.push_back(make<SchemaCompilerAnnotationPublic>(
          context, schema_context, relative_dynamic_context, JSON{true}, {},
          SchemaCompilerTargetType::Instance));
    }

    return {make<SchemaCompilerInternalContainer>(
        context, schema_context, dynamic_context, SchemaCompilerValueNone{},
        std::move(children),
        {make<SchemaCompilerAssertionTypeStrict>(
            context, schema_context, relative_dynamic_context,
            JSON::Type::Array, {}, SchemaCompilerTargetType::Instance)})};
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
  SchemaCompilerTemplate condition{make<SchemaCompilerAssertionTypeStrict>(
      context, schema_context, relative_dynamic_context, JSON::Type::Array, {},
      SchemaCompilerTargetType::Instance)};
  condition.push_back(make<SchemaCompilerAssertionSizeGreater>(
      context, schema_context, relative_dynamic_context, cursor, {},
      SchemaCompilerTargetType::Instance));

  SchemaCompilerTemplate children{make<SchemaCompilerLoopItems>(
      context, schema_context, relative_dynamic_context,
      SchemaCompilerValueUnsignedInteger{cursor},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer),
      SchemaCompilerTemplate{})};

  if (annotate) {
    children.push_back(make<SchemaCompilerAnnotationPublic>(
        context, schema_context, relative_dynamic_context, JSON{true}, {},
        SchemaCompilerTargetType::Instance));
  }

  return {make<SchemaCompilerInternalContainer>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children), std::move(condition))};
}

auto compiler_draft4_applicator_additionalitems_conditional_annotation(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context,
    const bool annotate) -> SchemaCompilerTemplate {
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
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  SchemaCompilerTemplate children;

  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (is_schema(entry.second)) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(make<SchemaCompilerInternalContainer>(
            context, schema_context, relative_dynamic_context,
            SchemaCompilerValueNone{},
            compile(context, schema_context, relative_dynamic_context,
                    {entry.first}, empty_pointer),

            // TODO: As an optimization, avoid this condition if the subschema
            // declares `required` and includes the given key
            {make<SchemaCompilerAssertionDefines>(
                context, schema_context, relative_dynamic_context, entry.first,
                {}, SchemaCompilerTargetType::Instance)}));
      }
    } else if (entry.second.is_array()) {
      std::set<JSON::String> properties;
      for (const auto &property : entry.second.as_array()) {
        assert(property.is_string());
        properties.emplace(property.to_string());
      }

      children.push_back(make<SchemaCompilerInternalDefinesAll>(
          context, schema_context, relative_dynamic_context,
          std::move(properties),
          // TODO: As an optimization, avoid this condition if the subschema
          // declares `required` and includes the given key
          {make<SchemaCompilerAssertionDefines>(
              context, schema_context, relative_dynamic_context, entry.first,
              {}, SchemaCompilerTargetType::Instance)},
          SchemaCompilerTargetType::Instance));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      std::move(children),
      type_condition(context, schema_context, JSON::Type::Object))};
}

auto compiler_draft4_validation_enum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.at(dynamic_context.keyword).size() == 1) {
    return {make<SchemaCompilerAssertionEqual>(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword).front(), {},
        SchemaCompilerTargetType::Instance)};
  }

  std::set<JSON> options;
  for (const auto &option :
       schema_context.schema.at(dynamic_context.keyword).as_array()) {
    options.insert(option);
  }

  return {make<SchemaCompilerAssertionEqualsAny>(
      context, schema_context, dynamic_context, std::move(options),
      SchemaCompilerTemplate{}, SchemaCompilerTargetType::Instance)};
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

  return {make<SchemaCompilerAssertionUnique>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      type_condition(context, schema_context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxlength(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, if `minLength` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1},
      type_condition(context, schema_context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minlength(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, if `maxLength` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1},
      type_condition(context, schema_context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, if `minItems` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1},
      type_condition(context, schema_context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minitems(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, if `maxItems` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1},
      type_condition(context, schema_context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, if `minProperties` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1},
      type_condition(context, schema_context, JSON::Type::Object),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minproperties(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, if `maxProperties` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1},
      type_condition(context, schema_context, JSON::Type::Object),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maximum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      context, schema_context, relative_dynamic_context, false,
      {make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context, JSON::Type::Real,
           {}, SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context,
           JSON::Type::Integer, {}, SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  // TODO: As an optimization, if `minimum` is set to the same number, do
  // a single equality assertion

  assert(schema_context.schema.is_object());
  if (schema_context.schema.defines("exclusiveMaximum") &&
      schema_context.schema.at("exclusiveMaximum").is_boolean() &&
      schema_context.schema.at("exclusiveMaximum").to_boolean()) {
    return {make<SchemaCompilerAssertionLess>(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword), std::move(condition),
        SchemaCompilerTargetType::Instance)};
  } else {
    return {make<SchemaCompilerAssertionLessEqual>(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword), std::move(condition),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_validation_minimum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      context, schema_context, relative_dynamic_context, false,
      {make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context, JSON::Type::Real,
           {}, SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context,
           JSON::Type::Integer, {}, SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  // TODO: As an optimization, if `maximum` is set to the same number, do
  // a single equality assertion

  assert(schema_context.schema.is_object());
  if (schema_context.schema.defines("exclusiveMinimum") &&
      schema_context.schema.at("exclusiveMinimum").is_boolean() &&
      schema_context.schema.at("exclusiveMinimum").to_boolean()) {
    return {make<SchemaCompilerAssertionGreater>(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword), std::move(condition),
        SchemaCompilerTargetType::Instance)};
  } else {
    return {make<SchemaCompilerAssertionGreaterEqual>(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword), std::move(condition),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_validation_multipleof(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      context, schema_context, relative_dynamic_context, false,
      {make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context, JSON::Type::Real,
           {}, SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context,
           JSON::Type::Integer, {}, SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionDivisible>(
      context, schema_context, dynamic_context,
      schema_context.schema.at(dynamic_context.keyword), std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

} // namespace internal
#endif
