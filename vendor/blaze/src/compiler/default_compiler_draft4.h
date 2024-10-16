#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator_context.h>

#include <algorithm> // std::sort, std::any_of
#include <cassert>   // assert
#include <regex>     // std::regex, std::regex_error
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <utility>   // std::move

#include "compile_helpers.h"

static auto parse_regex(const std::string &pattern,
                        const sourcemeta::jsontoolkit::URI &base,
                        const sourcemeta::jsontoolkit::Pointer &schema_location)
    -> std::regex {
  try {
    return std::regex{pattern, std::regex::ECMAScript | std::regex::nosubs};
  } catch (const std::regex_error &) {
    std::ostringstream message;
    message << "Invalid regular expression: " << pattern;
    throw sourcemeta::blaze::CompilerError(base, schema_location,
                                           message.str());
  }
}

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft4_core_ref(const Context &context,
                              const SchemaContext &schema_context,
                              const DynamicContext &dynamic_context)
    -> Template {
  // Determine the label
  const auto type{sourcemeta::jsontoolkit::ReferenceType::Static};
  const auto current{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  assert(context.frame.contains({type, current}));
  const auto &entry{context.frame.at({type, current})};
  if (!context.references.contains({type, entry.pointer})) {
    assert(schema_context.schema.at(dynamic_context.keyword).is_string());
    throw sourcemeta::jsontoolkit::SchemaReferenceError(
        schema_context.schema.at(dynamic_context.keyword).to_string(),
        entry.pointer, "The schema location is inside of an unknown keyword");
  }

  const auto &reference{context.references.at({type, entry.pointer})};
  const auto label{EvaluationContext{}.hash(
      schema_resource_id(context, reference.base.value_or("")),
      reference.fragment.value_or(""))};

  // The label is already registered, so just jump to it
  if (schema_context.labels.contains(label)) {
    return {make<ControlJump>(true, context, schema_context, dynamic_context,
                              ValueUnsignedInteger{label})};
  }

  auto new_schema_context{schema_context};
  new_schema_context.references.insert(reference.destination);

  std::size_t direct_children_references{0};
  if (context.frame.contains({type, reference.destination})) {
    for (const auto &reference_entry : context.references) {
      if (reference_entry.first.second.starts_with(
              context.frame.at({type, reference.destination}).pointer)) {
        direct_children_references += 1;
      }
    }
  }

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

  if (!is_recursive && direct_children_references <= 5) {
    if (context.mode == Mode::FastValidation &&
        // Expanding references inline when dynamic scoping is required
        // may not work, as we might omit the instruction that introduces
        // one of the necessary schema resources to the evaluator
        !context.uses_dynamic_scopes) {
      return compile(context, new_schema_context, dynamic_context,
                     sourcemeta::jsontoolkit::empty_pointer,
                     sourcemeta::jsontoolkit::empty_pointer,
                     reference.destination);
    } else {
      return {make<LogicalAnd>(
          true, context, schema_context, dynamic_context, ValueNone{},
          compile(context, new_schema_context, relative_dynamic_context,
                  sourcemeta::jsontoolkit::empty_pointer,
                  sourcemeta::jsontoolkit::empty_pointer,
                  reference.destination))};
    }
  }

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  new_schema_context.labels.insert(label);
  return {make<ControlLabel>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{label},
      compile(context, new_schema_context, relative_dynamic_context,
              sourcemeta::jsontoolkit::empty_pointer,
              sourcemeta::jsontoolkit::empty_pointer, reference.destination))};
}

auto compiler_draft4_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context)
    -> Template {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeObjectBounded>(true, context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeArrayBounded>(true, context, schema_context,
                                                dynamic_context,
                                                {minimum, maximum, false})};
      }

      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      return {make<AssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeStringBounded>(true, context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::String)};
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
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      return {make<AssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<sourcemeta::jsontoolkit::JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Integer);
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Integer);
      } else if (type_string == "string") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<AssertionTypeStrictAny>(true, context, schema_context,
                                         dynamic_context, std::move(types))};
  }

  return {};
}

