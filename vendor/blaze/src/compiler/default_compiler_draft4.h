#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator_context.h>

#include <algorithm> // std::sort, std::any_of, std::all_of, std::find_if, std::none_of
#include <cassert> // assert
#include <regex>   // std::regex, std::regex_error
#include <set>     // std::set
#include <sstream> // std::ostringstream
#include <utility> // std::move

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

static auto collect_jump_labels(const sourcemeta::blaze::Template &steps,
                                std::set<std::size_t> &output) -> void {
  for (const auto &variant : steps) {
    std::visit(
        [&output](const auto &step) {
          using T = std::decay_t<decltype(step)>;
          if constexpr (std::is_same_v<T, sourcemeta::blaze::ControlJump>) {
            output.emplace(step.value);
          } else if constexpr (requires { step.children; }) {
            collect_jump_labels(step.children, output);
          }
        },
        variant);
  }
}

static auto relative_schema_location_size(
    const sourcemeta::blaze::Template::value_type &variant) -> std::size_t {
  return std::visit(
      [](const auto &step) { return step.relative_schema_location.size(); },
      variant);
}

static auto defines_direct_enumeration(const sourcemeta::blaze::Template &steps)
    -> std::optional<std::size_t> {
  const auto iterator{
      std::find_if(steps.cbegin(), steps.cend(), [](const auto &step) {
        return std::holds_alternative<sourcemeta::blaze::AssertionEqual>(
                   step) ||
               std::holds_alternative<sourcemeta::blaze::AssertionEqualsAny>(
                   step);
      })};

  if (iterator == steps.cend()) {
    return std::nullopt;
  }

  return std::distance(steps.cbegin(), iterator);
}

static auto
is_inside_disjunctor(const sourcemeta::jsontoolkit::Pointer &pointer) -> bool {
  return pointer.size() > 2 && pointer.at(pointer.size() - 2).is_index() &&
         pointer.at(pointer.size() - 3).is_property() &&
         (pointer.at(pointer.size() - 3).to_property() == "oneOf" ||
          pointer.at(pointer.size() - 3).to_property() == "anyOf");
}

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft4_core_ref(const Context &context,
                              const SchemaContext &schema_context,
                              const DynamicContext &dynamic_context)
    -> Template {
  // Determine the label
  const auto &entry{static_frame_entry(context, schema_context)};
  const auto type{sourcemeta::jsontoolkit::ReferenceType::Static};
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
  if (schema_context.labels.contains(label) ||
      context.precompiled_static_schemas.contains(reference.destination)) {
    return {make<ControlJump>(context, schema_context, dynamic_context,
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
          context, schema_context, dynamic_context, ValueNone{},
          compile(context, new_schema_context, relative_dynamic_context,
                  sourcemeta::jsontoolkit::empty_pointer,
                  sourcemeta::jsontoolkit::empty_pointer,
                  reference.destination))};
    }
  }

  new_schema_context.labels.insert(label);
  Template children{
      compile(context, new_schema_context, relative_dynamic_context,
              sourcemeta::jsontoolkit::empty_pointer,
              sourcemeta::jsontoolkit::empty_pointer, reference.destination)};

  // If we ended up not using the label after all, then we can ignore the
  // wrapper, at the expense of compiling the reference instructions once more
  std::set<std::size_t> used_labels;
  collect_jump_labels(children, used_labels);
  if (!used_labels.contains(label)) {
    return compile(context, schema_context, dynamic_context,
                   sourcemeta::jsontoolkit::empty_pointer,
                   sourcemeta::jsontoolkit::empty_pointer,
                   reference.destination);
  }

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  return {make<ControlLabel>(context, schema_context, dynamic_context,
                             ValueUnsignedInteger{label}, std::move(children))};
}

