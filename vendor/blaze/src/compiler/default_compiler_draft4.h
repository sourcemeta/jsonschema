#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/regex.h>

#include <algorithm> // std::sort, std::any_of, std::all_of, std::find_if, std::none_of
#include <cassert> // assert
#include <set>     // std::set
#include <sstream> // std::ostringstream
#include <utility> // std::move

#include "compile_helpers.h"

static auto parse_regex(const std::string &pattern,
                        const sourcemeta::core::URI &base,
                        const sourcemeta::core::Pointer &schema_location)
    -> sourcemeta::core::Regex<sourcemeta::core::JSON::String> {
  const auto result{sourcemeta::core::to_regex(pattern)};
  if (!result.has_value()) {
    std::ostringstream message;
    message << "Invalid regular expression: " << pattern;
    throw sourcemeta::blaze::CompilerError(base, schema_location,
                                           message.str());
  }

  return result.value();
}

static auto collect_jump_labels(const sourcemeta::blaze::Instructions &steps,
                                std::set<std::size_t> &output) -> void {
  for (const auto &variant : steps) {
    if (variant.type == sourcemeta::blaze::InstructionIndex::ControlJump) {
      output.emplace(
          std::get<sourcemeta::blaze::ValueUnsignedInteger>(variant.value));
    } else {
      collect_jump_labels(variant.children, output);
    }
  }
}

static auto
relative_schema_location_size(const sourcemeta::blaze::Instruction &step)
    -> std::size_t {
  return step.relative_schema_location.size();
}

static auto
defines_direct_enumeration(const sourcemeta::blaze::Instructions &steps)
    -> std::optional<std::size_t> {
  const auto iterator{
      std::find_if(steps.cbegin(), steps.cend(), [](const auto &step) {
        return step.type ==
                   sourcemeta::blaze::InstructionIndex::AssertionEqual ||
               step.type ==
                   sourcemeta::blaze::InstructionIndex::AssertionEqualsAny;
      })};

  if (iterator == steps.cend()) {
    return std::nullopt;
  }

  return std::distance(steps.cbegin(), iterator);
}

static auto is_inside_disjunctor(const sourcemeta::core::Pointer &pointer)
    -> bool {
  return pointer.size() > 2 && pointer.at(pointer.size() - 2).is_index() &&
         pointer.at(pointer.size() - 3).is_property() &&
         (pointer.at(pointer.size() - 3).to_property() == "oneOf" ||
          pointer.at(pointer.size() - 3).to_property() == "anyOf");
}

static auto json_array_to_string_set(const sourcemeta::core::JSON &document)
    -> sourcemeta::blaze::ValueStringSet {
  sourcemeta::blaze::ValueStringSet result;
  for (const auto &value : document.as_array()) {
    assert(value.is_string());
    result.insert(value.to_string());
  }

  return result;
}

static auto
is_closed_properties_required(const sourcemeta::core::JSON &schema,
                              const sourcemeta::blaze::ValueStringSet &required)
    -> bool {
  return schema.defines("additionalProperties") &&
         schema.at("additionalProperties").is_boolean() &&
         !schema.at("additionalProperties").to_boolean() &&
         schema.defines("properties") && schema.at("properties").is_object() &&
         schema.at("properties").size() == required.size() &&
         std::all_of(required.begin(), required.end(),
                     [&schema](const auto &property) {
                       return schema.at("properties")
                           .defines(property.first, property.second);
                     });
}

