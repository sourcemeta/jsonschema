#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert> // assert
#include <regex>   // std::regex
#include <utility> // std::move

#include "compile_helpers.h"

namespace {

auto type_string_to_assertion(
    const sourcemeta::jsontoolkit::SchemaCompilerContext &context,
    const std::string &type)
    -> sourcemeta::jsontoolkit::SchemaCompilerTemplate {
  using namespace sourcemeta::jsontoolkit;
  if (type == "null") {
    return {make<SchemaCompilerAssertionType>(
        context, JSON::Type::Null, {}, SchemaCompilerTargetType::Instance)};
  } else if (type == "boolean") {
    return {make<SchemaCompilerAssertionType>(
        context, JSON::Type::Boolean, {}, SchemaCompilerTargetType::Instance)};
  } else if (type == "object") {
    return {make<SchemaCompilerAssertionType>(
        context, JSON::Type::Object, {}, SchemaCompilerTargetType::Instance)};
  } else if (type == "array") {
    return {make<SchemaCompilerAssertionType>(
        context, JSON::Type::Array, {}, SchemaCompilerTargetType::Instance)};
  } else if (type == "number") {
    const auto subcontext{applicate(context)};
    return {make<SchemaCompilerLogicalOr>(
        context, SchemaCompilerValueNone{},
        {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Real, {},
                                           SchemaCompilerTargetType::Instance),
         make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Integer, {},
                                           SchemaCompilerTargetType::Instance)},
        SchemaCompilerTemplate{})};
  } else if (type == "integer") {
    return {make<SchemaCompilerAssertionType>(
        context, JSON::Type::Integer, {}, SchemaCompilerTargetType::Instance)};
  } else if (type == "string") {
    return {make<SchemaCompilerAssertionType>(
        context, JSON::Type::String, {}, SchemaCompilerTargetType::Instance)};
  } else {
    return {};
  }
}

} // namespace

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft4_core_ref(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  // Determine the label
  const auto type{ReferenceType::Static};
  const auto current{keyword_location(context)};
  assert(context.frame.contains({type, current}));
  const auto &entry{context.frame.at({type, current})};
  assert(context.references.contains({type, entry.pointer}));
  const auto &reference{context.references.at({type, entry.pointer})};
  const auto label{std::hash<std::string>{}(reference.destination)};

  // The label is already registered, so just jump to it
  if (context.labels.contains(label)) {
    return {make<SchemaCompilerControlJump>(context, label, {})};
  }

  // TODO: Only create a wrapper "label" step if there is indeed recursion
  // going on, which we can probably confirm through the framing results.
  // Otherwise, if no recursion would actually happen, then fully unrolling
  // the references would be more efficient.

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  return {make<SchemaCompilerControlLabel>(context, label,
                                           compile(applicate(context, label),
                                                   empty_pointer, empty_pointer,
                                                   reference.destination))};
}

auto compiler_draft4_validation_type(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  if (context.value.is_string()) {
    return type_string_to_assertion(context, context.value.to_string());
  } else if (context.value.is_array()) {
    assert(!context.value.empty());
    SchemaCompilerTemplate disjunctors;
    const auto subcontext{applicate(context)};
    for (const auto &type : context.value.as_array()) {
      assert(type.is_string());
      SchemaCompilerTemplate disjunctor{
          type_string_to_assertion(subcontext, type.to_string())};
      assert(disjunctor.size() == 1);
      disjunctors.push_back(std::move(disjunctor).front());
    }

    assert(disjunctors.size() == context.value.size());
    return {make<SchemaCompilerLogicalOr>(context, SchemaCompilerValueNone{},
                                          std::move(disjunctors),
                                          SchemaCompilerTemplate{})};
  }

  return {};
}