auto compiler_draft4_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context)
    -> Template {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_null(); })) {
        return {};
      }

      return {
          make<AssertionTypeStrict>(context, schema_context, dynamic_context,
                                    sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_boolean(); })) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeObjectBounded>(context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_object(); })) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeArrayBounded>(context, schema_context,
                                                dynamic_context,
                                                {minimum, maximum, false})};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_array(); })) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_number(); })) {
        return {};
      }

      return {make<AssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_integer(); })) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeStringBounded>(context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_string(); })) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
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
      return {
          make<AssertionTypeStrict>(context, schema_context, dynamic_context,
                                    sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      return {make<AssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
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
    return {make<AssertionTypeStrictAny>(context, schema_context,
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
      return {make<AssertionDefines>(context, schema_context, dynamic_context,
                                     ValueString{*(properties.cbegin())})};
    } else {
      return {make<AssertionDefinesAll>(
          context, schema_context, dynamic_context, std::move(properties))};
    }
  } else {
    assert(
        schema_context.schema.at(dynamic_context.keyword).front().is_string());
    return {make<AssertionDefines>(
        context, schema_context, dynamic_context,
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

  return {make<LogicalAnd>(context, schema_context, dynamic_context,
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
    disjunctors.push_back(make<ControlGroup>(
        context, schema_context, relative_dynamic_context, ValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<sourcemeta::jsontoolkit::Pointer::Token::Index>(
                    index)})));
  }

  const auto requires_exhaustive{
      context.mode == Mode::Exhaustive ||
      !context.unevaluated_properties_schemas.empty() ||
      !context.unevaluated_items_schemas.empty()};

  return {make<LogicalOr>(context, schema_context, dynamic_context,
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
    disjunctors.push_back(make<ControlGroup>(
        context, schema_context, relative_dynamic_context, ValueNone{},
        compile(context, schema_context, relative_dynamic_context,
                {static_cast<sourcemeta::jsontoolkit::Pointer::Token::Index>(
                    index)})));
  }

  const auto requires_exhaustive{
      context.mode == Mode::Exhaustive ||
      !context.unevaluated_properties_schemas.empty() ||
      !context.unevaluated_items_schemas.empty()};

  return {make<LogicalXor>(context, schema_context, dynamic_context,
                           ValueBoolean{requires_exhaustive},
                           std::move(disjunctors))};
}

static auto compile_properties(const Context &context,
                               const SchemaContext &schema_context,
                               const DynamicContext &dynamic_context)
    -> std::vector<std::pair<std::string, Template>> {
  std::vector<std::pair<std::string, Template>> properties;
  for (const auto &entry : schema_context.schema.at("properties").as_object()) {
    properties.push_back(
        {entry.first, compile(context, schema_context, dynamic_context,
                              {entry.first}, {entry.first})});
  }

  // In many cases, `properties` have some subschemas that are small
  // and some subschemas that are large. To attempt to improve performance,
  // we prefer to evaluate smaller subschemas first, in the hope of failing
  // earlier without spending a lot of time on other subschemas
  std::sort(properties.begin(), properties.end(),
            [](const auto &left, const auto &right) {
              const auto left_size{recursive_template_size(left.second)};
              const auto right_size{recursive_template_size(right.second)};
              if (left_size == right_size) {
                const auto left_direct_enumeration{
                    defines_direct_enumeration(left.second)};
                const auto right_direct_enumeration{
                    defines_direct_enumeration(right.second)};

                // Enumerations always take precedence
                if (left_direct_enumeration.has_value() &&
                    right_direct_enumeration.has_value()) {
                  // If both options have a direct enumeration, we choose
                  // the one with the shorter relative schema location
                  return relative_schema_location_size(
                             left.second.at(left_direct_enumeration.value())) <
                         relative_schema_location_size(
                             right.second.at(right_direct_enumeration.value()));
                } else if (left_direct_enumeration.has_value()) {
                  return true;
                } else if (right_direct_enumeration.has_value()) {
                  return false;
                }

                return left.first < right.first;
              } else {
                return left_size < right_size;
              }
            });

  return properties;
}

auto compiler_draft4_applicator_properties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Template {
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
  const auto imports_const =
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

  const auto &current_entry{static_frame_entry(context, schema_context)};
  const auto inside_disjunctor{
      is_inside_disjunctor(schema_context.relative_pointer) ||
      // Check if any reference from `anyOf` or `oneOf` points to us
      std::any_of(context.references.cbegin(), context.references.cend(),
                  [&context, &current_entry](const auto &reference) {
                    if (!context.frame.contains(
                            {sourcemeta::jsontoolkit::ReferenceType::Static,
                             reference.second.destination})) {
                      return false;
                    }

                    const auto &target{
                        context.frame
                            .at({sourcemeta::jsontoolkit::ReferenceType::Static,
                                 reference.second.destination})
                            .pointer};
                    return is_inside_disjunctor(reference.first.second) &&
                           current_entry.pointer.initial() == target;
                  })};

  // There are two ways to compile `properties` depending on whether
  // most of the properties are marked as required using `required`
  // or whether most of the properties are optional. Each shines
  // in the corresponding case.
  const auto prefer_loop_over_instance{
      // This strategy only makes sense if most of the properties are "optional"
      required.size() <= (size / 4) &&
      // If `properties` only defines a relatively small amount of properties,
      // then its probably still faster to unroll
      size > 5 &&
      // Always unroll inside `oneOf` or `anyOf`, to have a
      // better chance at quickly short-circuiting
      (!inside_disjunctor ||
       std::none_of(
           schema_context.schema.at(dynamic_context.keyword)
               .as_object()
               .cbegin(),
           schema_context.schema.at(dynamic_context.keyword).as_object().cend(),
           [&](const auto &pair) {
             return pair.second.is_object() &&
                    ((imports_validation_vocabulary &&
                      pair.second.defines("enum")) ||
                     (imports_const && pair.second.defines("const")));
           }))};

  if (prefer_loop_over_instance) {
    ValueNamedIndexes indexes;
    Template children;
    std::size_t cursor = 0;

    for (auto &&[name, substeps] : compile_properties(
             context, schema_context, relative_dynamic_context)) {
      indexes.emplace(name, cursor);

      if (track_evaluation) {
        substeps.push_back(make<ControlEvaluate>(context, schema_context,
                                                 relative_dynamic_context,
                                                 ValuePointer{name}));
      }

      if (annotate) {
        substeps.push_back(make<AnnotationEmit>(
            context, schema_context, relative_dynamic_context,
            sourcemeta::jsontoolkit::JSON{name}));
      }

      // Note that the evaluator completely ignores this wrapper anyway
      children.push_back(make<ControlGroup>(context, schema_context,
                                            relative_dynamic_context,
                                            ValueNone{}, std::move(substeps)));
      cursor += 1;
    }

    return {make<LoopPropertiesMatch>(context, schema_context, dynamic_context,
                                      std::move(indexes), std::move(children))};
  }

  Template children;

  const auto effective_dynamic_context{context.mode == Mode::FastValidation
                                           ? dynamic_context
                                           : relative_dynamic_context};

  for (auto &&[name, substeps] :
       compile_properties(context, schema_context, effective_dynamic_context)) {
    if (annotate) {
      substeps.push_back(make<AnnotationEmit>(
          context, schema_context, effective_dynamic_context,
          sourcemeta::jsontoolkit::JSON{name}));
    }

    // Optimize `properties` where its subschemas just include a type check,
    // as that's a very common pattern

    if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
        std::holds_alternative<AssertionTypeStrict>(substeps.front())) {
      const auto &type_step{std::get<AssertionTypeStrict>(substeps.front())};
      if (track_evaluation) {
        children.push_back(
            rephrase<AssertionPropertyTypeStrictEvaluate>(type_step));
      } else {
        children.push_back(rephrase<AssertionPropertyTypeStrict>(type_step));
      }
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionType>(substeps.front())) {
      const auto &type_step{std::get<AssertionType>(substeps.front())};
      if (track_evaluation) {
        children.push_back(rephrase<AssertionPropertyTypeEvaluate>(type_step));
      } else {
        children.push_back(rephrase<AssertionPropertyType>(type_step));
      }
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionTypeStrictAny>(
                   substeps.front())) {
      const auto &type_step{std::get<AssertionTypeStrictAny>(substeps.front())};
      if (track_evaluation) {
        children.push_back(
            rephrase<AssertionPropertyTypeStrictAnyEvaluate>(type_step));
      } else {
        children.push_back(rephrase<AssertionPropertyTypeStrictAny>(type_step));
      }

    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionPropertyTypeStrict>(
                   substeps.front())) {
      children.push_back(unroll<AssertionPropertyTypeStrict>(
          substeps.front(), effective_dynamic_context.base_instance_location));
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionPropertyType>(
                   substeps.front())) {
      children.push_back(unroll<AssertionPropertyType>(
          substeps.front(), effective_dynamic_context.base_instance_location));
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               std::holds_alternative<AssertionPropertyTypeStrictAny>(
                   substeps.front())) {
      children.push_back(unroll<AssertionPropertyTypeStrictAny>(
          substeps.front(), effective_dynamic_context.base_instance_location));

    } else {
      if (track_evaluation) {
        if (context.mode == Mode::FastValidation) {
          // We need this wrapper as `ControlEvaluate` doesn't push to the stack
          substeps.push_back(make<LogicalAnd>(
              context, schema_context, effective_dynamic_context, ValueNone{},
              {make<ControlEvaluate>(context, schema_context,
                                     effective_dynamic_context,
                                     ValuePointer{name})}));
        } else {
          substeps.push_back(make<ControlEvaluate>(context, schema_context,
                                                   effective_dynamic_context,
                                                   ValuePointer{name}));
        }
      }

      if (imports_validation_vocabulary &&
          schema_context.schema.defines("type") &&
          schema_context.schema.at("type").is_string() &&
          schema_context.schema.at("type").to_string() == "object" &&
          required.contains(name)) {
        // We can avoid the container too and just inline these steps
        for (auto &&substep : substeps) {
          children.push_back(std::move(substep));
        }
      } else if (!substeps.empty()) {
        children.push_back(make<ControlGroupWhenDefines>(
            context, schema_context, effective_dynamic_context,
            ValueString{name}, std::move(substeps)));
      }
    }
  }

  if (context.mode == Mode::FastValidation) {
    return children;
  } else if (children.empty()) {
    return {};
  } else {
    return {make<LogicalAnd>(context, schema_context, dynamic_context,
                             ValueNone{}, std::move(children))};
  }
}

auto compiler_draft4_applicator_properties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_properties_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_applicator_patternproperties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Template {
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
      substeps.push_back(make<AnnotationBasenameToParent>(
          context, schema_context, relative_dynamic_context, ValueNone{}));
    }

    if (track_evaluation) {
      substeps.push_back(make<ControlEvaluate>(
          context, schema_context, relative_dynamic_context, ValuePointer{}));
    }

    // If the `patternProperties` subschema for the given pattern does
    // nothing, then we can avoid generating an entire loop for it
    if (!substeps.empty()) {
      const auto maybe_prefix{pattern_as_prefix(pattern)};
      if (maybe_prefix.has_value()) {
        children.push_back(make<LoopPropertiesStartsWith>(
            context, schema_context, dynamic_context,
            ValueString{maybe_prefix.value()}, std::move(substeps)));
      } else {
        children.push_back(make<LoopPropertiesRegex>(
            context, schema_context, dynamic_context,
            ValueRegex{parse_regex(pattern, schema_context.base,
                                   schema_context.relative_pointer),
                       pattern},
            std::move(substeps)));
      }
    }
  }

  return children;
}

auto compiler_draft4_applicator_patternproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_patternproperties_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_applicator_additionalproperties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Template {
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
        context, schema_context, relative_dynamic_context, ValueNone{}));
  }

  ValueStrings filter_strings;
  ValueStrings filter_prefixes;
  std::vector<ValueRegex> filter_regexes;

  if (schema_context.schema.defines("properties") &&
      schema_context.schema.at("properties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("properties").as_object()) {
      filter_strings.push_back(entry.first);
    }
  }

  if (schema_context.schema.defines("patternProperties") &&
      schema_context.schema.at("patternProperties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("patternProperties").as_object()) {
      const auto maybe_prefix{pattern_as_prefix(entry.first)};
      if (maybe_prefix.has_value()) {
        filter_prefixes.push_back(maybe_prefix.value());
      } else {
        filter_regexes.push_back(
            {parse_regex(entry.first, schema_context.base,
                         schema_context.relative_pointer.initial().concat(
                             {"patternProperties"})),
             entry.first});
      }
    }
  }

  // For performance, if a schema sets `additionalProperties: true` (or its
  // variants), we don't need to do anything
  if (!track_evaluation && children.empty()) {
    return {};
  }

  if (context.mode == Mode::FastValidation && children.size() == 1 &&
      std::holds_alternative<AssertionFail>(children.front()) &&
      !filter_strings.empty() && filter_prefixes.empty() &&
      filter_regexes.empty()) {
    return {make<LoopPropertiesWhitelist>(
        context, schema_context, dynamic_context, std::move(filter_strings))};
  }

  if (!filter_strings.empty() || !filter_prefixes.empty() ||
      !filter_regexes.empty()) {
    if (track_evaluation) {
      children.push_back(make<ControlEvaluate>(
          context, schema_context, relative_dynamic_context, ValuePointer{}));
    }

    return {make<LoopPropertiesExcept>(
        context, schema_context, dynamic_context,
        ValuePropertyFilter{std::move(filter_strings),
                            std::move(filter_prefixes),
                            std::move(filter_regexes)},
        std::move(children))};

    // Optimize `additionalProperties` set to just `type`, which is a
    // pretty common pattern
  } else if (context.mode == Mode::FastValidation && children.size() == 1 &&
             std::holds_alternative<AssertionTypeStrict>(children.front())) {
    const auto &type_step{std::get<AssertionTypeStrict>(children.front())};
    if (track_evaluation) {
      return {make<LoopPropertiesTypeStrictEvaluate>(
          context, schema_context, dynamic_context, type_step.value)};
    } else {
      return {make<LoopPropertiesTypeStrict>(context, schema_context,
                                             dynamic_context, type_step.value)};
    }
  } else if (context.mode == Mode::FastValidation && children.size() == 1 &&
             std::holds_alternative<AssertionType>(children.front())) {
    const auto &type_step{std::get<AssertionType>(children.front())};
    if (track_evaluation) {
      return {make<LoopPropertiesTypeEvaluate>(
          context, schema_context, dynamic_context, type_step.value)};
    } else {
      return {make<LoopPropertiesType>(context, schema_context, dynamic_context,
                                       type_step.value)};
    }
  } else if (context.mode == Mode::FastValidation && children.size() == 1 &&
             std::holds_alternative<AssertionTypeStrictAny>(children.front())) {
    const auto &type_step{std::get<AssertionTypeStrictAny>(children.front())};
    if (track_evaluation) {
      return {make<LoopPropertiesTypeStrictAnyEvaluate>(
          context, schema_context, dynamic_context, type_step.value)};
    } else {
      return {make<LoopPropertiesTypeStrictAny>(
          context, schema_context, dynamic_context, type_step.value)};
    }

  } else if (track_evaluation) {
    if (children.empty()) {
      return {make<ControlEvaluate>(context, schema_context, dynamic_context,
                                    ValuePointer{})};
    }

    return {make<LoopPropertiesEvaluate>(context, schema_context,
                                         dynamic_context, ValueNone{},
                                         std::move(children))};
  } else {
    return {make<LoopProperties>(context, schema_context, dynamic_context,
                                 ValueNone{}, std::move(children))};
  }
}