static auto
compile_properties(const sourcemeta::blaze::Context &context,
                   const sourcemeta::blaze::SchemaContext &schema_context,
                   const sourcemeta::blaze::DynamicContext &dynamic_context,
                   const sourcemeta::blaze::Instructions &)
    -> std::vector<std::pair<std::string, sourcemeta::blaze::Instructions>> {
  std::vector<std::pair<std::string, sourcemeta::blaze::Instructions>>
      properties;
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

static auto to_string_hashes(
    std::vector<std::pair<sourcemeta::blaze::ValueString,
                          sourcemeta::blaze::ValueStringSet::hash_type>>
        &hashes) -> sourcemeta::blaze::ValueStringHashes {
  assert(!hashes.empty());
  std::sort(hashes.begin(), hashes.end(),
            [](const auto &left, const auto &right) {
              return left.first.size() < right.first.size();
            });

  sourcemeta::blaze::ValueStringHashes result;
  // The idea with the table of contents is as follows: each index
  // marks the starting and end positions for a string where the size
  // is equal to the index.
  result.second.resize(hashes.back().first.size() + 1, std::make_pair(0, 0));
  for (std::size_t index = 0; index < hashes.size(); index++) {
    result.first.push_back(hashes[index].second);
    const auto string_size{hashes[index].first.size()};
    // We leave index 0 to represent the empty string
    const auto position{index + 1};
    const auto lower_bound{
        result.second[string_size].first == 0
            ? position
            : std::min(result.second[string_size].first, position)};
    const auto upper_bound{
        result.second[string_size].second == 0
            ? position
            : std::max(result.second[string_size].second, position)};
    assert(lower_bound <= upper_bound);
    assert(lower_bound > 0 && upper_bound > 0);
    assert(string_size < result.second.size());
    result.second[string_size] = std::make_pair(lower_bound, upper_bound);
  }

  assert(result.second.size() == hashes.back().first.size() + 1);
  return result;
}

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft4_core_ref(const Context &context,
                              const SchemaContext &schema_context,
                              const DynamicContext &dynamic_context,
                              const Instructions &) -> Instructions {
  // Determine the label
  const auto &entry{static_frame_entry(context, schema_context)};
  const auto type{sourcemeta::core::SchemaReferenceType::Static};
  if (!context.frame.references().contains({type, entry.pointer})) {
    assert(schema_context.schema.at(dynamic_context.keyword).is_string());
    throw sourcemeta::core::SchemaReferenceError(
        schema_context.schema.at(dynamic_context.keyword).to_string(),
        entry.pointer, "The schema location is inside of an unknown keyword");
  }

  const auto &reference{context.frame.references().at({type, entry.pointer})};
  const auto label{
      Evaluator{}.hash(schema_resource_id(context, reference.base.value_or("")),
                       reference.fragment.value_or(""))};

  // The label is already registered, so just jump to it
  if (schema_context.labels.contains(label) ||
      context.precompiled_static_schemas.contains(reference.destination)) {
    return {make(sourcemeta::blaze::InstructionIndex::ControlJump, context,
                 schema_context, dynamic_context, ValueUnsignedInteger{label})};
  }

  auto new_schema_context{schema_context};
  new_schema_context.references.insert(reference.destination);

  // TODO: Replace this logic with `.frame()` `destination_of` information
  std::size_t direct_children_references{0};
  if (context.frame.locations().contains({type, reference.destination})) {
    for (const auto &reference_entry : context.frame.references()) {
      if (reference_entry.first.second.starts_with(
              context.frame.locations()
                  .at({type, reference.destination})
                  .pointer)) {
        direct_children_references += 1;
      }
    }
  }

  // If the reference is not a recursive one, we can avoid the extra
  // overhead of marking the location for future jumps, and pretty much
  // just expand the reference destination in place.
  // TODO: Elevate the calculation required to detect recursive references
  // to Core's `.frame()`
  // See: https://github.com/sourcemeta/core/issues/1394
  const bool is_recursive{
      // This means the reference is directly recursive, by jumping to
      // a parent of the reference itself.
      (context.frame.locations().contains({type, reference.destination}) &&
       entry.pointer.starts_with(context.frame.locations()
                                     .at({type, reference.destination})
                                     .pointer)) ||
      schema_context.references.contains(reference.destination)};

  if (!is_recursive && direct_children_references <= 5) {
    if (context.mode == Mode::FastValidation &&
        // Expanding references inline when dynamic scoping is required
        // may not work, as we might omit the instruction that introduces
        // one of the necessary schema resources to the evaluator
        !context.uses_dynamic_scopes) {
      return compile(context, new_schema_context, dynamic_context,
                     sourcemeta::core::empty_pointer,
                     sourcemeta::core::empty_pointer, reference.destination);
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
                   schema_context, dynamic_context, ValueNone{},
                   compile(context, new_schema_context,
                           relative_dynamic_context(dynamic_context),
                           sourcemeta::core::empty_pointer,
                           sourcemeta::core::empty_pointer,
                           reference.destination))};
    }
  }

  new_schema_context.labels.insert(label);
  Instructions children{compile(
      context, new_schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer,
      reference.destination)};

  // If we ended up not using the label after all, then we can ignore the
  // wrapper, at the expense of compiling the reference instructions once more
  std::set<std::size_t> used_labels;
  collect_jump_labels(children, used_labels);
  if (!used_labels.contains(label)) {
    return compile(context, schema_context, dynamic_context,
                   sourcemeta::core::empty_pointer,
                   sourcemeta::core::empty_pointer, reference.destination);
  }

  // The idea to handle recursion is to expand the reference once, and when
  // doing so, create a "checkpoint" that we can jump back to in a subsequent
  // recursive reference. While unrolling the reference once may initially
  // feel weird, we do it so we can handle references purely in this keyword
  // handler, without having to add logic to every single keyword to check
  // whether something points to them and add the "checkpoint" themselves.
  return {make(sourcemeta::blaze::InstructionIndex::ControlLabel, context,
               schema_context, dynamic_context, ValueUnsignedInteger{label},
               std::move(children))};
}

