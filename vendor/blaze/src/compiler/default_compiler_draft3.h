#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT3_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT3_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/regex.h>

#include <algorithm> // std::sort, std::ranges::any_of, std::ranges::all_of, std::find_if, std::ranges::none_of
#include <cassert>   // assert
#include <set>       // std::set
#include <utility>   // std::move, std::to_underlying

#include "compile_helpers.h"

static auto parse_regex(const std::string &pattern,
                        const sourcemeta::core::URI &base,
                        const sourcemeta::core::WeakPointer &schema_location)
    -> sourcemeta::core::Regex {
  const auto result{sourcemeta::core::to_regex(pattern)};
  if (!result.has_value()) [[unlikely]] {
    throw sourcemeta::blaze::CompilerInvalidRegexError(
        base, to_pointer(schema_location), pattern);
  }

  return result.value();
}

static auto
relative_schema_location_size(const sourcemeta::blaze::Context &context,
                              const sourcemeta::blaze::Instruction &step)
    -> std::size_t {
  return context.extra[step.extra_index].relative_schema_location.size();
}

static auto
defines_direct_enumeration(const sourcemeta::blaze::Instructions &steps)
    -> std::optional<std::size_t> {
  const auto iterator{std::ranges::find_if(steps, [](const auto &step) -> auto {
    return step.type == sourcemeta::blaze::InstructionIndex::AssertionEqual ||
           step.type == sourcemeta::blaze::InstructionIndex::AssertionEqualsAny;
  })};

  if (iterator == steps.cend()) {
    return std::nullopt;
  }

  return std::distance(steps.cbegin(), iterator);
}

static auto is_inside_disjunctor(const sourcemeta::core::WeakPointer &pointer)
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
  return !schema.defines("patternProperties") &&
         schema.defines("additionalProperties") &&
         schema.at("additionalProperties").is_boolean() &&
         !schema.at("additionalProperties").to_boolean() &&
         schema.defines("properties") && schema.at("properties").is_object() &&
         schema.at("properties").size() == required.size() &&
         std::ranges::all_of(required, [&schema](const auto &property) -> auto {
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
    properties.emplace_back(
        entry.first,
        compile(context, schema_context, dynamic_context,
                sourcemeta::blaze::make_weak_pointer(entry.first),
                sourcemeta::blaze::make_weak_pointer(entry.first)));
  }

  // In many cases, `properties` have some subschemas that are small
  // and some subschemas that are large. To attempt to improve performance,
  // we prefer to evaluate smaller subschemas first, in the hope of failing
  // earlier without spending a lot of time on other subschemas
  if (context.tweaks.properties_reorder) {
    std::ranges::sort(
        properties, [&context](const auto &left, const auto &right) -> auto {
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
                         context,
                         left.second.at(left_direct_enumeration.value())) <
                     relative_schema_location_size(
                         context,
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
  }

  return properties;
}

static auto to_string_hashes(
    std::vector<std::pair<sourcemeta::blaze::ValueString,
                          sourcemeta::blaze::ValueStringSet::hash_type>>
        &hashes) -> sourcemeta::blaze::ValueStringHashes {
  assert(!hashes.empty());
  std::ranges::sort(hashes, [](const auto &left, const auto &right) -> auto {
    return left.first.size() < right.first.size();
  });

  sourcemeta::blaze::ValueStringHashes result;
  // The idea with the table of contents is as follows: each index
  // marks the starting and end positions for a string where the size
  // is equal to the index.
  result.second.resize(hashes.back().first.size() + 1, std::make_pair(0, 0));
  // TODO(C++23): Use std::views::enumerate when available in libc++
  for (std::size_t index = 0; index < hashes.size(); index++) {
    result.first.emplace_back(hashes[index].second, hashes[index].first);
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

auto compile_required_assertions(const Context &context,
                                 const SchemaContext &schema_context,
                                 const DynamicContext &dynamic_context,
                                 const Instructions &current,
                                 ValueStringSet properties_set)
    -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  const auto assume_object{schema_context.schema.defines("type") &&
                           schema_context.schema.at("type").is_string() &&
                           schema_context.schema.at("type").to_string() ==
                               "object"};

  if (properties_set.empty()) {
    return {};
  } else if (properties_set.size() > 1) {
    if (is_closed_properties_required(schema_context.schema, properties_set)) {
      if (context.mode == Mode::FastValidation && assume_object) {
        static const std::string properties_keyword{"properties"};
        // `SchemaContext::relative_pointer` is a reference, so the concatenated
        // pointer must outlive `new_schema_context`
        const auto properties_pointer{
            schema_context.relative_pointer.initial().concat(
                sourcemeta::blaze::make_weak_pointer(properties_keyword))};
        const SchemaContext new_schema_context{
            .relative_pointer = properties_pointer,
            .schema = schema_context.schema,
            .vocabularies = schema_context.vocabularies,
            .base = schema_context.base,
            .is_property_name = schema_context.is_property_name};
        const DynamicContext new_dynamic_context{
            .keyword = KEYWORD_PROPERTIES,
            .base_schema_location = sourcemeta::core::empty_weak_pointer,
            .base_instance_location = sourcemeta::core::empty_weak_pointer};
        auto properties{compile_properties(context, new_schema_context,
                                           new_dynamic_context, current)};
        if (std::ranges::all_of(properties, [](const auto &property) -> auto {
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
            std::ranges::all_of(properties_set,
                                [&hasher](const auto &property) -> auto {
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
    } else if (assume_object) {
      return {make(
          sourcemeta::blaze::InstructionIndex::AssertionDefinesAllStrict,
          context, schema_context, dynamic_context, std::move(properties_set))};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionDefinesAll,
                   context, schema_context, dynamic_context,
                   std::move(properties_set))};
    }
  } else if (assume_object) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionDefinesStrict,
                 context, schema_context, dynamic_context,
                 make_property(properties_set.begin()->first))};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionDefines, context,
                 schema_context, dynamic_context,
                 make_property(properties_set.begin()->first))};
  }
}

auto compiler_draft3_core_ref(const Context &context,
                              const SchemaContext &schema_context,
                              const DynamicContext &dynamic_context,
                              const Instructions &) -> Instructions {
  const auto &entry{static_frame_entry(context, schema_context)};
  const auto type{sourcemeta::blaze::SchemaReferenceType::Static};
  const auto reference{context.frame.reference(type, entry.pointer)};
  if (!reference.has_value()) [[unlikely]] {
    throw sourcemeta::blaze::SchemaReferenceError(
        schema_context.schema.at(dynamic_context.keyword).to_string(),
        to_pointer(schema_context.relative_pointer),
        "Could not resolve schema reference");
  }

  const auto key{std::make_tuple(type,
                                 std::string_view{reference->get().destination},
                                 schema_context.is_property_name)};
  assert(context.targets.contains(key));
  return {make(sourcemeta::blaze::InstructionIndex::ControlJump, context,
               schema_context, dynamic_context,
               ValueUnsignedInteger{context.targets.at(key).first})};
}

// There are two ways to compile `properties` depending on whether
// most of the properties are marked as required using `required`
// or whether most of the properties are optional. Each shines
// in the corresponding case.
auto properties_as_loop(const Context &context,
                        const SchemaContext &schema_context,
                        const sourcemeta::core::JSON &properties) -> bool {
  if (context.tweaks.properties_always_unroll) {
    return false;
  }

  using Known = sourcemeta::blaze::Vocabularies::Known;
  const auto size{properties.size()};
  const auto imports_validation_vocabulary =
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_4) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_6) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_7) ||
      schema_context.vocabularies.contains(
          Known::JSON_Schema_2019_09_Validation) ||
      schema_context.vocabularies.contains(
          Known::JSON_Schema_2020_12_Validation);
  const auto imports_const =
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_6) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_7) ||
      schema_context.vocabularies.contains(
          Known::JSON_Schema_2019_09_Validation) ||
      schema_context.vocabularies.contains(
          Known::JSON_Schema_2020_12_Validation);
  ValueStringSet required;
  for (const auto &entry : required_properties(schema_context)) {
    // Only count the required property if its indeed in "properties"
    if (properties.defines(entry.first)) {
      required.insert(entry.first);
    }
  }

  const auto &current_entry{static_frame_entry(context, schema_context)};
  const auto inside_disjunctor{
      is_inside_disjunctor(schema_context.relative_pointer) ||
      // Check if any reference from `anyOf` or `oneOf` points to us
      std::ranges::any_of(
          context.frame.references(),
          [&context, &current_entry](const auto &reference) -> auto {
            if (!context.frame.locations().contains(
                    {sourcemeta::blaze::SchemaReferenceType::Static,
                     reference.second.destination})) {
              return false;
            }

            const auto &target{
                context.frame.locations()
                    .at({sourcemeta::blaze::SchemaReferenceType::Static,
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
       std::ranges::none_of(
           properties.as_object(), [&](const auto &pair) -> auto {
             return pair.second.is_object() &&
                    ((imports_validation_vocabulary &&
                      pair.second.defines("enum")) ||
                     (imports_const && pair.second.defines("const")));
           }));
}

auto draft3_any_type_instructions(const Context &context,
                                  const SchemaContext &schema_context,
                                  const DynamicContext &dynamic_context)
    -> Instructions {
  if (context.mode != Mode::Exhaustive) {
    return {};
  }

  using sourcemeta::core::JSON;
  ValueTypes types{};
  types.set(std::to_underlying(JSON::Type::Null));
  types.set(std::to_underlying(JSON::Type::Boolean));
  types.set(std::to_underlying(JSON::Type::Object));
  types.set(std::to_underlying(JSON::Type::Array));
  types.set(std::to_underlying(JSON::Type::Integer));
  types.set(std::to_underlying(JSON::Type::Real));
  types.set(std::to_underlying(JSON::Type::Decimal));
  types.set(std::to_underlying(JSON::Type::String));
  return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
               context, schema_context, dynamic_context, types)};
}

auto draft3_set_type_bits(const std::string &name, ValueTypes &types) -> bool {
  using sourcemeta::core::JSON;
  if (name == "null") {
    types.set(std::to_underlying(JSON::Type::Null));
  } else if (name == "boolean") {
    types.set(std::to_underlying(JSON::Type::Boolean));
  } else if (name == "object") {
    types.set(std::to_underlying(JSON::Type::Object));
  } else if (name == "array") {
    types.set(std::to_underlying(JSON::Type::Array));
  } else if (name == "integer") {
    types.set(std::to_underlying(JSON::Type::Integer));
  } else if (name == "string") {
    types.set(std::to_underlying(JSON::Type::String));
  } else if (name == "number") {
    types.set(std::to_underlying(JSON::Type::Real));
    types.set(std::to_underlying(JSON::Type::Integer));
    types.set(std::to_underlying(JSON::Type::Decimal));
  } else {
    return false;
  }
  return true;
}

auto is_integer_type_check(const Instruction &instruction) -> bool {
  return (instruction.type == InstructionIndex::AssertionType ||
          instruction.type == InstructionIndex::AssertionTypeStrict) &&
         std::get<ValueType>(instruction.value) ==
             sourcemeta::core::JSON::Type::Integer;
}

auto has_strict_integer_type(const Instructions &children) -> bool {
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionTypeStrict &&
        std::get<ValueType>(child.value) ==
            sourcemeta::core::JSON::Type::Integer) {
      return true;
    }
  }

  return false;
}