auto compiler_draft4_validation_required(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  } else if (schema_context.schema.at(dynamic_context.keyword).size() > 1) {
    std::vector<sourcemeta::jsontoolkit::JSON::String> properties;
    for (const auto &property :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(property.is_string());
      properties.push_back(property.to_string());
    }

    if (properties.size() == 1) {
      return {make<AssertionDefines>(true, context, schema_context,
                                     dynamic_context,
                                     ValueString{*(properties.cbegin())})};
    } else {
      return {make<AssertionDefinesAll>(true, context, schema_context,
                                        dynamic_context,
                                        std::move(properties))};
    }
  } else {
    assert(
        schema_context.schema.at(dynamic_context.keyword).front().is_string());
    return {make<AssertionDefines>(
        true, context, schema_context, dynamic_context,
        ValueString{schema_context.schema.at(dynamic_context.keyword)
                        .front()
                        .to_string()})};
  }
}

auto compiler_draft4_applicator_allof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Template children;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    for (auto &&step :
         compile(context, schema_context, relative_dynamic_context,
                 {static_cast<sourcemeta::jsontoolkit::Pointer::Token::Index>(
                     index)})) {
      children.push_back(std::move(step));
    }
  }

  return {make<LogicalAnd>(true, context, schema_context, dynamic_context,
                           ValueNone{}, std::move(children))};
}

auto compiler_draft4_applicator_anyof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Template disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make<LogicalAnd>(
        false, context, schema_context, relative_dynamic_context, ValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<sourcemeta::jsontoolkit::Pointer::Token::Index>(
                    index)})));
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 context.uses_unevaluated_properties ||
                                 context.uses_unevaluated_items};

  return {make<LogicalOr>(true, context, schema_context, dynamic_context,
                          ValueBoolean{requires_exhaustive},
                          std::move(disjunctors))};
}

auto compiler_draft4_applicator_oneof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Template disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make<LogicalAnd>(
        false, context, schema_context, relative_dynamic_context, ValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<sourcemeta::jsontoolkit::Pointer::Token::Index>(
                    index)})));
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 context.uses_unevaluated_properties ||
                                 context.uses_unevaluated_items};

  return {make<LogicalXor>(true, context, schema_context, dynamic_context,
                           ValueBoolean{requires_exhaustive},
                           std::move(disjunctors))};
}