auto compiler_draft4_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context,
                                     const Instructions &) -> Instructions {
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

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Null)};
    } else if (type == "boolean") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_boolean(); })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};

      if (context.mode == Mode::FastValidation) {
        if (maximum.has_value() && minimum == 0) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeObjectUpper,
              context, schema_context, dynamic_context,
              ValueUnsignedInteger{maximum.value()})};
        } else if (minimum > 0 || maximum.has_value()) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeObjectBounded,
              context, schema_context, dynamic_context,
              ValueRange{minimum, maximum, false})};
        }
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_object(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("required")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};

      if (context.mode == Mode::FastValidation) {
        if (maximum.has_value() && minimum == 0) {
          return {
              make(sourcemeta::blaze::InstructionIndex::AssertionTypeArrayUpper,
                   context, schema_context, dynamic_context,
                   ValueUnsignedInteger{maximum.value()})};
        } else if (minimum > 0 || maximum.has_value()) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeArrayBounded,
              context, schema_context, dynamic_context,
              ValueRange{minimum, maximum, false})};
        }
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_array(); })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Array)};
    } else if (type == "number") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_number(); })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context,
                   std::vector<sourcemeta::core::JSON::Type>{
                       sourcemeta::core::JSON::Type::Real,
                       sourcemeta::core::JSON::Type::Integer})};
    } else if (type == "integer") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_integer(); })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};

      if (context.mode == Mode::FastValidation) {
        if (maximum.has_value() && minimum == 0) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeStringUpper,
              context, schema_context, dynamic_context,
              ValueUnsignedInteger{maximum.value()})};
        } else if (minimum > 0 || maximum.has_value()) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeStringBounded,
              context, schema_context, dynamic_context,
              ValueRange{minimum, maximum, false})};
        }
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_string(); })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
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
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Object)};
    } else if (type == "array") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Array)};
    } else if (type == "number") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context,
                   std::vector<sourcemeta::core::JSON::Type>{
                       sourcemeta::core::JSON::Type::Real,
                       sourcemeta::core::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    } else if (type == "string") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<sourcemeta::core::JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(sourcemeta::core::JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(sourcemeta::core::JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(sourcemeta::core::JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(sourcemeta::core::JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(sourcemeta::core::JSON::Type::Integer);
        types.push_back(sourcemeta::core::JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(sourcemeta::core::JSON::Type::Integer);
      } else if (type_string == "string") {
        types.push_back(sourcemeta::core::JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                 context, schema_context, dynamic_context, std::move(types))};
  }

  return {};
}

auto compiler_draft4_validation_required(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &current)
    -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  const auto assume_object{schema_context.schema.defines("type") &&
                           schema_context.schema.at("type").is_string() &&
                           schema_context.schema.at("type").to_string() ==
                               "object"};

  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  } else if (schema_context.schema.at(dynamic_context.keyword).size() > 1) {
    ValueStringSet properties_set{json_array_to_string_set(
        schema_context.schema.at(dynamic_context.keyword))};
    if (properties_set.size() == 1) {
      if (context.mode == Mode::FastValidation && assume_object) {
        return {
            make(sourcemeta::blaze::InstructionIndex::AssertionDefinesStrict,
                 context, schema_context, dynamic_context,
                 make_property(properties_set.begin()->first))};
      } else {
        return {make(sourcemeta::blaze::InstructionIndex::AssertionDefines,
                     context, schema_context, dynamic_context,
                     make_property(properties_set.begin()->first))};
      }
    } else if (is_closed_properties_required(schema_context.schema,
                                             properties_set)) {
      if (context.mode == Mode::FastValidation && assume_object) {
        const SchemaContext new_schema_context{
            schema_context.relative_pointer.initial().concat({"properties"}),
            schema_context.schema,
            schema_context.vocabularies,
            schema_context.base,
            schema_context.labels,
            schema_context.references};
        const DynamicContext new_dynamic_context{
            "properties", sourcemeta::core::empty_pointer,
            sourcemeta::core::empty_pointer, false};
        auto properties{compile_properties(context, new_schema_context,
                                           new_dynamic_context, current)};
        if (std::all_of(properties.cbegin(), properties.cend(),
                        [](const auto &property) {
                          return property.second.size() == 1 &&
                                 property.second.front().type ==
                                     InstructionIndex::AssertionTypeStrict;
                        })) {
          std::set<ValueType> types;
          for (const auto &property : properties) {
            types.insert(std::get<ValueType>(property.second.front().value));
          }

          if (types.size() == 1) {
            // Handled in `properties`
            return {};
          }
        }

        sourcemeta::core::PropertyHashJSON<ValueString> hasher;
        if (context.mode == Mode::FastValidation &&
            properties_set.size() == 3 &&
            std::all_of(properties_set.begin(), properties_set.end(),
                        [&hasher](const auto &property) {
                          return hasher.is_perfect(property.second);
                        })) {
          std::vector<std::pair<ValueString, ValueStringSet::hash_type>> hashes;
          for (const auto &property : properties_set) {
            hashes.emplace_back(property.first, property.second);
          }

          return {make(sourcemeta::blaze::InstructionIndex::
                           AssertionDefinesExactlyStrictHash3,
                       context, schema_context, dynamic_context,
                       to_string_hashes(hashes))};
        }

        return {make(
            sourcemeta::blaze::InstructionIndex::AssertionDefinesExactlyStrict,
            context, schema_context, dynamic_context,
            std::move(properties_set))};
      } else {
        return {
            make(sourcemeta::blaze::InstructionIndex::AssertionDefinesExactly,
                 context, schema_context, dynamic_context,
                 std::move(properties_set))};
      }
    } else if (context.mode == Mode::FastValidation && assume_object) {
      return {make(
          sourcemeta::blaze::InstructionIndex::AssertionDefinesAllStrict,
          context, schema_context, dynamic_context, std::move(properties_set))};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionDefinesAll,
                   context, schema_context, dynamic_context,
                   std::move(properties_set))};
    }
  } else if (context.mode == Mode::FastValidation && assume_object) {
    assert(
        schema_context.schema.at(dynamic_context.keyword).front().is_string());
    return {make(sourcemeta::blaze::InstructionIndex::AssertionDefinesStrict,
                 context, schema_context, dynamic_context,
                 make_property(schema_context.schema.at(dynamic_context.keyword)
                                   .front()
                                   .to_string()))};
  } else {
    assert(
        schema_context.schema.at(dynamic_context.keyword).front().is_string());
    return {make(sourcemeta::blaze::InstructionIndex::AssertionDefines, context,
                 schema_context, dynamic_context,
                 make_property(schema_context.schema.at(dynamic_context.keyword)
                                   .front()
                                   .to_string()))};
  }
}