auto is_integer_type_bounded_pattern(const Instructions &children) -> bool {
  if (children.size() != 3) {
    return false;
  }

  bool has_type{false};
  bool has_min{false};
  bool has_max{false};
  for (const auto &child : children) {
    if (is_integer_type_check(child)) {
      has_type = true;
    } else if (child.type == InstructionIndex::AssertionGreaterEqual &&
               std::get<ValueJSON>(child.value).is_integer()) {
      has_min = true;
    } else if (child.type == InstructionIndex::AssertionLessEqual &&
               std::get<ValueJSON>(child.value).is_integer()) {
      has_max = true;
    }
  }

  return has_type && has_min && has_max;
}

auto is_integer_type_lower_bound_pattern(const Instructions &children) -> bool {
  if (children.size() != 2) {
    return false;
  }

  bool has_type{false};
  bool has_min{false};
  for (const auto &child : children) {
    if (is_integer_type_check(child)) {
      has_type = true;
    } else if (child.type == InstructionIndex::AssertionGreaterEqual &&
               std::get<ValueJSON>(child.value).is_integer()) {
      has_min = true;
    }
  }

  return has_type && has_min;
}

auto extract_integer_lower_bound(const Instructions &children) -> std::int64_t {
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionGreaterEqual) {
      return std::get<ValueJSON>(child.value).to_integer();
    }
  }

  return 0;
}

auto extract_integer_bounds(const Instructions &children)
    -> ValueIntegerBounds {
  std::int64_t minimum{0};
  std::int64_t maximum{0};
  for (const auto &child : children) {
    if (child.type == InstructionIndex::AssertionGreaterEqual) {
      minimum = std::get<ValueJSON>(child.value).to_integer();
    } else if (child.type == InstructionIndex::AssertionLessEqual) {
      maximum = std::get<ValueJSON>(child.value).to_integer();
    }
  }

  return {minimum, maximum};
}