auto compiler_draft4_applicator_properties_conditional_annotation(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate) -> Template {
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
  const auto imports_validation_vocabulary =
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
  if (imports_validation_vocabulary &&
      schema_context.schema.defines("required") &&
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
  std::vector<std::pair<std::string, Template>> properties;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    properties.push_back(
        {entry.first, compile(context, schema_context, relative_dynamic_context,
                              {entry.first}, {entry.first})});
    if (required.contains(entry.first)) {
      is_required += 1;
    }
  }

  // In many cases, `properties` have some subschemas that are small
  // and some subschemas that are large. To attempt to improve performance,
  // we prefer to evaluate smaller subschemas first, in the hope of failing
  // earlier without spending a lot of time on other subschemas
  std::sort(properties.begin(), properties.end(),
            [](const auto &left, const auto &right) {
              return (left.second.size() == right.second.size())
                         ? (left.first < right.first)
                         : (left.second.size() < right.second.size());
            });

  assert(schema_context.relative_pointer.back().is_property());
  assert(schema_context.relative_pointer.back().to_property() ==
         dynamic_context.keyword);
  const auto relative_pointer_size{schema_context.relative_pointer.size()};
  const auto is_directly_inside_oneof{
      relative_pointer_size > 2 &&
      schema_context.relative_pointer.at(relative_pointer_size - 2)
          .is_index() &&
      schema_context.relative_pointer.at(relative_pointer_size - 3)
          .is_property() &&
      schema_context.relative_pointer.at(relative_pointer_size - 3)
              .to_property() == "oneOf"};

  // There are two ways to compile `properties` depending on whether
  // most of the properties are marked as required using `required`
  // or whether most of the properties are optional. Each shines
  // in the corresponding case.
  const auto prefer_loop_over_instance{
      // This strategy only makes sense if most of the properties are "optional"
      is_required <= (size / 2) &&
      // If `properties` only defines a relatively small amount of properties,
      // then its probably still faster to unroll
      schema_context.schema.at(dynamic_context.keyword).size() > 5 &&
      // Always unroll inside `oneOf`, to have a better chance at
      // short-circuiting quickly
      !is_directly_inside_oneof};

  if (prefer_loop_over_instance) {
    ValueNamedIndexes indexes;
    Template children;
    std::size_t cursor = 0;

    for (auto &&[name, substeps] : properties) {
      indexes.emplace(name, cursor);
      if (annotate) {
        substeps.push_back(make<AnnotationEmit>(
            true, context, schema_context, relative_dynamic_context,
            sourcemeta::jsontoolkit::JSON{name}));
      }

      // Note that the evaluator completely ignores this wrapper anyway
      children.push_back(make<LogicalAnd>(false, context, schema_context,
                                          relative_dynamic_context, ValueNone{},
                                          std::move(substeps)));
      cursor += 1;
    }

    return {make<LoopPropertiesMatch>(true, context, schema_context,
                                      dynamic_context, std::move(indexes),
                                      std::move(children))};
  }

  Template children;

  for (auto &&[name, substeps] : properties) {
    if (annotate) {
      substeps.push_back(make<AnnotationEmit>(
          true, context, schema_context, relative_dynamic_context,
          sourcemeta::jsontoolkit::JSON{name}));
    }

    const auto assume_object{imports_validation_vocabulary &&
                             schema_context.schema.defines("type") &&
                             schema_context.schema.at("type").is_string() &&
                             schema_context.schema.at("type").to_string() ==
                                 "object"};

    // We can avoid this "defines" condition if the property is a required one
    if (imports_validation_vocabulary && assume_object &&
        schema_context.schema.defines("required") &&
        schema_context.schema.at("required").is_array() &&
        schema_context.schema.at("required")
            .contains(sourcemeta::jsontoolkit::JSON{name})) {
      // We can avoid the container too and just inline these steps
      for (auto &&substep : substeps) {
        children.push_back(std::move(substep));
      }

      // Optimize `properties` where its subschemas just include a type check,
      // as that's a very common pattern

    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionTypeStrict>(substeps.front())) {
      const auto &type_step{std::get<AssertionTypeStrict>(substeps.front())};
      children.push_back(AssertionPropertyTypeStrict{
          type_step.relative_schema_location,
          dynamic_context.base_instance_location.concat(
              type_step.relative_instance_location),
          type_step.keyword_location, type_step.schema_resource,
          type_step.dynamic, type_step.report, type_step.value});
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionType>(substeps.front())) {
      const auto &type_step{std::get<AssertionType>(substeps.front())};
      children.push_back(AssertionPropertyType{
          type_step.relative_schema_location,
          dynamic_context.base_instance_location.concat(
              type_step.relative_instance_location),
          type_step.keyword_location, type_step.schema_resource,
          type_step.dynamic, type_step.report, type_step.value});
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionPropertyTypeStrict>(
                   substeps.front())) {
      children.push_back(unroll<AssertionPropertyTypeStrict>(
          relative_dynamic_context, substeps.front(),
          dynamic_context.base_instance_location));
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionPropertyType>(
                   substeps.front())) {
      children.push_back(unroll<AssertionPropertyType>(
          relative_dynamic_context, substeps.front(),
          dynamic_context.base_instance_location));

    } else {
      children.push_back(make<LogicalWhenDefines>(
          false, context, schema_context, relative_dynamic_context,
          ValueString{name}, std::move(substeps)));
    }
  }

  // Optimize away the wrapper when emitting a single instruction
  if (context.mode == Mode::FastValidation && children.size() == 1 &&
      std::holds_alternative<AssertionPropertyTypeStrict>(children.front())) {
    return {
        unroll<AssertionPropertyTypeStrict>(dynamic_context, children.front())};
  }

  return {make<LogicalAnd>(true, context, schema_context, dynamic_context,
                           ValueNone{}, std::move(children))};
}