auto compiler_draft4_applicator_allof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Instructions children;

  if (context.mode == Mode::FastValidation &&
      // TODO: Make this work with `$dynamicRef`
      !context.uses_dynamic_scopes) {
    for (std::uint64_t index = 0;
         index < schema_context.schema.at(dynamic_context.keyword).size();
         index++) {
      for (auto &&step : compile(
               context, schema_context, dynamic_context,
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return children;
  } else {
    for (std::uint64_t index = 0;
         index < schema_context.schema.at(dynamic_context.keyword).size();
         index++) {
      for (auto &&step : compile(
               context, schema_context,
               relative_dynamic_context(dynamic_context),
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_applicator_anyof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Instructions disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make(
        sourcemeta::blaze::InstructionIndex::ControlGroup, context,
        schema_context, relative_dynamic_context(dynamic_context), ValueNone{},
        compile(
            context, schema_context, relative_dynamic_context(dynamic_context),
            {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})));
  }

  if (context.mode == Mode::FastValidation &&
      std::all_of(disjunctors.cbegin(), disjunctors.cend(),
                  [](const auto &instruction) {
                    return instruction.children.size() == 1 &&
                           (instruction.children.front().type ==
                                sourcemeta::blaze::InstructionIndex::
                                    AssertionTypeStrict ||
                            instruction.children.front().type ==
                                sourcemeta::blaze::InstructionIndex::
                                    AssertionTypeStrictAny);
                  })) {
    ValueTypes types;
    for (const auto &instruction : disjunctors) {
      if (instruction.children.front().type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStrict) {
        const auto &value{
            *std::get_if<ValueType>(&instruction.children.front().value)};
        types.push_back(value);
      }

      if (instruction.children.front().type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny) {
        const auto &value{
            *std::get_if<ValueTypes>(&instruction.children.front().value)};
        for (const auto type : value) {
          types.push_back(type);
        }
      }
    }

    assert(!types.empty());
    if (types.size() > 1) {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, std::move(types))};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   ValueType{*types.cbegin()})};
    }
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 requires_evaluation(context, schema_context)};

  return {make(sourcemeta::blaze::InstructionIndex::LogicalOr, context,
               schema_context, dynamic_context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

auto compiler_draft4_applicator_oneof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());
  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Instructions disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make(
        sourcemeta::blaze::InstructionIndex::ControlGroup, context,
        schema_context, relative_dynamic_context(dynamic_context), ValueNone{},
        compile(
            context, schema_context, relative_dynamic_context(dynamic_context),
            {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})));
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 requires_evaluation(context, schema_context)};

  return {make(sourcemeta::blaze::InstructionIndex::LogicalXor, context,
               schema_context, dynamic_context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

// There are two ways to compile `properties` depending on whether
// most of the properties are marked as required using `required`
// or whether most of the properties are optional. Each shines
// in the corresponding case.
auto properties_as_loop(const Context &context,
                        const SchemaContext &schema_context,
                        const sourcemeta::core::JSON &properties) -> bool {
  const auto size{properties.size()};
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
          properties.defines(property.to_string())) {
        required.insert(property.to_string());
      }
    }
  }

  const auto &current_entry{static_frame_entry(context, schema_context)};
  const auto inside_disjunctor{
      is_inside_disjunctor(schema_context.relative_pointer) ||
      // Check if any reference from `anyOf` or `oneOf` points to us
      std::any_of(context.frame.references().cbegin(),
                  context.frame.references().cend(),
                  [&context, &current_entry](const auto &reference) {
                    if (!context.frame.locations().contains(
                            {sourcemeta::core::SchemaReferenceType::Static,
                             reference.second.destination})) {
                      return false;
                    }

                    const auto &target{
                        context.frame.locations()
                            .at({sourcemeta::core::SchemaReferenceType::Static,
                                 reference.second.destination})
                            .pointer};
                    return is_inside_disjunctor(reference.first.second) &&
                           current_entry.pointer.initial() == target;
                  })};

  if (!inside_disjunctor &&
      schema_context.schema.defines("additionalProperties") &&
      schema_context.schema.at("additionalProperties").is_boolean() &&
      !schema_context.schema.at("additionalProperties").to_boolean() &&
      // If all properties are required, we should still unroll
      required.size() < size) {
    return true;
  }

  return
      // This strategy only makes sense if most of the properties are "optional"
      required.size() <= (size / 4) &&
      // If `properties` only defines a relatively small amount of properties,
      // then its probably still faster to unroll
      size > 5 &&
      // Always unroll inside `oneOf` or `anyOf`, to have a
      // better chance at quickly short-circuiting
      (!inside_disjunctor ||
       std::none_of(properties.as_object().cbegin(),
                    properties.as_object().cend(), [&](const auto &pair) {
                      return pair.second.is_object() &&
                             ((imports_validation_vocabulary &&
                               pair.second.defines("enum")) ||
                              (imports_const && pair.second.defines("const")));
                    }));
}