auto compiler_draft3_applicator_properties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &current,
    const bool annotate, const bool track_evaluation) -> Instructions {
  if (schema_context.is_property_name) {
    return {};
  }

  if (!schema_context.schema.at(dynamic_context.keyword).is_object()) {
    return {};
  }

  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  const bool emit_annotation{
      annotate && annotations_enabled(context, dynamic_context.keyword)};

  if (properties_as_loop(context, schema_context,
                         schema_context.schema.at(dynamic_context.keyword))) {
    ValueNamedIndexes indexes;
    Instructions children;
    std::size_t cursor = 0;

    for (auto &&[name, substeps] : compile_properties(
             context, schema_context, relative_dynamic_context(), current)) {
      indexes.emplace(name, cursor);

      if (track_evaluation) {
        substeps.push_back(make(
            sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
            schema_context, relative_dynamic_context(), ValuePointer{name}));
      }

      if (emit_annotation) {
        substeps.push_back(
            make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
                 schema_context, relative_dynamic_context(),
                 sourcemeta::core::JSON{name}));
      }

      // Note that the evaluator completely ignores this wrapper anyway
      children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                              context, schema_context,
                              relative_dynamic_context(), ValueNone{},
                              std::move(substeps)));
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

  const auto effective_dynamic_context{context.mode == Mode::FastValidation
                                           ? dynamic_context
                                           : relative_dynamic_context()};

  const auto assume_object{schema_context.schema.defines("type") &&
                           schema_context.schema.at("type").is_string() &&
                           schema_context.schema.at("type").to_string() ==
                               "object"};

  ValueStringSet required{required_properties(schema_context)};

  auto properties{compile_properties(context, schema_context,
                                     effective_dynamic_context, current)};

  if (!emit_annotation && context.mode == Mode::FastValidation &&
      !required.empty() &&
      schema_context.schema.defines("additionalProperties") &&
      schema_context.schema.at("additionalProperties").is_boolean() &&
      !schema_context.schema.at("additionalProperties").to_boolean() &&
      required.size() ==
          schema_context.schema.at(dynamic_context.keyword).size() &&
      std::ranges::all_of(properties,
                          [&required](const auto &property) -> auto {
                            return required.contains(property.first);
                          })) {
    if (std::ranges::all_of(properties, [](const auto &property) -> auto {
          return property.second.size() == 1 &&
                 property.second.front().type ==
                     InstructionIndex::AssertionTypeStrict;
        })) {
      std::set<ValueType> types;
      for (const auto &property : properties) {
        types.insert(std::get<ValueType>(property.second.front().value));
      }

      if (types.size() == 1 &&
          !schema_context.schema.defines("patternProperties")) {
        if (assume_object) {
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

    if (std::ranges::all_of(properties, [](const auto &property) -> auto {
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

  auto attempt_object_fusion{context.mode == Mode::FastValidation &&
                             !emit_annotation && !track_evaluation &&
                             assume_object};
  if (attempt_object_fusion) {
    for (const auto &entry : schema_context.schema.as_object()) {
      const auto &keyword{entry.first};
      if (keyword == "type" || keyword == "required" ||
          keyword == dynamic_context.keyword) {
        continue;
      }

      if (keyword == "additionalProperties" && entry.second.is_boolean() &&
          entry.second.to_boolean()) {
        continue;
      }

      const auto &keyword_type{
          context.walker(keyword, schema_context.vocabularies).type};
      using enum sourcemeta::blaze::SchemaKeywordType;
      if (keyword_type == Assertion || keyword_type == Annotation ||
          keyword_type == Unknown || keyword_type == Comment ||
          keyword_type == Other || keyword_type == LocationMembers) {
        continue;
      }

      attempt_object_fusion = false;
      break;
    }
  }
  ValueObjectProperties fusion_entries;
  Instructions fusion_children;
  bool fusion_possible{attempt_object_fusion};

  for (auto &&[name, substeps] : properties) {
    if (emit_annotation) {
      substeps.push_back(
          make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
               schema_context, effective_dynamic_context,
               sourcemeta::core::JSON{name}));
    }

    // Optimize `properties` where its subschemas just include a type check

    if (context.mode == Mode::FastValidation && track_evaluation &&
        substeps.size() == 1 &&
        substeps.front().type == InstructionIndex::AssertionTypeStrict) {
      children.push_back(rephrase(context,
                                  sourcemeta::blaze::InstructionIndex::
                                      AssertionPropertyTypeStrictEvaluate,
                                  substeps.front()));
    } else if (context.mode == Mode::FastValidation && track_evaluation &&
               substeps.size() == 1 &&
               substeps.front().type == InstructionIndex::AssertionType) {
      children.push_back(rephrase(
          context,
          sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeEvaluate,
          substeps.front()));
    } else if (context.mode == Mode::FastValidation && track_evaluation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionTypeStrictAny) {
      children.push_back(rephrase(context,
                                  sourcemeta::blaze::InstructionIndex::
                                      AssertionPropertyTypeStrictAnyEvaluate,
                                  substeps.front()));

      // NOLINTBEGIN(bugprone-branch-clone)
    } else if (!fusion_possible && context.mode == Mode::FastValidation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyTypeStrict) {
      children.push_back(
          unroll(context, substeps.front(),
                 effective_dynamic_context.base_instance_location));
    } else if (!fusion_possible && context.mode == Mode::FastValidation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyType) {
      children.push_back(
          unroll(context, substeps.front(),
                 effective_dynamic_context.base_instance_location));
    } else if (!fusion_possible && context.mode == Mode::FastValidation &&
               substeps.size() == 1 &&
               substeps.front().type ==
                   InstructionIndex::AssertionPropertyTypeStrictAny) {
      children.push_back(
          unroll(context, substeps.front(),
                 effective_dynamic_context.base_instance_location));
      // NOLINTEND(bugprone-branch-clone)

    } else {
      if (track_evaluation) {
        auto new_base_instance_location{
            effective_dynamic_context.base_instance_location};
        new_base_instance_location.push_back({name});
        substeps.push_back(
            make(sourcemeta::blaze::InstructionIndex::Evaluate, context,
                 schema_context,
                 DynamicContext{
                     .keyword = effective_dynamic_context.keyword,
                     .base_schema_location =
                         effective_dynamic_context.base_schema_location,
                     .base_instance_location = new_base_instance_location},
                 ValueNone{}));
      }

      if (context.mode == Mode::FastValidation && !substeps.empty()) {
        if (is_integer_type_bounded_pattern(substeps)) {
          auto bounds = extract_integer_bounds(substeps);
          const auto index =
              has_strict_integer_type(substeps)
                  ? InstructionIndex::AssertionTypeIntegerBoundedStrict
                  : InstructionIndex::AssertionTypeIntegerBounded;
          auto instance_location = substeps.front().relative_instance_location;
          substeps.clear();
          auto fused = make(index, context, schema_context,
                            relative_dynamic_context(), bounds);
          fused.relative_instance_location = std::move(instance_location);
          substeps.push_back(std::move(fused));
        } else if (is_integer_type_lower_bound_pattern(substeps)) {
          const auto minimum = extract_integer_lower_bound(substeps);
          const auto index =
              has_strict_integer_type(substeps)
                  ? InstructionIndex::AssertionTypeIntegerLowerBoundStrict
                  : InstructionIndex::AssertionTypeIntegerLowerBound;
          auto instance_location = substeps.front().relative_instance_location;
          substeps.clear();
          auto fused =
              make(index, context, schema_context, relative_dynamic_context(),
                   ValueIntegerBounds{minimum, 0});
          fused.relative_instance_location = std::move(instance_location);
          substeps.push_back(std::move(fused));
        } else if (substeps.size() == 2) {
          bool has_items_bounded{false};
          bool has_array_type{false};
          std::size_t items_index{0};
          std::size_t array_index{0};
          for (std::size_t step_index = 0; step_index < 2; step_index++) {
            if (substeps[step_index].type ==
                InstructionIndex::LoopItemsIntegerBounded) {
              has_items_bounded = true;
              items_index = step_index;
            } else if (substeps[step_index].type ==
                       InstructionIndex::AssertionTypeArrayBounded) {
              has_array_type = true;
              array_index = step_index;
            }
          }

          if (has_items_bounded && has_array_type) {
            auto integer_bounds{
                std::get<ValueIntegerBounds>(substeps[items_index].value)};
            auto range{std::get<ValueRange>(substeps[array_index].value)};
            auto instance_location =
                substeps[items_index].relative_instance_location;
            Value fused_value{
                ValueIntegerBoundsWithSize{integer_bounds, std::move(range)}};
            substeps.clear();
            auto fused =
                make(InstructionIndex::LoopItemsIntegerBoundedSized, context,
                     schema_context, effective_dynamic_context, fused_value);
            fused.relative_instance_location = std::move(instance_location);
            substeps.push_back(std::move(fused));
          }
        }
      }

      if (fusion_possible && substeps.size() >= 2 &&
          std::ranges::any_of(substeps, [](const auto &step) -> auto {
            return step.type ==
                   InstructionIndex::AssertionObjectPropertiesSimple;
          })) {
        std::erase_if(substeps, [](const auto &step) -> auto {
          if (step.type == InstructionIndex::AssertionDefinesAllStrict ||
              step.type == InstructionIndex::AssertionDefinesAll) {
            return true;
          }

          if ((step.type == InstructionIndex::AssertionTypeStrict ||
               step.type == InstructionIndex::AssertionType) &&
              std::get<ValueType>(step.value) ==
                  sourcemeta::core::JSON::Type::Object) {
            return true;
          }

          return false;
        });
      }

      if (fusion_possible && substeps.size() == 1 &&
          substeps.front().type != InstructionIndex::ControlJump &&
          substeps.front().type != InstructionIndex::ControlDynamicAnchorJump) {
        const auto is_required{assume_object && required.contains(name)};
        auto prop{make_property(name)};
        auto fusion_child{substeps.front()};
        fusion_child.relative_instance_location = {};
        auto fusion_extra{context.extra[fusion_child.extra_index]};
        fusion_extra.relative_schema_location = {};
        fusion_child.extra_index = context.extra.size();
        context.extra.push_back(std::move(fusion_extra));

        fusion_entries.emplace_back(prop.first, prop.second, is_required);
        fusion_children.push_back(std::move(fusion_child));
      } else {
        fusion_possible = false;
      }

      if (!substeps.empty()) {
        // As a performance shortcut
        if (effective_dynamic_context.base_instance_location.empty()) {
          if (assume_object && required.contains(name)) {
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
    if (fusion_possible && !fusion_entries.empty() &&
        !annotations_collected(context)) {
      for (const auto &req : required) {
        const auto &req_name{req.first};
        bool already_tracked{false};
        for (const auto &entry : fusion_entries) {
          if (std::get<0>(entry) == req_name) {
            already_tracked = true;
            break;
          }
        }
        if (!already_tracked) {
          auto prop{make_property(req_name)};
          fusion_entries.emplace_back(prop.first, prop.second, true);
        }
      }

      if (fusion_entries.size() > 32) {
        return children;
      }

      return {make(InstructionIndex::AssertionObjectPropertiesSimple, context,
                   schema_context, dynamic_context,
                   Value{std::move(fusion_entries)},
                   std::move(fusion_children))};
    }

    return children;
  } else if (children.empty()) {
    return {};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::LogicalWhenType, context,
                 schema_context, dynamic_context,
                 sourcemeta::core::JSON::Type::Object, std::move(children))};
  }
}

auto compiler_draft3_applicator_properties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &current)
    -> Instructions {
  auto property_instructions{compiler_draft3_applicator_properties_with_options(
      context, schema_context, dynamic_context, current, false, false)};

  using Known = sourcemeta::blaze::Vocabularies::Known;
  const auto is_draft3{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3_Hyper)};
  if (!is_draft3) {
    return property_instructions;
  }

  ValueStringSet required{required_properties(schema_context)};
  if (required.empty()) {
    return property_instructions;
  }

  auto required_instructions{compile_required_assertions(
      context, schema_context, dynamic_context, current, std::move(required))};
  if (required_instructions.empty()) {
    return property_instructions;
  }

  Instructions result{std::move(required_instructions)};
  for (auto &&step : property_instructions) {
    result.push_back(std::move(step));
  }
  return result;
}

auto compiler_draft3_applicator_patternproperties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_object()) {
    return {};
  }

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

  std::ranges::sort(patterns);

  // For each regular expression and corresponding subschema in the object
  for (const auto &pattern : patterns) {
    auto substeps{compile(context, schema_context, relative_dynamic_context(),
                          sourcemeta::blaze::make_weak_pointer(pattern))};

    if (annotate && annotations_enabled(context, dynamic_context.keyword)) {
      substeps.push_back(make(
          sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent,
          context, schema_context, relative_dynamic_context(), ValueNone{}));
    }

    if (track_evaluation) {
      substeps.push_back(
          make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
               schema_context, relative_dynamic_context(), ValuePointer{}));
    }

    if (context.mode == Mode::FastValidation && !track_evaluation &&
        patterns.size() == 1 &&
        (!schema_context.schema.defines("properties") ||
         (schema_context.schema.at("properties").is_object() &&
          schema_context.schema.at("properties").empty())) &&
        schema_context.schema.defines("additionalProperties") &&
        schema_context.schema.at("additionalProperties").is_boolean() &&
        !schema_context.schema.at("additionalProperties").to_boolean()) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::LoopPropertiesRegexClosed,
               context, schema_context, dynamic_context,
               ValueRegex{.first = parse_regex(pattern, schema_context.base,
                                               schema_context.relative_pointer),
                          .second = pattern},
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
        children.push_back(make(
            sourcemeta::blaze::InstructionIndex::LoopPropertiesRegex, context,
            schema_context, dynamic_context,
            ValueRegex{.first = parse_regex(pattern, schema_context.base,
                                            schema_context.relative_pointer),
                       .second = pattern},
            std::move(substeps)));
      }
    }
  }

  return children;
}