auto compiler_draft4_applicator_additionalproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_additionalproperties_with_options(
      context, schema_context, dynamic_context, false, false);
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
      context, schema_context, dynamic_context,
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
    return {make<AssertionStringType>(context, schema_context, dynamic_context,
                                      ValueStringType::URI)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {make<AssertionRegex>(                                              \
        context, schema_context, dynamic_context,                              \
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
  std::size_t subschemas{0};
  for (const auto &subschema :
       walk_subschemas(context, schema_context, dynamic_context)) {
    if (subschema.pointer.empty()) {
      continue;
    }

    subschemas += 1;
  }

  Template children{compile(context, schema_context, relative_dynamic_context,
                            sourcemeta::jsontoolkit::empty_pointer,
                            sourcemeta::jsontoolkit::empty_pointer)};

  // Only emit a `not` instruction that keeps track of
  // evaluation if we really need it. If the "not" subschema
  // does not define applicators, then that's an easy case
  // we can skip
  if (subschemas > 0 && (!context.unevaluated_properties_schemas.empty() ||
                         !context.unevaluated_items_schemas.empty())) {
    return {make<LogicalNotEvaluate>(context, schema_context, dynamic_context,
                                     ValueNone{}, std::move(children))};
  } else {
    return {make<LogicalNot>(context, schema_context, dynamic_context,
                             ValueNone{}, std::move(children))};
  }
}

auto compiler_draft4_applicator_items_array(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Template {
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
  for (std::size_t cursor = 0; cursor < items_size; cursor++) {
    Template subchildren;
    for (std::size_t index = 0; index < cursor + 1; index++) {
      for (const auto &substep : subschemas.at(index)) {
        subchildren.push_back(substep);
      }
    }

    if (annotate) {
      subchildren.push_back(make<AnnotationEmit>(
          context, schema_context, relative_dynamic_context,
          sourcemeta::jsontoolkit::JSON{cursor}));
    }

    children.push_back(make<ControlGroup>(context, schema_context,
                                          relative_dynamic_context, ValueNone{},
                                          std::move(subchildren)));
  }

  Template tail;
  for (const auto &subschema : subschemas) {
    for (const auto &substep : subschema) {
      tail.push_back(substep);
    }
  }

  if (annotate) {
    tail.push_back(make<AnnotationEmit>(
        context, schema_context, relative_dynamic_context,
        sourcemeta::jsontoolkit::JSON{children.size() - 1}));
    tail.push_back(make<AnnotationEmit>(context, schema_context,
                                        relative_dynamic_context,
                                        sourcemeta::jsontoolkit::JSON{true}));
  }

  children.push_back(make<ControlGroup>(context, schema_context,
                                        relative_dynamic_context, ValueNone{},
                                        std::move(tail)));

  if (track_evaluation) {
    return {make<AssertionArrayPrefixEvaluate>(context, schema_context,
                                               dynamic_context, ValueNone{},
                                               std::move(children))};
  } else {
    return {make<AssertionArrayPrefix>(context, schema_context, dynamic_context,
                                       ValueNone{}, std::move(children))};
  }
}

auto compiler_draft4_applicator_items_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    if (annotate || track_evaluation) {
      Template subchildren{compile(context, schema_context,
                                   relative_dynamic_context,
                                   sourcemeta::jsontoolkit::empty_pointer,
                                   sourcemeta::jsontoolkit::empty_pointer)};

      Template children;

      if (!subchildren.empty()) {
        children.push_back(
            make<LoopItems>(context, schema_context, dynamic_context,
                            ValueUnsignedInteger{0}, std::move(subchildren)));
      }

      if (!annotate && !track_evaluation) {
        return children;
      }

      Template tail;

      if (annotate) {
        tail.push_back(make<AnnotationEmit>(
            context, schema_context, relative_dynamic_context,
            sourcemeta::jsontoolkit::JSON{true}));
      }

      if (track_evaluation) {
        tail.push_back(make<ControlEvaluate>(
            context, schema_context, relative_dynamic_context, ValuePointer{}));
      }

      children.push_back(make<LogicalWhenType>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array, std::move(tail)));

      return children;
    }

    Template children{compile(context, schema_context, relative_dynamic_context,
                              sourcemeta::jsontoolkit::empty_pointer,
                              sourcemeta::jsontoolkit::empty_pointer)};
    if (track_evaluation) {
      children.push_back(make<ControlEvaluate>(
          context, schema_context, relative_dynamic_context, ValuePointer{}));
    }

    if (children.empty()) {
      return {};
    }

    if (context.mode == Mode::FastValidation && children.size() == 1) {
      if (std::holds_alternative<AssertionTypeStrict>(children.front())) {
        return {make<LoopItemsTypeStrict>(
            context, schema_context, dynamic_context,
            std::get<AssertionTypeStrict>(children.front()).value)};
      } else if (std::holds_alternative<AssertionType>(children.front())) {
        return {make<LoopItemsType>(
            context, schema_context, dynamic_context,
            std::get<AssertionType>(children.front()).value)};
      } else if (std::holds_alternative<AssertionTypeStrictAny>(
                     children.front())) {
        return {make<LoopItemsTypeStrictAny>(
            context, schema_context, dynamic_context,
            std::get<AssertionTypeStrictAny>(children.front()).value)};
      }
    }

    return {make<LoopItems>(context, schema_context, dynamic_context,
                            ValueUnsignedInteger{0}, std::move(children))};
  }

  return compiler_draft4_applicator_items_array(
      context, schema_context, dynamic_context, annotate, track_evaluation);
}