auto compiler_draft4_applicator_properties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &current,
    const bool annotate, const bool track_evaluation) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  if (properties_as_loop(context, schema_context,
                         schema_context.schema.at(dynamic_context.keyword))) {
    ValueNamedIndexes indexes;
    Instructions children;
    std::size_t cursor = 0;

    for (auto &&[name, substeps] : compile_properties(
             context, schema_context, relative_dynamic_context(dynamic_context),
             current)) {
      indexes.assign(name, cursor);

      if (track_evaluation) {
        substeps.push_back(
            make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
                 schema_context, relative_dynamic_context(dynamic_context),
                 ValuePointer{name}));
      }

      if (annotate) {
        substeps.push_back(
            make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
                 schema_context, relative_dynamic_context(dynamic_context),
                 sourcemeta::core::JSON{name}));
      }

      // Note that the evaluator completely ignores this wrapper anyway
      children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                              context, schema_context,
                              relative_dynamic_context(dynamic_context),
                              ValueNone{}, std::move(substeps)));
      cursor += 1;
    }

    if (context.mode == Mode::FastValidation && !track_evaluation &&
        !schema_context.schema.defines("patternProperties") &&
        schema_context.schema.defines("additionalProperties") &&
        schema_context.schema.at("additionalProperties").is_boolean() &&
        !schema_context.schema.at("additionalProperties").to_boolean()) {
      return {
          make(sourcemeta::blaze::InstructionIndex::LoopPropertiesMatchClosed,
               context, schema_context, dynamic_context, std::move(indexes),
               std::move(children))};
    }

    return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesMatch,
                 context, schema_context, dynamic_context, std::move(indexes),
                 std::move(children))};
  }

  Instructions children;

  const auto effective_dynamic_context{
      context.mode == Mode::FastValidation
          ? dynamic_context
          : relative_dynamic_context(dynamic_context)};

  const auto assume_object{schema_context.schema.defines("type") &&
                           schema_context.schema.at("type").is_string() &&
                           schema_context.schema.at("type").to_string() ==
                               "object"};

  auto properties{compile_properties(context, schema_context,
                                     effective_dynamic_context, current)};

  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("additionalProperties") &&
      schema_context.schema.at("additionalProperties").is_boolean() &&
      !schema_context.schema.at("additionalProperties").to_boolean() &&
      // TODO: Check that the validation vocabulary is present
      schema_context.schema.defines("required") &&
      schema_context.schema.at("required").is_array() &&
      schema_context.schema.at("required").size() ==
          schema_context.schema.at(dynamic_context.keyword).size() &&
      std::all_of(properties.cbegin(), properties.cend(),
                  [&schema_context](const auto &property) {
                    return schema_context.schema.at("required")
                        .contains(sourcemeta::core::JSON{property.first});
                  })) {
    if (std::all_of(properties.cbegin(), properties.cend(),
                    [](const auto &property) {
                      return property.second.size() == 1 &&
                             property.second.front().type ==
                                 InstructionIndex::AssertionTypeStrict;
                    })) {
      std::set<ValueType> types;
      for (const auto &property : properties) {
        types.insert(std::get<ValueType>(property.second.front().value));
      }

      if (types.size() == 1) {
        if (schema_context.schema.defines("required") &&
            !schema_context.schema.defines("patternProperties") &&
            assume_object) {
          auto required_copy = schema_context.schema.at("required");
          std::sort(required_copy.as_array().begin(),
                    required_copy.as_array().end());
          ValueStringSet required{json_array_to_string_set(required_copy)};
          if (is_closed_properties_required(schema_context.schema, required)) {
            sourcemeta::core::PropertyHashJSON<ValueString> hasher;
            std::vector<std::pair<ValueString, ValueStringSet::hash_type>>
                perfect_hashes;
            for (const auto &entry : required) {
              assert(required.contains(entry.first, entry.second));
              if (hasher.is_perfect(entry.second)) {
                perfect_hashes.emplace_back(entry.first, entry.second);
              }
            }

            if (perfect_hashes.size() == required.size()) {
              return {make(sourcemeta::blaze::InstructionIndex::
                               LoopPropertiesExactlyTypeStrictHash,
                           context, schema_context, dynamic_context,
                           ValueTypedHashes{*types.cbegin(),
                                            to_string_hashes(perfect_hashes)})};
            }

            return {make(
                sourcemeta::blaze::InstructionIndex::
                    LoopPropertiesExactlyTypeStrict,
                context, schema_context, dynamic_context,
                ValueTypedProperties{*types.cbegin(), std::move(required)})};
          }
        }

        return {
            make(sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrict,
                 context, schema_context, dynamic_context, *types.cbegin())};
      }
    }

    if (std::all_of(properties.cbegin(), properties.cend(),
                    [](const auto &property) {
                      return property.second.size() == 1 &&
                             property.second.front().type ==
                                 InstructionIndex::AssertionType;
                    })) {
      std::set<ValueType> types;
      for (const auto &property : properties) {
        types.insert(std::get<ValueType>(property.second.front().value));
      }

      if (types.size() == 1) {
        return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesType,
                     context, schema_context, dynamic_context,
                     *types.cbegin())};
      }
    }
  }

  for (auto &&[name, substeps] : properties) {
    if (annotate) {
      substeps.push_back(
          make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
               schema_context, effective_dynamic_context,
               sourcemeta::core::JSON{name}));
    }

    // Optimize `properties` where its subschemas just include a type check,
    // as that's a very common pattern

    if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
        substeps.front().type == InstructionIndex::AssertionTypeStrict) {
      const auto &type_step{substeps.front()};
      if (track_evaluation) {
        children.push_back(rephrase(sourcemeta::blaze::InstructionIndex::
                                        AssertionPropertyTypeStrictEvaluate,
                                    type_step));
      } else {
        children.push_back(rephrase(
            sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeStrict,
            type_step));
      }
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               substeps.front().type == InstructionIndex::AssertionType) {
      const auto &type_step{substeps.front()};
      if (track_evaluation) {
        children.push_back(rephrase(
            sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeEvaluate,
            type_step));
      } else {
        children.push_back(
            rephrase(sourcemeta::blaze::InstructionIndex::AssertionPropertyType,
                     type_step));
      }
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionTypeStrictAny) {
      const auto &type_step{substeps.front()};
      if (track_evaluation) {
        children.push_back(rephrase(sourcemeta::blaze::InstructionIndex::
                                        AssertionPropertyTypeStrictAnyEvaluate,
                                    type_step));
      } else {
        children.push_back(rephrase(
            sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeStrictAny,
            type_step));
      }

    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyTypeStrict) {
      children.push_back(unroll(
          substeps.front(), effective_dynamic_context.base_instance_location));
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyType) {
      children.push_back(unroll(
          substeps.front(), effective_dynamic_context.base_instance_location));
    } else if (context.mode == Mode::FastValidation && substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyTypeStrictAny) {
      children.push_back(unroll(
          substeps.front(), effective_dynamic_context.base_instance_location));

    } else {
      if (track_evaluation) {
        if (context.mode == Mode::FastValidation) {
          // We need this wrapper as `ControlEvaluate` doesn't push to the stack
          substeps.push_back(
              make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
                   schema_context, effective_dynamic_context, ValueNone{},
                   {make(sourcemeta::blaze::InstructionIndex::ControlEvaluate,
                         context, schema_context, effective_dynamic_context,
                         ValuePointer{name})}));
        } else {
          substeps.push_back(make(
              sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
              schema_context, effective_dynamic_context, ValuePointer{name}));
        }
      }

      if (!substeps.empty()) {
        // As a performance shortcut
        if (effective_dynamic_context.base_instance_location.empty()) {
          if (assume_object &&
              // TODO: Check that the validation vocabulary is present
              schema_context.schema.defines("required") &&
              schema_context.schema.at("required").is_array() &&
              schema_context.schema.at("required")
                  .contains(sourcemeta::core::JSON{name})) {
            for (auto &&step : substeps) {
              children.push_back(std::move(step));
            }
          } else {
            children.push_back(make(sourcemeta::blaze::InstructionIndex::
                                        ControlGroupWhenDefinesDirect,
                                    context, schema_context,
                                    effective_dynamic_context,
                                    make_property(name), std::move(substeps)));
          }
        } else {
          children.push_back(
              make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenDefines,
                   context, schema_context, effective_dynamic_context,
                   make_property(name), std::move(substeps)));
        }
      }
    }
  }

  if (context.mode == Mode::FastValidation) {
    return children;
  } else if (children.empty()) {
    return {};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_applicator_properties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &current)
    -> Instructions {
  return compiler_draft4_applicator_properties_with_options(
      context, schema_context, dynamic_context, current, false, false);
}