auto compiler_draft3_applicator_patternproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft3_applicator_patternproperties_with_options(
      context, schema_context, dynamic_context, false, false);
}

// Determine whether the `properties` keyword on its own enforces a closed
// object (i.e. compiles to one of the `LoopPropertiesExactly*` forms). This
// happens when every property subschema reduces to a single strict type
// assertion of the same type. In that case `additionalProperties: false` is
// already enforced by `properties` and does not need to emit anything.
inline auto
properties_enforce_closed_object(const Context &context,
                                 const SchemaContext &schema_context) -> bool {
  const bool assume_object{schema_context.schema.defines("type") &&
                           schema_context.schema.at("type").is_string() &&
                           schema_context.schema.at("type").to_string() ==
                               "object"};
  if (!assume_object || !schema_context.schema.defines("properties") ||
      !schema_context.schema.at("properties").is_object()) {
    return false;
  }

  // `SchemaContext::relative_pointer` is a reference, so the concatenated
  // pointer must outlive `new_schema_context`
  const auto properties_pointer{
      schema_context.relative_pointer.initial().concat(
          sourcemeta::blaze::make_weak_pointer(KEYWORD_PROPERTIES))};
  const SchemaContext new_schema_context{
      .relative_pointer = properties_pointer,
      .schema = schema_context.schema,
      .vocabularies = schema_context.vocabularies,
      .base = schema_context.base,
      .is_property_name = schema_context.is_property_name};
  const DynamicContext new_dynamic_context{
      .keyword = KEYWORD_PROPERTIES,
      .base_schema_location = sourcemeta::core::empty_weak_pointer,
      .base_instance_location = sourcemeta::core::empty_weak_pointer};
  const auto properties{
      compile_properties(context, new_schema_context, new_dynamic_context, {})};
  if (!std::ranges::all_of(properties, [](const auto &property) -> auto {
        return property.second.size() == 1 &&
               property.second.front().type ==
                   InstructionIndex::AssertionTypeStrict;
      })) {
    return false;
  }

  std::set<ValueType> types;
  for (const auto &property : properties) {
    types.insert(std::get<ValueType>(property.second.front().value));
  }

  return types.size() == 1;
}