auto compiler_draft4_validation_required(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  if (context.value.size() > 1) {
    SchemaCompilerTemplate children;
    const auto subcontext{applicate(context)};
    for (const auto &property : context.value.as_array()) {
      assert(property.is_string());
      children.push_back(make<SchemaCompilerAssertionDefines>(
          subcontext, property.to_string(), {},
          SchemaCompilerTargetType::Instance));
    }

    return {make<SchemaCompilerLogicalAnd>(
        context, SchemaCompilerValueNone{}, std::move(children),
        type_condition(context, JSON::Type::Object))};
  } else {
    assert(context.value.front().is_string());
    return {make<SchemaCompilerAssertionDefines>(
        context, context.value.front().to_string(),
        type_condition(context, JSON::Type::Object),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_applicator_allof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  SchemaCompilerTemplate result;
  for (std::uint64_t index = 0; index < context.value.size(); index++) {
    for (auto &&step :
         compile(context, {static_cast<Pointer::Token::Index>(index)})) {
      result.push_back(std::move(step));
    }
  }

  return result;
}

auto compiler_draft4_applicator_anyof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0; index < context.value.size(); index++) {
    disjunctors.push_back(make<SchemaCompilerLogicalAnd>(
        subcontext, SchemaCompilerValueNone{},
        compile(subcontext, {static_cast<Pointer::Token::Index>(index)}),
        SchemaCompilerTemplate{}));
  }

  return {make<SchemaCompilerLogicalOr>(context, SchemaCompilerValueNone{},
                                        std::move(disjunctors),
                                        SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_oneof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());
  assert(!context.value.empty());

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate disjunctors;
  for (std::uint64_t index = 0; index < context.value.size(); index++) {
    disjunctors.push_back(make<SchemaCompilerLogicalAnd>(
        subcontext, SchemaCompilerValueNone{},
        compile(subcontext, {static_cast<Pointer::Token::Index>(index)}),
        SchemaCompilerTemplate{}));
  }

  return {make<SchemaCompilerLogicalXor>(context, SchemaCompilerValueNone{},
                                         std::move(disjunctors),
                                         SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_properties(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_object());
  if (context.value.empty()) {
    return {};
  }

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children;
  for (auto &[key, subschema] : context.value.as_object()) {
    auto substeps{compile(subcontext, {key}, {key})};
    // TODO: As an optimization, only emit an annotation if
    // `additionalProperties` is also declared in the same subschema Annotations
    // as such don't exist in Draft 4, so emit a private annotation instead
    substeps.push_back(make<SchemaCompilerAnnotationPrivate>(
        subcontext, JSON{key}, {}, SchemaCompilerTargetType::Instance));
    children.push_back(make<SchemaCompilerLogicalAnd>(
        subcontext, SchemaCompilerValueNone{}, std::move(substeps),
        // TODO: As an optimization, avoid this condition if the subschema
        // declares `required` and includes the given key
        {make<SchemaCompilerAssertionDefines>(
            subcontext, key, {}, SchemaCompilerTargetType::Instance)}));
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),
      type_condition(context, JSON::Type::Object))};
}

auto compiler_draft4_applicator_patternproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_object());
  if (context.value.empty()) {
    return {};
  }

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children;

  // For each regular expression and corresponding subschema in the object
  for (auto &entry : context.value.as_object()) {
    auto substeps{compile(subcontext, {entry.first}, {})};

    // TODO: As an optimization, only emit an annotation if
    // `additionalProperties` is also declared in the same subschema Annotations
    // as such don't exist in Draft 4, so emit a private annotation instead The
    // evaluator will make sure the same annotation is not reported twice. For
    // example, if the same property matches more than one subschema in
    // `patternProperties`
    substeps.push_back(make<SchemaCompilerAnnotationPrivate>(
        subcontext,
        SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                             empty_pointer},
        {}, SchemaCompilerTargetType::InstanceParent));

    // The instance property matches the schema property regex
    SchemaCompilerTemplate loop_condition{make<SchemaCompilerAssertionRegex>(
        subcontext,
        SchemaCompilerValueRegex{
            std::regex{entry.first, std::regex::ECMAScript}, entry.first},
        {}, SchemaCompilerTargetType::InstanceBasename)};

    // Loop over the instance properties
    children.push_back(make<SchemaCompilerLoopProperties>(
        subcontext, SchemaCompilerValueNone{},
        {make<SchemaCompilerLogicalAnd>(subcontext, SchemaCompilerValueNone{},
                                        std::move(substeps),
                                        std::move(loop_condition))},
        SchemaCompilerTemplate{}));
  }

  // If the instance is an object...
  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Object, {},
                                         SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_additionalproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};

  // Evaluate the subschema against the current property if it
  // was NOT collected as an annotation on either "properties" or
  // "patternProperties"
  SchemaCompilerTemplate conjunctions{

      // TODO: As an optimization, avoid this condition if the subschema does
      // not declare `properties`
      make<SchemaCompilerAssertionNotContains>(
          subcontext,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"properties"}),

      // TODO: As an optimization, avoid this condition if the subschema does
      // not declare `patternProperties`
      make<SchemaCompilerAssertionNotContains>(
          subcontext,
          SchemaCompilerTarget{SchemaCompilerTargetType::InstanceBasename,
                               empty_pointer},
          {}, SchemaCompilerTargetType::ParentAdjacentAnnotations,
          Pointer{"patternProperties"}),
  };

  SchemaCompilerTemplate wrapper{make<SchemaCompilerLogicalAnd>(
      subcontext, SchemaCompilerValueNone{},
      compile(subcontext, empty_pointer, empty_pointer),
      {make<SchemaCompilerLogicalAnd>(subcontext, SchemaCompilerValueNone{},
                                      std::move(conjunctions),
                                      SchemaCompilerTemplate{})})};

  return {make<SchemaCompilerLoopProperties>(
      context, SchemaCompilerValueNone{}, {std::move(wrapper)},

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Object, {},
                                         SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_validation_pattern(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_string());
  const auto &regex_string{context.value.to_string()};
  return {make<SchemaCompilerAssertionRegex>(
      context,
      SchemaCompilerValueRegex{std::regex{regex_string, std::regex::ECMAScript},
                               regex_string},
      type_condition(context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_format(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  if (!context.value.is_string()) {
    return {};
  }

  // Regular expressions

  static const std::string FORMAT_REGEX_IPV4{
      "^(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-"
      "9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-"
      "9][0-9]|[0-9])\\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])$"};

  const auto &format{context.value.to_string()};

  if (format == "uri") {
    return {make<SchemaCompilerAssertionStringType>(
        context, SchemaCompilerValueStringType::URI,
        type_condition(context, JSON::Type::String),
        SchemaCompilerTargetType::Instance)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {make<SchemaCompilerAssertionRegex>(                                \
        context,                                                               \
        SchemaCompilerValueRegex{                                              \
            std::regex{(regular_expression), std::regex::ECMAScript},          \
            (regular_expression)},                                             \
        type_condition(context, JSON::Type::String),                           \
        SchemaCompilerTargetType::Instance)};                                  \
  }

  COMPILE_FORMAT_REGEX("ipv4", FORMAT_REGEX_IPV4)

#undef COMPILE_FORMAT_REGEX

  return {};
}

auto compiler_draft4_applicator_not(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerLogicalNot>(
      context, SchemaCompilerValueNone{},
      compile(applicate(context), empty_pointer, empty_pointer),
      SchemaCompilerTemplate{})};
}

auto compiler_draft4_applicator_items(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};
  if (context.value.is_object()) {
    return {make<SchemaCompilerLoopItems>(
        context, SchemaCompilerValueUnsignedInteger{0},
        compile(subcontext, empty_pointer, empty_pointer),

        // TODO: As an optimization, avoid this condition if the subschema
        // declares `type` to `array` already
        {make<SchemaCompilerAssertionType>(
            subcontext, JSON::Type::Array, {},
            SchemaCompilerTargetType::Instance)})};
  }

  assert(context.value.is_array());
  const auto &array{context.value.as_array()};

  SchemaCompilerTemplate children;
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    const auto index{
        static_cast<std::size_t>(std::distance(array.cbegin(), iterator))};
    children.push_back(make<SchemaCompilerLogicalAnd>(
        subcontext, SchemaCompilerValueNone{},
        compile(subcontext, {index}, {index}),

        // TODO: As an optimization, avoid this condition if the subschema
        // declares a corresponding `minItems`
        {make<SchemaCompilerAssertionSizeGreater>(
            subcontext, index, {}, SchemaCompilerTargetType::Instance)}));
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `array` already
      {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Array, {},
                                         SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_additionalitems(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.schema.is_object());

  // Nothing to do here
  if (!context.schema.defines("items") ||
      context.schema.at("items").is_object()) {
    return {};
  }

  const auto cursor{
      (context.schema.defines("items") && context.schema.at("items").is_array())
          ? context.schema.at("items").size()
          : 0};

  return {make<SchemaCompilerLoopItems>(
      context, SchemaCompilerValueUnsignedInteger{cursor},
      compile(applicate(context), empty_pointer, empty_pointer),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `array` already
      {make<SchemaCompilerAssertionType>(context, JSON::Type::Array, {},
                                         SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft4_applicator_dependencies(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_object());
  SchemaCompilerTemplate children;
  const auto subcontext{applicate(context)};

  for (const auto &entry : context.value.as_object()) {
    if (entry.second.is_object()) {
      children.push_back(make<SchemaCompilerLogicalAnd>(
          subcontext, SchemaCompilerValueNone{},
          compile(subcontext, {entry.first}, empty_pointer),

          // TODO: As an optimization, avoid this condition if the subschema
          // declares `required` and includes the given key
          {make<SchemaCompilerAssertionDefines>(
              subcontext, entry.first, {},
              SchemaCompilerTargetType::Instance)}));
    } else if (entry.second.is_array()) {
      SchemaCompilerTemplate substeps;
      for (const auto &key : entry.second.as_array()) {
        assert(key.is_string());
        substeps.push_back(make<SchemaCompilerAssertionDefines>(
            subcontext, key.to_string(), {},
            SchemaCompilerTargetType::Instance));
      }

      children.push_back(make<SchemaCompilerLogicalAnd>(
          subcontext, SchemaCompilerValueNone{}, std::move(substeps),

          // TODO: As an optimization, avoid this condition if the subschema
          // declares `required` and includes the given key
          {make<SchemaCompilerAssertionDefines>(
              subcontext, entry.first, {},
              SchemaCompilerTargetType::Instance)}));
    }
  }

  return {make<SchemaCompilerLogicalAnd>(
      context, SchemaCompilerValueNone{}, std::move(children),
      type_condition(context, JSON::Type::Object))};
}

auto compiler_draft4_validation_enum(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_array());

  if (context.value.size() == 1) {
    return {
        make<SchemaCompilerAssertionEqual>(context, context.value.front(), {},
                                           SchemaCompilerTargetType::Instance)};
  }

  SchemaCompilerTemplate children;
  const auto subcontext{applicate(context)};
  for (const auto &choice : context.value.as_array()) {
    children.push_back(make<SchemaCompilerAssertionEqual>(
        subcontext, choice, {}, SchemaCompilerTargetType::Instance));
  }

  return {make<SchemaCompilerLogicalOr>(context, SchemaCompilerValueNone{},
                                        std::move(children),
                                        SchemaCompilerTemplate{})};
}

auto compiler_draft4_validation_uniqueitems(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  if (!context.value.is_boolean() || !context.value.to_boolean()) {
    return {};
  }

  return {make<SchemaCompilerAssertionUnique>(
      context, SchemaCompilerValueNone{},
      type_condition(context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxlength(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `minLength` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.to_integer()) + 1},
      type_condition(context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minlength(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `maxLength` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.to_integer()) - 1},
      type_condition(context, JSON::Type::String),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxitems(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `minItems` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.to_integer()) + 1},
      type_condition(context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minitems(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_integer());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `maxItems` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.to_integer()) - 1},
      type_condition(context, JSON::Type::Array),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maxproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_integer());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `minProperties` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeLess>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.to_integer()) + 1},
      type_condition(context, JSON::Type::Object),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_minproperties(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_integer());
  assert(context.value.is_positive());

  // TODO: As an optimization, if `maxProperties` is set to the same number, do
  // a single size equality assertion
  return {make<SchemaCompilerAssertionSizeGreater>(
      context,
      SchemaCompilerValueUnsignedInteger{
          static_cast<unsigned long>(context.value.to_integer()) - 1},
      type_condition(context, JSON::Type::Object),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft4_validation_maximum(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  const auto subcontext{applicate(context)};

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      subcontext, SchemaCompilerValueNone{},
      {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  // TODO: As an optimization, if `minimum` is set to the same number, do
  // a single equality assertion

  assert(context.schema.is_object());
  if (context.schema.defines("exclusiveMaximum") &&
      context.schema.at("exclusiveMaximum").is_boolean() &&
      context.schema.at("exclusiveMaximum").to_boolean()) {
    return {make<SchemaCompilerAssertionLess>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  } else {
    return {make<SchemaCompilerAssertionLessEqual>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_validation_minimum(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  const auto subcontext{applicate(context)};

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      subcontext, SchemaCompilerValueNone{},
      {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  // TODO: As an optimization, if `maximum` is set to the same number, do
  // a single equality assertion

  assert(context.schema.is_object());
  if (context.schema.defines("exclusiveMinimum") &&
      context.schema.at("exclusiveMinimum").is_boolean() &&
      context.schema.at("exclusiveMinimum").to_boolean()) {
    return {make<SchemaCompilerAssertionGreater>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  } else {
    return {make<SchemaCompilerAssertionGreaterEqual>(
        context, context.value, std::move(condition),
        SchemaCompilerTargetType::Instance)};
  }
}

auto compiler_draft4_validation_multipleof(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  assert(context.value.is_positive());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      subcontext, SchemaCompilerValueNone{},
      {make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionType>(subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionDivisible>(
      context, context.value, std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

} // namespace internal
#endif