auto compiler_draft4_applicator_properties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_properties_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_patternproperties_conditional_annotation(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Template children;

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
      substeps.push_back(make<AnnotationBasenameToParent>(
          true, context, schema_context, relative_dynamic_context,
          ValueNone{}));
    }

    // If the `patternProperties` subschema for the given pattern does
    // nothing, then we can avoid generating an entire loop for it
    if (!substeps.empty()) {
      // Loop over the instance properties
      children.push_back(make<LoopPropertiesRegex>(
          // Treat this as an internal step
          false, context, schema_context, relative_dynamic_context,
          ValueRegex{parse_regex(pattern, schema_context.base,
                                 schema_context.relative_pointer),
                     pattern},
          std::move(substeps)));
    }
  }

  if (children.empty()) {
    return {};
  }

  // If the instance is an object...
  return {make<LogicalWhenType>(true, context, schema_context, dynamic_context,
                                sourcemeta::jsontoolkit::JSON::Type::Object,
                                std::move(children))};
}

auto compiler_draft4_applicator_patternproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_patternproperties_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_additionalproperties_conditional_annotation(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Template children{compile(context, schema_context, relative_dynamic_context,
                            sourcemeta::jsontoolkit::empty_pointer,
                            sourcemeta::jsontoolkit::empty_pointer)};

  if (annotate) {
    children.push_back(make<AnnotationBasenameToParent>(
        true, context, schema_context, relative_dynamic_context, ValueNone{}));
  }

  ValuePropertyFilter filter;

  if (schema_context.schema.defines("properties") &&
      schema_context.schema.at("properties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("properties").as_object()) {
      filter.first.push_back(entry.first);
    }
  }

  if (schema_context.schema.defines("patternProperties") &&
      schema_context.schema.at("patternProperties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("patternProperties").as_object()) {
      filter.second.push_back(
          {parse_regex(entry.first, schema_context.base,
                       schema_context.relative_pointer.initial().concat(
                           {"patternProperties"})),
           entry.first});
    }
  }

  // For performance, if a schema sets `additionalProperties: true` (or its
  // variants), we don't need to do anything
  if (children.empty()) {
    return {};
  }

  if (!filter.first.empty() || !filter.second.empty()) {
    return {make<LoopPropertiesExcept>(true, context, schema_context,
                                       dynamic_context, std::move(filter),
                                       std::move(children))};
  } else {
    if (context.mode == Mode::FastValidation && children.size() == 1) {
      // Optimize `additionalProperties` set to just `type`, which is a
      // pretty common pattern
      if (std::holds_alternative<AssertionTypeStrict>(children.front())) {
        const auto &type_step{std::get<AssertionTypeStrict>(children.front())};
        return {make<LoopPropertiesTypeStrict>(
            true, context, schema_context, dynamic_context, type_step.value)};
      } else if (std::holds_alternative<AssertionType>(children.front())) {
        const auto &type_step{std::get<AssertionType>(children.front())};
        return {make<LoopPropertiesType>(true, context, schema_context,
                                         dynamic_context, type_step.value)};
      }
    }

    return {make<LoopProperties>(true, context, schema_context, dynamic_context,
                                 ValueNone{}, std::move(children))};
  }
}

auto compiler_draft4_applicator_additionalproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_additionalproperties_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_validation_pattern(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_string());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  const auto &regex_string{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  return {make<AssertionRegex>(
      true, context, schema_context, dynamic_context,
      ValueRegex{parse_regex(regex_string, schema_context.base,
                             schema_context.relative_pointer),
                 regex_string})};
}

auto compiler_draft4_validation_format(const Context &context,
                                       const SchemaContext &schema_context,
                                       const DynamicContext &dynamic_context)
    -> Template {
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
    return {make<AssertionStringType>(true, context, schema_context,
                                      dynamic_context, ValueStringType::URI)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {make<AssertionRegex>(                                              \
        true, context, schema_context, dynamic_context,                        \
        ValueRegex{parse_regex(regular_expression, schema_context.base,        \
                               schema_context.relative_pointer),               \
                   (regular_expression)})};                                    \
  }

  COMPILE_FORMAT_REGEX("ipv4", FORMAT_REGEX_IPV4)

#undef COMPILE_FORMAT_REGEX

  return {};
}