auto compiler_draft3_applicator_additionalproperties_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Instructions children{compile(context, schema_context,
                                relative_dynamic_context(),
                                sourcemeta::core::empty_weak_pointer,
                                sourcemeta::core::empty_weak_pointer)};

  if (annotate && annotations_enabled(context, dynamic_context.keyword)) {
    children.push_back(
        make(sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent,
             context, schema_context, relative_dynamic_context(), ValueNone{}));
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
        static const std::string pattern_properties_keyword{
            "patternProperties"};
        filter_regexes.push_back(
            {.first =
                 parse_regex(entry.first, schema_context.base,
                             schema_context.relative_pointer.initial().concat(
                                 sourcemeta::blaze::make_weak_pointer(
                                     pattern_properties_keyword))),
             .second = entry.first});
      }
    }
  }

  // For performance, if a schema sets `additionalProperties: true` (or its
  // variants), we don't need to do anything
  if (!track_evaluation && children.empty()) {
    return {};
  }

  // When `additionalProperties: false` with only `properties` (no
  // patternProperties), and `properties` is compiled as a loop
  // (LoopPropertiesMatchClosed), that loop already handles rejecting unknown
  // properties, so we don't need to emit anything for `additionalProperties`
  if (context.mode == Mode::FastValidation && children.size() == 1 &&
      children.front().type == InstructionIndex::AssertionFail &&
      !filter_strings.empty() && filter_prefixes.empty() &&
      filter_regexes.empty() &&
      properties_as_loop(context, schema_context,
                         schema_context.schema.at("properties"))) {
    return {};
  }

  // When all properties are required and `additionalProperties: false`, the
  // object is closed by another keyword, so we don't need to emit anything for
  // `additionalProperties`. This happens either because `required` compiles to
  // an `AssertionDefinesExactly` variant (only when there is more than one
  // required property) or because `properties` itself compiles to a closed
  // form. With a single required property `required` only compiles to
  // `AssertionDefinesStrict`, which does not reject unknown properties, so we
  // must still emit the closure unless `properties` enforces it.
  if (context.mode == Mode::FastValidation && children.size() == 1 &&
      children.front().type == InstructionIndex::AssertionFail &&
      !filter_strings.empty() && filter_prefixes.empty() &&
      filter_regexes.empty()) {
    const auto required{required_properties(schema_context)};
    if (is_closed_properties_required(schema_context.schema, required) &&
        (required.size() > 1 ||
         (properties_enforce_closed_object(context, schema_context) &&
          !annotations_enabled(context, "properties")))) {
      return {};
    }
  }

  if (context.mode == Mode::FastValidation && filter_strings.empty() &&
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
               schema_context, relative_dynamic_context(), ValuePointer{}));
    }

    return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesExcept,
                 context, schema_context, dynamic_context,
                 ValuePropertyFilter{std::move(filter_strings),
                                     std::move(filter_prefixes),
                                     std::move(filter_regexes)},
                 std::move(children))};
  } else if (track_evaluation) {
    if (children.empty()) {
      return {make(sourcemeta::blaze::InstructionIndex::Evaluate, context,
                   schema_context, dynamic_context, ValueNone{})};
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

auto compiler_draft3_applicator_additionalproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft3_applicator_additionalproperties_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft3_validation_pattern(const Context &context,
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

  const auto &regex_string{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  return {
      make(sourcemeta::blaze::InstructionIndex::AssertionRegex, context,
           schema_context, dynamic_context,
           ValueRegex{.first = parse_regex(regex_string, schema_context.base,
                                           schema_context.relative_pointer),
                      .second = regex_string})};
}

auto compiler_draft3_applicator_items_array(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (schema_context.is_property_name) {
    return {};
  }

  if (!schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return {};
  }

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

  const bool emit_annotation{
      annotate && annotations_enabled(context, dynamic_context.keyword)};

  // Precompile subschemas
  std::vector<Instructions> subschemas;
  subschemas.reserve(items_size);
  const auto &array{
      schema_context.schema.at(dynamic_context.keyword).as_array()};
  for (auto iterator{array.cbegin()}; iterator != array.cend(); ++iterator) {
    subschemas.push_back(compile(context, schema_context,
                                 relative_dynamic_context(),
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

    if (emit_annotation) {
      subchildren.push_back(
          make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
               schema_context, relative_dynamic_context(),
               sourcemeta::core::JSON{cursor}));
    }

    children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                            context, schema_context, relative_dynamic_context(),
                            ValueNone{}, std::move(subchildren)));
  }

  Instructions tail;
  for (const auto &subschema : subschemas) {
    for (const auto &substep : subschema) {
      tail.push_back(substep);
    }
  }

  if (emit_annotation) {
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                        context, schema_context, relative_dynamic_context(),
                        sourcemeta::core::JSON{children.size() - 1}));
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                        context, schema_context, relative_dynamic_context(),
                        sourcemeta::core::JSON{true}));
  }

  children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                          context, schema_context, relative_dynamic_context(),
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

auto is_number_type_check(const Instruction &instruction) -> bool {
  if (instruction.type != InstructionIndex::AssertionTypeStrictAny) {
    return false;
  }

  const auto &value{std::get<ValueTypes>(instruction.value)};
  const auto numeric_count{
      static_cast<std::size_t>(value.test(
          std::to_underlying(sourcemeta::core::JSON::Type::Integer))) +
      static_cast<std::size_t>(
          value.test(std::to_underlying(sourcemeta::core::JSON::Type::Real))) +
      static_cast<std::size_t>(value.test(
          std::to_underlying(sourcemeta::core::JSON::Type::Decimal)))};
  return numeric_count >= 2 && value.count() == numeric_count;
}

auto is_integer_bounded_pattern(const Instructions &children) -> bool {
  if (children.size() != 3) {
    return false;
  }

  bool has_type{false};
  bool has_min{false};
  bool has_max{false};
  for (const auto &child : children) {
    if (is_number_type_check(child)) {
      has_type = true;
    } else if (child.type == InstructionIndex::AssertionGreaterEqual) {
      if (!std::get<ValueJSON>(child.value).is_integer()) {
        return false;
      }
      has_min = true;
    } else if (child.type == InstructionIndex::AssertionLessEqual) {
      if (!std::get<ValueJSON>(child.value).is_integer()) {
        return false;
      }
      has_max = true;
    }
  }

  return has_type && has_min && has_max;
}