auto compiler_draft4_applicator_patternproperties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Instructions children;

  // To guarantee ordering
  std::vector<std::string> patterns;
  for (auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    patterns.push_back(entry.first);
  }

  std::sort(patterns.begin(), patterns.end());

  // For each regular expression and corresponding subschema in the object
  for (const auto &pattern : patterns) {
    auto substeps{compile(context, schema_context,
                          relative_dynamic_context(dynamic_context), {pattern},
                          {})};

    if (annotate) {
      substeps.push_back(
          make(sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent,
               context, schema_context,
               relative_dynamic_context(dynamic_context), ValueNone{}));
    }

    if (track_evaluation) {
      substeps.push_back(
          make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
               schema_context, relative_dynamic_context(dynamic_context),
               ValuePointer{}));
    }

    if (context.mode == Mode::FastValidation && !track_evaluation &&
        patterns.size() == 1 && !schema_context.schema.defines("properties") &&
        schema_context.schema.defines("additionalProperties") &&
        schema_context.schema.at("additionalProperties").is_boolean() &&
        !schema_context.schema.at("additionalProperties").to_boolean()) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::LoopPropertiesRegexClosed,
               context, schema_context, dynamic_context,
               ValueRegex{parse_regex(pattern, schema_context.base,
                                      schema_context.relative_pointer),
                          pattern},
               std::move(substeps)));

      // If the `patternProperties` subschema for the given pattern does
      // nothing, then we can avoid generating an entire loop for it
    } else if (!substeps.empty()) {
      const auto maybe_prefix{pattern_as_prefix(pattern)};
      if (maybe_prefix.has_value()) {
        children.push_back(
            make(sourcemeta::blaze::InstructionIndex::LoopPropertiesStartsWith,
                 context, schema_context, dynamic_context,
                 ValueString{maybe_prefix.value()}, std::move(substeps)));
      } else {
        children.push_back(
            make(sourcemeta::blaze::InstructionIndex::LoopPropertiesRegex,
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
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft4_applicator_patternproperties_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_applicator_additionalproperties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Instructions children{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  if (annotate) {
    children.push_back(
        make(sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent,
             context, schema_context, relative_dynamic_context(dynamic_context),
             ValueNone{}));
  }

  ValueStringSet filter_strings;
  ValueStrings filter_prefixes;
  std::vector<ValueRegex> filter_regexes;

  if (schema_context.schema.defines("properties") &&
      schema_context.schema.at("properties").is_object()) {
    for (const auto &entry :
         schema_context.schema.at("properties").as_object()) {
      filter_strings.insert(entry.first);
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
      children.front().type == InstructionIndex::AssertionFail &&
      !filter_strings.empty() && filter_prefixes.empty() &&
      filter_regexes.empty()) {
    if (properties_as_loop(context, schema_context,
                           schema_context.schema.at("properties"))) {
      return {};
    } else if (!children.empty() &&
               children.front().type == InstructionIndex::AssertionFail) {
      return {};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesWhitelist,
                   context, schema_context, dynamic_context,
                   std::move(filter_strings))};
    }
  } else if (context.mode == Mode::FastValidation && filter_strings.empty() &&
             filter_prefixes.empty() && filter_regexes.size() == 1 &&
             !track_evaluation && !children.empty() &&
             children.front().type == InstructionIndex::AssertionFail) {
    return {};
  }

  if (!filter_strings.empty() || !filter_prefixes.empty() ||
      !filter_regexes.empty()) {
    if (track_evaluation) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
               schema_context, relative_dynamic_context(dynamic_context),
               ValuePointer{}));
    }

    return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesExcept,
                 context, schema_context, dynamic_context,
                 ValuePropertyFilter{std::move(filter_strings),
                                     std::move(filter_prefixes),
                                     std::move(filter_regexes)},
                 std::move(children))};

    // Optimize `additionalProperties` set to just `type`, which is a
    // pretty common pattern
  } else if (context.mode == Mode::FastValidation && children.size() == 1 &&
             children.front().type == InstructionIndex::AssertionTypeStrict) {
    const auto &type_step{children.front()};
    if (track_evaluation) {
      return {make(
          sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrictEvaluate,
          context, schema_context, dynamic_context, type_step.value)};
    } else {
      return {
          make(sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrict,
               context, schema_context, dynamic_context, type_step.value)};
    }
  } else if (context.mode == Mode::FastValidation && children.size() == 1 &&
             children.front().type == InstructionIndex::AssertionType) {
    const auto &type_step{children.front()};
    if (track_evaluation) {
      return {
          make(sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeEvaluate,
               context, schema_context, dynamic_context, type_step.value)};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesType,
                   context, schema_context, dynamic_context, type_step.value)};
    }
  } else if (context.mode == Mode::FastValidation && children.size() == 1 &&
             children.front().type ==
                 InstructionIndex::AssertionTypeStrictAny) {
    const auto &type_step{children.front()};
    if (track_evaluation) {
      return {make(sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesTypeStrictAnyEvaluate,
                   context, schema_context, dynamic_context, type_step.value)};
    } else {
      return {
          make(sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrictAny,
               context, schema_context, dynamic_context, type_step.value)};
    }

  } else if (track_evaluation) {
    if (children.empty()) {
      return {make(sourcemeta::blaze::InstructionIndex::ControlEvaluate,
                   context, schema_context, dynamic_context, ValuePointer{})};
    }

    return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesEvaluate,
                 context, schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  } else if (children.size() == 1 &&
             children.front().type == InstructionIndex::AssertionFail) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionObjectSizeLess,
                 context, schema_context, dynamic_context,
                 ValueUnsignedInteger{1})};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::LoopProperties, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_applicator_additionalproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft4_applicator_additionalproperties_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_validation_pattern(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_string());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  const auto &regex_string{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  return {make(sourcemeta::blaze::InstructionIndex::AssertionRegex, context,
               schema_context, dynamic_context,
               ValueRegex{parse_regex(regex_string, schema_context.base,
                                      schema_context.relative_pointer),
                          regex_string})};
}

auto compiler_draft4_validation_format(const Context &context,
                                       const SchemaContext &schema_context,
                                       const DynamicContext &dynamic_context,
                                       const Instructions &) -> Instructions {
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
    return {make(sourcemeta::blaze::InstructionIndex::AssertionStringType,
                 context, schema_context, dynamic_context,
                 ValueStringType::URI)};
  }