auto compiler_draft4_applicator_not(const Context &context,
                                    const SchemaContext &schema_context,
                                    const DynamicContext &dynamic_context)
    -> Template {
  // Only emit a `not` instruction that keeps track of
  // dropping annotations if we really need it
  if (context.mode != Mode::FastValidation ||
      context.uses_unevaluated_properties || context.uses_unevaluated_items) {
    return {make<AnnotationNot>(
        true, context, schema_context, dynamic_context, ValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                sourcemeta::jsontoolkit::empty_pointer,
                sourcemeta::jsontoolkit::empty_pointer))};
  } else {
    return {make<LogicalNot>(
        true, context, schema_context, dynamic_context, ValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                sourcemeta::jsontoolkit::empty_pointer,
                sourcemeta::jsontoolkit::empty_pointer))};
  }
}

auto compiler_draft4_applicator_items_array(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate) -> Template {
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
  std::vector<Template> subschemas;
  subschemas.reserve(items_size);
  const auto &array{
      schema_context.schema.at(dynamic_context.keyword).as_array()};
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    subschemas.push_back(compile(context, schema_context,
                                 relative_dynamic_context, {subschemas.size()},
                                 {subschemas.size()}));
  }

  Template children;
  for (std::size_t cursor = items_size; cursor > 0; cursor--) {
    Template subchildren;
    for (std::size_t index = 0; index < cursor; index++) {
      for (const auto &substep : subschemas.at(index)) {
        subchildren.push_back(substep);
      }
    }

    // The first entry
    if (cursor == items_size) {
      if (annotate) {
        subchildren.push_back(make<AnnotationWhenArraySizeEqual>(
            true, context, schema_context, relative_dynamic_context,
            ValueIndexedJSON{cursor, sourcemeta::jsontoolkit::JSON{true}}));
        subchildren.push_back(make<AnnotationWhenArraySizeGreater>(
            true, context, schema_context, relative_dynamic_context,
            ValueIndexedJSON{cursor,
                             sourcemeta::jsontoolkit::JSON{cursor - 1}}));
      }

      children.push_back(make<LogicalWhenArraySizeGreater>(
          false, context, schema_context, relative_dynamic_context,
          ValueUnsignedInteger{cursor - 1}, std::move(subchildren)));
    } else {
      if (annotate) {
        subchildren.push_back(make<AnnotationEmit>(
            true, context, schema_context, relative_dynamic_context,
            sourcemeta::jsontoolkit::JSON{cursor - 1}));
      }

      children.push_back(make<LogicalWhenArraySizeEqual>(
          false, context, schema_context, relative_dynamic_context,
          ValueUnsignedInteger{cursor}, std::move(subchildren)));
    }
  }

  return {make<LogicalWhenType>(true, context, schema_context, dynamic_context,
                                sourcemeta::jsontoolkit::JSON::Type::Array,
                                std::move(children))};
}

auto compiler_draft4_applicator_items_conditional_annotation(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    if (annotate) {
      Template children;
      children.push_back(make<LoopItems>(
          true, context, schema_context, relative_dynamic_context,
          ValueUnsignedInteger{0},
          compile(context, schema_context, relative_dynamic_context,
                  sourcemeta::jsontoolkit::empty_pointer,
                  sourcemeta::jsontoolkit::empty_pointer)));
      children.push_back(make<AnnotationEmit>(
          true, context, schema_context, relative_dynamic_context,
          sourcemeta::jsontoolkit::JSON{true}));

      return {make<LogicalWhenType>(
          false, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array, std::move(children))};
    }

    return {make<LoopItems>(
        true, context, schema_context, dynamic_context, ValueUnsignedInteger{0},
        compile(context, schema_context, relative_dynamic_context,
                sourcemeta::jsontoolkit::empty_pointer,
                sourcemeta::jsontoolkit::empty_pointer))};
  }

  return compiler_draft4_applicator_items_array(context, schema_context,
                                                dynamic_context, annotate);
}