auto compiler_draft3_applicator_items_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  const bool emit_annotation{
      annotate && annotations_enabled(context, dynamic_context.keyword)};

  if (is_schema(schema_context.schema.at(dynamic_context.keyword))) {
    if (emit_annotation || track_evaluation) {
      Instructions subchildren{compile(context, schema_context,
                                       relative_dynamic_context(),
                                       sourcemeta::core::empty_weak_pointer,
                                       sourcemeta::core::empty_weak_pointer)};

      Instructions children;

      if (!subchildren.empty()) {
        children.push_back(make(sourcemeta::blaze::InstructionIndex::LoopItems,
                                context, schema_context, dynamic_context,
                                ValueNone{}, std::move(subchildren)));
      }

      if (!emit_annotation && !track_evaluation) {
        return children;
      }

      Instructions tail;

      if (emit_annotation) {
        tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                            context, schema_context, relative_dynamic_context(),
                            sourcemeta::core::JSON{true}));
      }

      if (track_evaluation) {
        tail.push_back(
            make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
                 schema_context, relative_dynamic_context(), ValuePointer{}));
      }

      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::LogicalWhenType, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON::Type::Array, std::move(tail)));

      return children;
    }

    Instructions children{compile(context, schema_context,
                                  relative_dynamic_context(),
                                  sourcemeta::core::empty_weak_pointer,
                                  sourcemeta::core::empty_weak_pointer)};
    if (track_evaluation) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
               schema_context, relative_dynamic_context(), ValuePointer{}));
    }

    if (children.empty()) {
      return {};
    }

    if (context.mode == Mode::FastValidation && children.size() == 3 &&
        is_integer_bounded_pattern(children)) {
      return {make(sourcemeta::blaze::InstructionIndex::LoopItemsIntegerBounded,
                   context, schema_context, dynamic_context,
                   extract_integer_bounds(children))};
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
          return {Instruction{.type = sourcemeta::blaze::InstructionIndex::
                                  LoopItemsPropertiesExactlyTypeStrictHash3,
                              .relative_instance_location =
                                  current.relative_instance_location,
                              .value = std::move(value_copy),
                              .children = {},
                              .extra_index = current.extra_index}};
        }

        return {Instruction{.type = sourcemeta::blaze::InstructionIndex::
                                LoopItemsPropertiesExactlyTypeStrictHash,
                            .relative_instance_location =
                                current.relative_instance_location,
                            .value = std::move(value_copy),
                            .children = {},
                            .extra_index = current.extra_index}};
      }
    }

    return {make(sourcemeta::blaze::InstructionIndex::LoopItems, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }

  return compiler_draft3_applicator_items_array(
      context, schema_context, dynamic_context, annotate, track_evaluation);
}

auto compiler_draft3_applicator_items(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  return compiler_draft3_applicator_items_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft3_applicator_additionalitems_from_cursor(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const std::size_t cursor,
    const bool annotate, const bool track_evaluation) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  const bool emit_annotation{
      annotate && annotations_enabled(context, dynamic_context.keyword)};

  Instructions subchildren{compile(context, schema_context,
                                   relative_dynamic_context(),
                                   sourcemeta::core::empty_weak_pointer,
                                   sourcemeta::core::empty_weak_pointer)};

  Instructions children;

  if (!subchildren.empty()) {
    if (context.mode == Mode::FastValidation && cursor == 0 &&
        !emit_annotation && !track_evaluation &&
        is_integer_bounded_pattern(subchildren)) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::LoopItemsIntegerBounded,
               context, schema_context, dynamic_context,
               extract_integer_bounds(subchildren)));
      return children;
    }

    children.push_back(make(sourcemeta::blaze::InstructionIndex::LoopItemsFrom,
                            context, schema_context, dynamic_context,
                            ValueUnsignedInteger{cursor},
                            std::move(subchildren)));
  }

  // Avoid one extra wrapper instruction if possible
  if (!emit_annotation && !track_evaluation) {
    return children;
  }

  Instructions tail;

  if (emit_annotation) {
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::AnnotationEmit,
                        context, schema_context, relative_dynamic_context(),
                        sourcemeta::core::JSON{true}));
  }

  if (track_evaluation) {
    tail.push_back(make(sourcemeta::blaze::InstructionIndex::ControlEvaluate,
                        context, schema_context, relative_dynamic_context(),
                        ValuePointer{}));
  }

  assert(!tail.empty());
  children.push_back(
      make(sourcemeta::blaze::InstructionIndex::LogicalWhenArraySizeGreater,
           context, schema_context, dynamic_context,
           ValueUnsignedInteger{cursor}, std::move(tail)));

  return children;
}

auto compiler_draft3_applicator_additionalitems_with_options(
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

  return compiler_draft3_applicator_additionalitems_from_cursor(
      context, schema_context, dynamic_context, cursor, annotate,
      track_evaluation);
}

auto compiler_draft3_applicator_additionalitems(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft3_applicator_additionalitems_with_options(
      context, schema_context, dynamic_context, false, false);
}

auto compiler_draft3_validation_enum(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context,
                                     const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return {};
  }

  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                 schema_context, dynamic_context, ValueNone{})};
  }

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

auto compiler_draft3_validation_uniqueitems(
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

auto compiler_draft3_validation_maxlength(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context,
                                          const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_integral()) {
    return {};
  }

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

auto compiler_draft3_validation_minlength(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context,
                                          const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_integral()) {
    return {};
  }

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

  const auto value{
      schema_context.schema.at(dynamic_context.keyword).as_integer()};
  if (value <= 0) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionStringSizeGreater,
               context, schema_context, dynamic_context,
               ValueUnsignedInteger{static_cast<unsigned long>(value - 1)})};
}

auto compiler_draft3_validation_maxitems(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_integral()) {
    return {};
  }

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

auto compiler_draft3_validation_minitems(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_integral()) {
    return {};
  }

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

  const auto value{
      schema_context.schema.at(dynamic_context.keyword).as_integer()};
  if (value <= 0) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionArraySizeGreater,
               context, schema_context, dynamic_context,
               ValueUnsignedInteger{static_cast<unsigned long>(value - 1)})};
}

auto compiler_draft3_validation_maximum(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_number()) {
    return {};
  }

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

auto compiler_draft3_validation_minimum(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_number()) {
    return {};
  }

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

auto compiler_draft3_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context,
                                     const Instructions &) -> Instructions {
  const auto &value{schema_context.schema.at(dynamic_context.keyword)};

  using Known = sourcemeta::blaze::Vocabularies::Known;
  const auto is_draft3{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3_Hyper)};

  if (is_draft3) {
    if (value.is_string() && value.to_string() == "any") {
      return draft3_any_type_instructions(context, schema_context,
                                          dynamic_context);
    }

    if (value.is_array()) {
      bool has_object{false};
      for (const auto &element : value.as_array()) {
        if (element.is_string() && element.to_string() == "any") {
          return draft3_any_type_instructions(context, schema_context,
                                              dynamic_context);
        }
        if (element.is_object()) {
          has_object = true;
        }
      }

      if (has_object) {
        if (context.mode == Mode::FastValidation && value.size() == 1) {
          return compile(
              context, schema_context, dynamic_context,
              {static_cast<sourcemeta::core::Pointer::Token::Index>(0)});
        }

        Instructions disjunctors;
        for (std::uint64_t index = 0; index < value.size(); index++) {
          const auto &element{value.at(index)};
          Instructions branch;

          if (element.is_object()) {
            branch = compile(
                context, schema_context, relative_dynamic_context(),
                {static_cast<sourcemeta::core::Pointer::Token::Index>(index)});
          } else if (element.is_string()) {
            const auto &type_string{element.to_string()};
            if (type_string == "null") {
              branch.push_back(
                  make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                       context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::JSON::Type::Null));
            } else if (type_string == "boolean") {
              branch.push_back(
                  make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                       context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::JSON::Type::Boolean));
            } else if (type_string == "object") {
              branch.push_back(
                  make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                       context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::JSON::Type::Object));
            } else if (type_string == "array") {
              branch.push_back(
                  make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                       context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::JSON::Type::Array));
            } else if (type_string == "number") {
              ValueTypes types{};
              types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
              types.set(
                  std::to_underlying(sourcemeta::core::JSON::Type::Integer));
              types.set(
                  std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
              branch.push_back(make(
                  sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                  context, schema_context, relative_dynamic_context(), types));
            } else if (type_string == "integer") {
              branch.push_back(
                  make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                       context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::JSON::Type::Integer));
            } else if (type_string == "string") {
              branch.push_back(
                  make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                       context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::JSON::Type::String));
            } else {
              continue;
            }
          } else {
            continue;
          }

          disjunctors.push_back(
              make(sourcemeta::blaze::InstructionIndex::ControlGroup, context,
                   schema_context, relative_dynamic_context(), ValueNone{},
                   std::move(branch)));
        }

        return {make(sourcemeta::blaze::InstructionIndex::LogicalOr, context,
                     schema_context, dynamic_context, ValueBoolean{false},
                     std::move(disjunctors))};
      }
    }
  }

  if (value.is_string()) {
    const auto &type{value.to_string()};
    if (type == "null") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_null();
                              })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Null)};
    } else if (type == "boolean") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_boolean();
                              })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Boolean)};
    } else if (type == "object") {
      if (!is_draft3) {
        const auto minimum{unsigned_integer_property(schema_context.schema,
                                                     "minProperties", 0)};
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
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_object();
                              })) {
        return {};
      }

      if (!is_draft3 && context.mode == Mode::FastValidation &&
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
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_array();
                              })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Array)};
    } else if (type == "number") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_number();
                              })) {
        return {};
      }

      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, types)};
    } else if (type == "integer") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_integer();
                              })) {
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
          std::ranges::all_of(schema_context.schema.at("enum").as_array(),
                              [](const auto &candidate) -> auto {
                                return candidate.is_string();
                              })) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (value.is_array() && value.size() == 1 &&
             value.front().is_string()) {
    const auto &type{value.front().to_string()};
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
      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, types)};
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
  } else if (value.is_array()) {
    ValueTypes types{};
    for (const auto &element : value.as_array()) {
      assert(element.is_string());
      const auto &type_string{element.to_string()};
      if (type_string == "null") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Null));
      } else if (type_string == "boolean") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Boolean));
      } else if (type_string == "object") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Object));
      } else if (type_string == "array") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Array));
      } else if (type_string == "number") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      } else if (type_string == "integer") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      } else if (type_string == "string") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::String));
      }
    }

    if (!types.any()) {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                   schema_context, dynamic_context, ValueNone{})};
    }

    return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                 context, schema_context, dynamic_context, types)};
  }

  return {};
}