#define COMPILE_FORMAT_REGEX(name, regular_expression)                         \
  if (format == (name)) {                                                      \
    return {                                                                   \
        make(sourcemeta::blaze::InstructionIndex::AssertionRegex, context,     \
             schema_context, dynamic_context,                                  \
             ValueRegex{parse_regex(regular_expression, schema_context.base,   \
                                    schema_context.relative_pointer),          \
                        (regular_expression)})};                               \
  }

  COMPILE_FORMAT_REGEX("ipv4", FORMAT_REGEX_IPV4)

#undef COMPILE_FORMAT_REGEX

  return {};
}

auto compiler_draft4_applicator_not(const Context &context,
                                    const SchemaContext &schema_context,
                                    const DynamicContext &dynamic_context,
                                    const Instructions &) -> Instructions {
  std::size_t subschemas{0};
  for (const auto &subschema :
       walk_subschemas(context, schema_context, dynamic_context)) {
    if (subschema.pointer.empty()) {
      continue;
    }

    subschemas += 1;
  }

  Instructions children{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track_items{
      std::any_of(context.unevaluated.cbegin(), context.unevaluated.cend(),
                  [](const auto &dependency) {
                    return dependency.first.ends_with("unevaluatedItems");
                  })};

  // Only emit a `not` instruction that keeps track of
  // evaluation if we really need it. If the "not" subschema
  // does not define applicators, then that's an easy case
  // we can skip
  if (subschemas > 0 &&
      (requires_evaluation(context, schema_context) || track_items)) {
    return {make(sourcemeta::blaze::InstructionIndex::LogicalNotEvaluate,
                 context, schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::LogicalNot, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_applicator_items_array(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
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
  std::vector<Instructions> subschemas;
  subschemas.reserve(items_size);
  const auto &array{
      schema_context.schema.at(dynamic_context.keyword).as_array()};
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    subschemas.push_back(compile(context, schema_context,
                                 relative_dynamic_context(dynamic_context),
                                 {subschemas.size()}, {subschemas.size()}));
  }

  Instructions children;
  for (std::size_t cursor = 0; cursor < items_size; cursor++) {
    Instructions subchildren;
    for (std::size_t index = 0; index < cursor + 1; index++) {
      for (const auto &substep : subschemas.at(index)) {
        subchildren.push_back(substep);
      }
    }

    if (annotate) {
      subchildren.push_back(
          make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
               schema_context, relative_dynamic_context(dynamic_context),
               sourcemeta::core::JSON{cursor}));
    }

    children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                            context, schema_context,
                            relative_dynamic_context(dynamic_context),
                            ValueNone{}, std::move(subchildren)));
  }

  Instructions tail;
  for (const auto &subschema : subschemas) {
    for (const auto &substep : subschema) {
      tail.push_back(substep);
    }
  }

  if (annotate) {
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                        context, schema_context,
                        relative_dynamic_context(dynamic_context),
                        sourcemeta::core::JSON{children.size() - 1}));
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                        context, schema_context,
                        relative_dynamic_context(dynamic_context),
                        sourcemeta::core::JSON{true}));
  }

  children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                          context, schema_context,
                          relative_dynamic_context(dynamic_context),
                          ValueNone{}, std::move(tail)));

  if (track_evaluation) {
    return {
        make(sourcemeta::blaze::InstructionIndex::AssertionArrayPrefixEvaluate,
             context, schema_context, dynamic_context, ValueNone{},
             std::move(children))};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionArrayPrefix,
                 context, schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_applicator_items_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    if (annotate || track_evaluation) {
      Instructions subchildren{compile(
          context, schema_context, relative_dynamic_context(dynamic_context),
          sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

      Instructions children;

      if (!subchildren.empty()) {
        children.push_back(make(sourcemeta::blaze::InstructionIndex::LoopItems,
                                context, schema_context, dynamic_context,
                                ValueNone{}, std::move(subchildren)));
      }

      if (!annotate && !track_evaluation) {
        return children;
      }

      Instructions tail;

      if (annotate) {
        tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                            context, schema_context,
                            relative_dynamic_context(dynamic_context),
                            sourcemeta::core::JSON{true}));
      }

      if (track_evaluation) {
        tail.push_back(
            make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
                 schema_context, relative_dynamic_context(dynamic_context),
                 ValuePointer{}));
      }

      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::LogicalWhenType, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON::Type::Array, std::move(tail)));

      return children;
    }

    Instructions children{compile(
        context, schema_context, relative_dynamic_context(dynamic_context),
        sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};
    if (track_evaluation) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
               schema_context, relative_dynamic_context(dynamic_context),
               ValuePointer{}));
    }

    if (children.empty()) {
      return {};
    }

    if (context.mode == Mode::FastValidation && children.size() == 1) {
      if (children.front().type == InstructionIndex::AssertionTypeStrict) {
        return {make(sourcemeta::blaze::InstructionIndex::LoopItemsTypeStrict,
                     context, schema_context, dynamic_context,
                     children.front().value)};
      } else if (children.front().type == InstructionIndex::AssertionType) {
        return {make(sourcemeta::blaze::InstructionIndex::LoopItemsType,
                     context, schema_context, dynamic_context,
                     children.front().value)};
      } else if (children.front().type ==
                 InstructionIndex::AssertionTypeStrictAny) {
        return {make(
            sourcemeta::blaze::InstructionIndex::LoopItemsTypeStrictAny,
            context, schema_context, dynamic_context, children.front().value)};
      } else if (children.front().type ==
                 InstructionIndex::LoopPropertiesExactlyTypeStrictHash) {
        auto value_copy = children.front().value;
        auto current{make(sourcemeta::blaze::InstructionIndex::LoopItems,
                          context, schema_context, dynamic_context, ValueNone{},
                          std::move(children))};
        if (std::get<ValueTypedHashes>(value_copy).second.first.size() == 3) {
          return {{sourcemeta::blaze::InstructionIndex::
                       LoopItemsPropertiesExactlyTypeStrictHash3,
                   current.relative_schema_location,
                   current.relative_instance_location,
                   current.keyword_location,
                   current.schema_resource,
                   std::move(value_copy),
                   {}}};
        }

        return {{sourcemeta::blaze::InstructionIndex::
                     LoopItemsPropertiesExactlyTypeStrictHash,
                 current.relative_schema_location,
                 current.relative_instance_location,
                 current.keyword_location,
                 current.schema_resource,
                 std::move(value_copy),
                 {}}};
      }
    }

    return {make(sourcemeta::blaze::InstructionIndex::LoopItems, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }

  return compiler_draft4_applicator_items_array(
      context, schema_context, dynamic_context, annotate, track_evaluation);
}