auto compiler_draft4_applicator_items(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  return compiler_draft4_applicator_items_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_additionalitems_from_cursor(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const std::size_t cursor,
    const bool annotate) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  Template children{
      make<LoopItems>(true, context, schema_context, relative_dynamic_context,
                      ValueUnsignedInteger{cursor},
                      compile(context, schema_context, relative_dynamic_context,
                              sourcemeta::jsontoolkit::empty_pointer,
                              sourcemeta::jsontoolkit::empty_pointer))};

  if (annotate) {
    children.push_back(make<AnnotationEmit>(
        true, context, schema_context, relative_dynamic_context,
        sourcemeta::jsontoolkit::JSON{true}));
  }

  return {make<LogicalWhenArraySizeGreater>(
      false, context, schema_context, dynamic_context,
      ValueUnsignedInteger{cursor}, std::move(children))};
}

auto compiler_draft4_applicator_additionalitems_conditional_annotation(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate) -> Template {
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
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_additionalitems_conditional_annotation(
      context, schema_context, dynamic_context, false);
}

auto compiler_draft4_applicator_dependencies(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  Template children;
  ValueStringMap dependencies;

  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (is_schema(entry.second)) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(make<LogicalWhenDefines>(
            false, context, schema_context, relative_dynamic_context,
            ValueString{entry.first},
            compile(context, schema_context, relative_dynamic_context,
                    {entry.first}, sourcemeta::jsontoolkit::empty_pointer)));
      }
    } else if (entry.second.is_array()) {
      std::vector<sourcemeta::jsontoolkit::JSON::String> properties;
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
    children.push_back(make<AssertionPropertyDependencies>(
        false, context, schema_context, relative_dynamic_context,
        std::move(dependencies)));
  }

  return {make<LogicalWhenType>(true, context, schema_context, dynamic_context,
                                sourcemeta::jsontoolkit::JSON::Type::Object,
                                std::move(children))};
}

auto compiler_draft4_validation_enum(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.at(dynamic_context.keyword).size() == 1) {
    return {make<AssertionEqual>(
        true, context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword).front()})};
  }

  std::vector<sourcemeta::jsontoolkit::JSON> options;
  for (const auto &option :
       schema_context.schema.at(dynamic_context.keyword).as_array()) {
    options.push_back(option);
  }

  return {make<AssertionEqualsAny>(true, context, schema_context,
                                   dynamic_context, std::move(options))};
}

auto compiler_draft4_validation_uniqueitems(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  if (!schema_context.schema.at(dynamic_context.keyword).is_boolean() ||
      !schema_context.schema.at(dynamic_context.keyword).to_boolean()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  return {make<AssertionUnique>(true, context, schema_context, dynamic_context,
                                ValueNone{})};
}

auto compiler_draft4_validation_maxlength(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "string") {
    return {};
  }

  return {make<AssertionStringSizeLess>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minlength(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "string") {
    return {};
  }

  return {make<AssertionStringSizeGreater>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maxitems(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "array") {
    return {};
  }

  return {make<AssertionArraySizeLess>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minitems(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "array") {
    return {};
  }

  return {make<AssertionArraySizeGreater>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maxproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "object") {
    return {};
  }

  return {make<AssertionObjectSizeLess>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_integer() ||
         schema_context.schema.at(dynamic_context.keyword).is_integer_real());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "object") {
    return {};
  }

  return {make<AssertionObjectSizeGreater>(
      true, context, schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maximum(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context)
    -> Template {
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
    return {make<AssertionLess>(
        true, context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make<AssertionLessEqual>(
        true, context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword)})};
  }
}

auto compiler_draft4_validation_minimum(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context)
    -> Template {
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
    return {make<AssertionGreater>(
        true, context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make<AssertionGreaterEqual>(
        true, context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword)})};
  }
}

auto compiler_draft4_validation_multipleof(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<AssertionDivisible>(
      true, context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

} // namespace internal
#endif