auto compiler_draft3_validation_disallow(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &) -> Instructions {
  const auto &value{schema_context.schema.at(dynamic_context.keyword)};

  const auto contains_any{
      (value.is_string() && value.to_string() == "any") ||
      (value.is_array() &&
       std::ranges::any_of(value.as_array(), [](const auto &element) -> auto {
         return element.is_string() && element.to_string() == "any";
       }))};
  if (contains_any) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                 schema_context, dynamic_context, ValueNone{})};
  }

  ValueTypes types{};
  Instructions subschema_nots;

  if (value.is_string()) {
    draft3_set_type_bits(value.to_string(), types);
  } else if (value.is_array()) {
    for (std::uint64_t index = 0; index < value.size(); index++) {
      const auto &element{value.at(index)};
      if (element.is_string()) {
        draft3_set_type_bits(element.to_string(), types);
      } else if (element.is_object()) {
        sourcemeta::core::WeakPointer index_suffix;
        index_suffix.push_back(index);
        const auto element_uri{
            sourcemeta::core::to_uri(
                schema_context.relative_pointer.concat(index_suffix),
                schema_context.base)
                .canonicalize()
                .recompose()};

        auto inner_instructions{
            compile(context, schema_context, relative_dynamic_context(),
                    sourcemeta::core::empty_weak_pointer,
                    sourcemeta::core::empty_weak_pointer, element_uri)};

        const auto element_relative_pointer{
            schema_context.relative_pointer.concat(index_suffix)};
        const SchemaContext element_schema_context{
            .relative_pointer = element_relative_pointer,
            .schema = schema_context.schema,
            .vocabularies = schema_context.vocabularies,
            .base = schema_context.base,
            .is_property_name = schema_context.is_property_name};

        const auto element_base_schema_location{
            sourcemeta::blaze::make_weak_pointer(dynamic_context.keyword,
                                                 index)};
        const DynamicContext element_dynamic_context{
            .keyword = KEYWORD_EMPTY,
            .base_schema_location = element_base_schema_location,
            .base_instance_location = dynamic_context.base_instance_location};

        subschema_nots.push_back(
            make(sourcemeta::blaze::InstructionIndex::LogicalNot, context,
                 element_schema_context, element_dynamic_context, ValueNone{},
                 std::move(inner_instructions)));
      }
    }
  }

  Instructions result;
  if (types.any()) {
    result.push_back(
        make(sourcemeta::blaze::InstructionIndex::AssertionNotTypeStrictAny,
             context, schema_context, dynamic_context, types));
  }
  for (auto &&instruction : subschema_nots) {
    result.push_back(std::move(instruction));
  }
  return result;
}