auto compiler_draft4_applicator_items(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  return compiler_draft4_applicator_items_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_applicator_additionalitems_from_cursor(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const std::size_t cursor,
    const bool annotate, const bool track_evaluation) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  Template subchildren{compile(context, schema_context,
                               relative_dynamic_context,
                               sourcemeta::jsontoolkit::empty_pointer,
                               sourcemeta::jsontoolkit::empty_pointer)};

  Template children;

  if (!subchildren.empty()) {
    children.push_back(make<LoopItems>(context, schema_context, dynamic_context,
                                       ValueUnsignedInteger{cursor},
                                       std::move(subchildren)));
  }

  // Avoid one extra wrapper instruction if possible
  if (!annotate && !track_evaluation) {
    return children;
  }

  Template tail;

  if (annotate) {
    tail.push_back(make<AnnotationEmit>(context, schema_context,
                                        relative_dynamic_context,
                                        sourcemeta::jsontoolkit::JSON{true}));
  }

  if (track_evaluation) {
    tail.push_back(make<ControlEvaluate>(
        context, schema_context, relative_dynamic_context, ValuePointer{}));
  }

  assert(!tail.empty());
  children.push_back(make<LogicalWhenArraySizeGreater>(
      context, schema_context, dynamic_context, ValueUnsignedInteger{cursor},
      std::move(tail)));

  return children;
}