auto compiler_draft4_applicator_items(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  return compiler_draft4_applicator_items_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_applicator_additionalitems_from_cursor(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const std::size_t cursor,
    const bool annotate, const bool track_evaluation) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  Instructions subchildren{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  Instructions children;

  if (!subchildren.empty()) {
    children.push_back(make(sourcemeta::blaze::InstructionIndex::LoopItemsFrom,
                            context, schema_context, dynamic_context,
                            ValueUnsignedInteger{cursor},
                            std::move(subchildren)));
  }

  // Avoid one extra wrapper instruction if possible
  if (!annotate && !track_evaluation) {
    return children;
  }

  Instructions tail;

  if (annotate) {
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                        context, schema_context,
                        relative_dynamic_context(dynamic_context),
                        sourcemeta::core::JSON{true}));
  }

  if (track_evaluation) {
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::ControlEvaluate,
                        context, schema_context,
                        relative_dynamic_context(dynamic_context),
                        ValuePointer{}));
  }

  assert(!tail.empty());
  children.push_back(
      make(sourcemeta::blaze::InstructionIndex::LogicalWhenArraySizeGreater,
           context, schema_context, dynamic_context,
           ValueUnsignedInteger{cursor}, std::move(tail)));

  return children;
}

auto compiler_draft4_applicator_additionalitems_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
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
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft4_applicator_additionalitems_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft4_applicator_dependencies(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_object());
  Instructions children;
  ValueStringMap dependencies;

  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (is_schema(entry.second)) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(make(
            sourcemeta::blaze::InstructionIndex::LogicalWhenDefines, context,
            schema_context, dynamic_context, make_property(entry.first),
            compile(context, schema_context,
                    relative_dynamic_context(dynamic_context), {entry.first},
                    sourcemeta::core::empty_pointer)));
      }
    } else if (entry.second.is_array()) {
      std::vector<sourcemeta::core::JSON::String> properties;
      for (const auto &property : entry.second.as_array()) {
        assert(property.is_string());
        properties.push_back(property.to_string());
      }

      if (!properties.empty()) {
        dependencies.assign(entry.first, std::move(properties));
      }
    }
  }

  if (!dependencies.empty()) {
    children.push_back(make(
        sourcemeta::blaze::InstructionIndex::AssertionPropertyDependencies,
        context, schema_context, dynamic_context, std::move(dependencies)));
  }

  return children;
}

auto compiler_draft4_validation_enum(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context,
                                     const Instructions &) -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_array());

  if (schema_context.schema.at(dynamic_context.keyword).size() == 1) {
    return {
        make(sourcemeta::blaze::InstructionIndex::AssertionEqual, context,
             schema_context, dynamic_context,
             sourcemeta::core::JSON{
                 schema_context.schema.at(dynamic_context.keyword).front()})};
  }

  std::vector<std::pair<sourcemeta::blaze::ValueString,
                        sourcemeta::blaze::ValueStringSet::hash_type>>
      perfect_string_hashes;
  ValueSet options;
  sourcemeta::core::PropertyHashJSON<ValueString> hasher;
  for (const auto &option :
       schema_context.schema.at(dynamic_context.keyword).as_array()) {
    if (option.is_string()) {
      const auto hash{hasher(option.to_string())};
      if (hasher.is_perfect(hash)) {
        perfect_string_hashes.emplace_back(option.to_string(), hash);
      }
    }

    options.insert(option);
  }

  // Only apply this optimisation on fast validation, as it
  // can affect error messages
  if (context.mode == Mode::FastValidation &&
      perfect_string_hashes.size() == options.size()) {
    return {
        make(sourcemeta::blaze::InstructionIndex::AssertionEqualsAnyStringHash,
             context, schema_context, dynamic_context,
             to_string_hashes(perfect_string_hashes))};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionEqualsAny, context,
               schema_context, dynamic_context, std::move(options))};
}

auto compiler_draft4_validation_uniqueitems(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_boolean() ||
      !schema_context.schema.at(dynamic_context.keyword).to_boolean()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionUnique, context,
               schema_context, dynamic_context, ValueNone{})};
}

auto compiler_draft4_validation_maxlength(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context,
                                          const Instructions &)
    -> Instructions {
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

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionStringSizeLess, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minlength(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context,
                                          const Instructions &)
    -> Instructions {
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

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionStringSizeGreater, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maxitems(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &) -> Instructions {
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

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionArraySizeLess, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minitems(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &) -> Instructions {
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

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionArraySizeGreater, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maxproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
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

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeLess, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
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

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeGreater, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) -
          1})};
}

auto compiler_draft4_validation_maximum(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &) -> Instructions {
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
    return {make(sourcemeta::blaze::InstructionIndex::AssertionLess, context,
                 schema_context, dynamic_context,
                 sourcemeta::core::JSON{
                     schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionLessEqual,
                 context, schema_context, dynamic_context,
                 sourcemeta::core::JSON{
                     schema_context.schema.at(dynamic_context.keyword)})};
  }
}

auto compiler_draft4_validation_minimum(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &) -> Instructions {
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
    return {make(sourcemeta::blaze::InstructionIndex::AssertionGreater, context,
                 schema_context, dynamic_context,
                 sourcemeta::core::JSON{
                     schema_context.schema.at(dynamic_context.keyword)})};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionGreaterEqual,
                 context, schema_context, dynamic_context,
                 sourcemeta::core::JSON{
                     schema_context.schema.at(dynamic_context.keyword)})};
  }
}

auto compiler_draft4_validation_multipleof(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());
  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionDivisible, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON{
                   schema_context.schema.at(dynamic_context.keyword)})};
}

} // namespace internal
#endif