auto compiler_draft3_applicator_extends(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &) -> Instructions {
  const auto &value{schema_context.schema.at(dynamic_context.keyword)};

  if (value.is_object()) {
    // TODO: Make this work with `$dynamicRef`
    if (context.mode == Mode::FastValidation && !context.uses_dynamic_scopes) {
      return compile(context, schema_context, dynamic_context,
                     sourcemeta::core::empty_weak_pointer,
                     sourcemeta::core::empty_weak_pointer);
    }

    auto inner{compile(context, schema_context, relative_dynamic_context(),
                       sourcemeta::core::empty_weak_pointer,
                       sourcemeta::core::empty_weak_pointer)};
    if (inner.empty()) {
      return {};
    }

    return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(inner))};
  }

  if (!value.is_array()) {
    return {};
  }

  if (value.empty()) {
    return {};
  }

  Instructions children;

  // TODO: Make this work with `$dynamicRef`
  if (context.mode == Mode::FastValidation && !context.uses_dynamic_scopes) {
    for (std::uint64_t index = 0; index < value.size(); index++) {
      for (auto &&step : compile(
               context, schema_context, dynamic_context,
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return children;
  }

  for (std::uint64_t index = 0; index < value.size(); index++) {
    auto arm{
        compile(context, schema_context, relative_dynamic_context(),
                {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})};
    if (arm.empty()) {
      continue;
    }
    children.push_back(make(sourcemeta::blaze::InstructionIndex::ControlGroup,
                            context, schema_context, relative_dynamic_context(),
                            ValueNone{}, std::move(arm)));
  }

  if (children.empty()) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
               schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

auto compiler_draft3_applicator_dependencies(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  if (!schema_context.schema.at(dynamic_context.keyword).is_object()) {
    return {};
  }

  using Known = sourcemeta::blaze::Vocabularies::Known;
  const auto is_draft3{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3_Hyper)};

  Instructions children;
  ValueStringMap dependencies;

  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (is_schema(entry.second)) {
      if (!entry.second.is_boolean() || !entry.second.to_boolean()) {
        children.push_back(make(
            sourcemeta::blaze::InstructionIndex::LogicalWhenDefines, context,
            schema_context, dynamic_context, make_property(entry.first),
            compile(context, schema_context, relative_dynamic_context(),
                    sourcemeta::blaze::make_weak_pointer(entry.first))));
      }
    } else if (entry.second.is_array()) {
      std::vector<sourcemeta::core::JSON::String> properties;
      for (const auto &property : entry.second.as_array()) {
        assert(property.is_string());
        properties.push_back(property.to_string());
      }

      if (!properties.empty()) {
        dependencies.emplace(entry.first, properties);
      }
    } else if (is_draft3 && entry.second.is_string()) {
      dependencies.emplace(entry.first,
                           std::vector<sourcemeta::core::JSON::String>{
                               entry.second.to_string()});
    }
  }

  if (!dependencies.empty()) {
    children.push_back(make(
        sourcemeta::blaze::InstructionIndex::AssertionPropertyDependencies,
        context, schema_context, dynamic_context, std::move(dependencies)));
  }

  return children;
}

auto compiler_draft3_validation_divisibleby(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_number()) {
    return {};
  }

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

auto compiler_draft3_validation_format(const Context &context,
                                       const SchemaContext &schema_context,
                                       const DynamicContext &dynamic_context,
                                       const Instructions &) -> Instructions {
  using Known = sourcemeta::blaze::Vocabularies::Known;
  static constexpr auto unsupported_dialect_message{
      "The format assertion tweak not supported in this dialect"};

  const auto is_2019_09_format{
      schema_context.vocabularies.contains(Known::JSON_Schema_2019_09_Format)};
  const auto is_2020_12_format_annotation{schema_context.vocabularies.contains(
      Known::JSON_Schema_2020_12_Format_Annotation)};
  const auto is_2020_12_format_assertion{schema_context.vocabularies.contains(
      Known::JSON_Schema_2020_12_Format_Assertion)};

  const auto force_assertion{
      schema_context.schema.is_object() &&
      schema_context.schema.defines("x-format-assertion") &&
      schema_context.schema.at("x-format-assertion").is_boolean() &&
      schema_context.schema.at("x-format-assertion").to_boolean()};

  const auto assert_active{context.tweaks.format_assertion || force_assertion};

  if ((is_2019_09_format && assert_active) || is_2020_12_format_assertion ||
      (is_2020_12_format_annotation && assert_active)) {
    const auto &format{schema_context.schema.at(dynamic_context.keyword)};
    if (!format.is_string()) {
      return {};
    }

    const auto &name{format.to_string()};
    ValueStringType type;
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "time") {
      type = ValueStringType::Time;
    } else if (name == "duration") {
      type = ValueStringType::Duration;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "idn-email") {
      type = ValueStringType::IDNEmail;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "idn-hostname") {
      type = ValueStringType::IDNHostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (name == "uri-reference") {
      type = ValueStringType::URIReference;
    } else if (name == "iri") {
      type = ValueStringType::IRI;
    } else if (name == "iri-reference") {
      type = ValueStringType::IRIReference;
    } else if (name == "uri-template") {
      type = ValueStringType::URITemplate;
    } else if (name == "json-pointer") {
      type = ValueStringType::JSONPointer;
    } else if (name == "relative-json-pointer") {
      type = ValueStringType::RelativeJSONPointer;
    } else if (name == "regex") {
      type = ValueStringType::Regex;
    } else if (name == "uuid") {
      type = ValueStringType::UUID;
    } else {
      return {};
    }

    Instructions instructions{
        make(sourcemeta::blaze::InstructionIndex::AssertionStringType, context,
             schema_context, dynamic_context, type)};

    if (annotations_enabled(context, dynamic_context.keyword)) {
      Instructions annotation_children{
          make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON{
                   schema_context.schema.at(dynamic_context.keyword)})};
      instructions.push_back(
          make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenType,
               context, schema_context, relative_dynamic_context(),
               ValueType::String, std::move(annotation_children)));
    }

    return instructions;
  }

  if (is_2019_09_format || is_2020_12_format_annotation) {
    if (!annotations_enabled(context, dynamic_context.keyword)) {
      return {};
    }

    Instructions children{
        make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
             schema_context, dynamic_context,
             sourcemeta::core::JSON{
                 schema_context.schema.at(dynamic_context.keyword)})};

    return {make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenType,
                 context, schema_context, relative_dynamic_context(),
                 ValueType::String, std::move(children))};
  }

  if (!assert_active) {
    return {};
  }

  const auto &format{schema_context.schema.at(dynamic_context.keyword)};
  if (!format.is_string()) {
    return {};
  }

  const auto &name{format.to_string()};
  ValueStringType type;

  const auto is_draft3{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_3_Hyper)};
  const auto is_draft4{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_4) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_4_Hyper)};
  const auto is_draft6{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_6) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_6_Hyper)};
  const auto is_draft7{
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_7) ||
      schema_context.vocabularies.contains(Known::JSON_Schema_Draft_7_Hyper)};

  if (is_draft7) {
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "time") {
      type = ValueStringType::Time;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "idn-email") {
      type = ValueStringType::IDNEmail;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "idn-hostname") {
      type = ValueStringType::IDNHostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (name == "uri-reference") {
      type = ValueStringType::URIReference;
    } else if (name == "uri-template") {
      type = ValueStringType::URITemplate;
    } else if (name == "json-pointer") {
      type = ValueStringType::JSONPointer;
    } else if (name == "relative-json-pointer") {
      type = ValueStringType::RelativeJSONPointer;
    } else if (name == "regex") {
      type = ValueStringType::Regex;
    } else if (name == "iri") {
      type = ValueStringType::IRI;
    } else if (name == "iri-reference") {
      type = ValueStringType::IRIReference;
    } else {
      return {};
    }
  } else if (is_draft4 || is_draft6) {
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "hostname") {
      type = ValueStringType::Hostname;
    } else if (name == "ipv4") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (is_draft6 && name == "uri-reference") {
      type = ValueStringType::URIReference;
    } else if (is_draft6 && name == "uri-template") {
      type = ValueStringType::URITemplate;
    } else if (is_draft6 && name == "json-pointer") {
      type = ValueStringType::JSONPointer;
    } else {
      return {};
    }
  } else if (is_draft3) {
    if (name == "date-time") {
      type = ValueStringType::DateTime;
    } else if (name == "email") {
      type = ValueStringType::Email;
    } else if (name == "host-name") {
      type = ValueStringType::Hostname;
    } else if (name == "ip-address") {
      type = ValueStringType::IPv4;
    } else if (name == "ipv6") {
      type = ValueStringType::IPv6;
    } else if (name == "uri") {
      type = ValueStringType::URI;
    } else if (name == "date") {
      type = ValueStringType::Date;
    } else if (name == "time") {
      type = ValueStringType::PartialTime;
    } else if (name == "utc-millisec") {
      // TODO: Support this old format, even though not even the official test
      // suite covers it
      throw sourcemeta::blaze::CompilerError(
          schema_context.base, to_pointer(schema_context.relative_pointer),
          "The \"utc-millisec\" format is not supported in assertion mode yet");
    } else if (name == "regex") {
      type = ValueStringType::Regex;
    } else if (name == "color") {
      type = ValueStringType::Color;
    } else if (name == "style") {
      // TODO: Support this old format, even though not even the official test
      // suite covers it
      throw sourcemeta::blaze::CompilerError(
          schema_context.base, to_pointer(schema_context.relative_pointer),
          "The \"style\" format is not supported in assertion mode yet");
    } else if (name == "phone") {
      // TODO: Support this old format, even though not even the official test
      // suite covers it
      throw sourcemeta::blaze::CompilerError(
          schema_context.base, to_pointer(schema_context.relative_pointer),
          "The \"phone\" format is not supported in assertion mode yet");
    } else {
      return {};
    }
  } else {
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, to_pointer(schema_context.relative_pointer),
        unsupported_dialect_message);
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionStringType,
               context, schema_context, dynamic_context, type)};
}

} // namespace internal
#endif