auto compiler_draft4_applicator_additionalitems_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Template {
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
      context, schema_context, dynamic_context, cursor, annotate,
      track_evaluation);
}

auto compiler_draft4_applicator_additionalitems(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_additionalitems_with_options(
      context, schema_context, dynamic_context, false, false);
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
            context, schema_context, dynamic_context, ValueString{entry.first},
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
        context, schema_context, dynamic_context, std::move(dependencies)));
  }

  return children;
}

auto compiler_draft4_validation_enum(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context)
    -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.at(dynamic_context.keyword).size() == 1) {
    return {make<AssertionEqual>(
        context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword).front()})};
  }

  std::vector<sourcemeta::jsontoolkit::JSON> options;
  for (const auto &option :
       schema_context.schema.at(dynamic_context.keyword).as_array()) {
    options.push_back(option);
  }

  return {make<AssertionEqualsAny>(context, schema_context, dynamic_context,
                                   std::move(options))};
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

  return {make<AssertionUnique>(context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
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
        context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make<AssertionLessEqual>(
        context, schema_context, dynamic_context,
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
        context, schema_context, dynamic_context,
        sourcemeta::jsontoolkit::JSON{
            schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make<AssertionGreaterEqual>(
        context, schema_context, dynamic_context,
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
      context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

} // namespace internal
#endif
